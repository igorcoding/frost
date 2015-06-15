#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <stddef.h>
#include "http_server.h"

#define SELF(w, type, f) (type*) ( (char *) &(w) - offsetof(type, f))

namespace frost {

    http_server::http_server(uint16_t port)
        : _state(state::IDLE),
          _port(port),
          _listenfd(-1),
          _active_connections(0) {
    }

    http_server::~http_server() {
        if (_state != state::STOPPED) {
            stop();
        }
        for (auto it = _signals.begin(); it != _signals.end(); ++it) {
            delete it->second;
            it->second = nullptr;
        }
    }


    int http_server::start() {
        int l = listen();
        if (l != 0) return l;
        accept();
        return 0;
    }

    void http_server::stop() {
        if (_listenfd > -1) {
            ::close(_listenfd);
            _listenfd = -1;
        }
        _accept_w.stop();
        stop_signal_watchers();
        _loop.break_loop(ev::how_t::ALL);
        _state = state::STOPPED;
    }

    int http_server::listen() {
        if ((_listenfd = start_listen()) < 0) {
            return -1;
        }
        std::cout << "Listening 0.0.0.0:" << _port << std::endl;
        add_on_exit_watchers();
        return 0;
    }

    void http_server::accept() {
        _accept_w.set<http_server, &http_server::accept_cb>(this);
        _accept_w.set(_listenfd, ev::READ);

        start_signal_watchers();
        _accept_w.start();
        _state = state::STARTED;
        _loop.run();
    }

    void http_server::notify_fork_child() {
        _loop.post_fork();
    }

    int http_server::start_listen() {
        struct sockaddr_in serv_addr;
        int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

        if (sockfd < 0) {
            perror("Error creating socket");
            return -1;
        }

        int yes = 1;
        if (::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
            perror("Error setting socket options");
            return -1;
        }

        ::bzero((char *) &serv_addr, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(_port);

        if (::bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("Bind error");
            return -1;
        }

        if (::listen(sockfd, BACKLOG_SIZE) < 0) {
            perror("Listen error");
            return -1;
        }

        return sockfd;
    }

    void http_server::signal_cb(ev::sig& w, int revents) {
        w.stop();
        if (ev::ERROR & revents) {
            std::cerr << "error" << std::endl;
            return;
        }
        int sig = w.signum;

        auto s = _signals_cb.find(sig);
        if (s != _signals_cb.end()) {
            s->second();
        }
    }

    void http_server::accept_cb(ev::io& w, int revents) {
        if (ev::ERROR & revents) {
            perror("[http_server::accept_cb] error");
            return;
        }

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

//        std::cout << "active connections: " << _active_connections << std::endl;
        int client_fd = ::accept(w.fd, (struct sockaddr *) &client_addr, &client_len);
        if (client_fd < 0) {
            perror("[http_server::accept_cb] accept error");
            return;
        }
        ++_active_connections;
        create_request(client_fd);
    }

    void http_server::read_cb(ev::io& w, int revents) {
        if (ev::ERROR & revents) {
            perror("read_cb: ERROR");
            return;
        }
        auto req = SELF(w, http_request, _rw);
        w.stop();
        req->_tw.stop();

        again:
            ssize_t nread = ::recv(w.fd, req->_rbuf + req->_ruse, req->_rlen - req->_ruse, 0);
            if (nread > 0) {
                req->_ruse += nread;

                auto p = req->parse();
//                auto p = parse_result::GOOD;
                switch (p) {
                    case parse_result::NEED_MORE: {
                        if (req->_ruse >= req->_rlen) {
                            auto resp = create_response(req);
                            resp->write(status_code::REQUEST_ENTITY_TOO_LARGE, "", 0);
                            resp->finish();
                        } else {
                            w.start();
                            req->_tw.again();
                        }
                        return;
                    }
                    case parse_result::GOOD: {
//                         printf("====\nGot request:\n%.*s\n====\n", req->_ruse, req->_rbuf);
                        auto resp = create_response(req);
                        auto cb = _router.get_route(req->path());
                        if (cb == nullptr) {
                            char* buf = new char[1024];
                            int len = snprintf(buf, 1024, "Path \'%.*s\' not found on the server",
                                               (int) req->path().length(), req->path().c_str());
                            if (len >= 0) {
                                resp->write(status_code::NOT_FOUND, buf, static_cast<size_t>(len));
                            } else {
                                perror("[http_server::read_cb] snprintf failed");
                            }
                            resp->finish();
                            delete[] buf;
                        } else {
                            // TODO: check if method is allowed
                            (*cb)(req, resp);
                        }

                        return;
                    }
                    case parse_result::BAD: {
                        auto resp = create_response(req);
                        resp->write(status_code::BAD_REQUEST, "Couldn\'t parse your reqeust", 27);
                        resp->finish();
                        return;
                    }
                }

            } else if (nread < 0) {
                switch (errno) {
                    case EAGAIN:
                        return;
                    case EINTR:
                        goto again;
                    default:
                        perror("read error");
                        delete req;
                }
            } else {
                --_active_connections;
                auto resp = req->get_http_response();
                delete resp;
                delete req;
            }
    }

    void http_server::write_cb(ev::io& w, int revents) {
        if (ev::ERROR & revents) {
            perror("[http_server::write_cb] ERROR");
            return;
        }
        auto resp = SELF(w, http_response, _ww);
        http_request* req = resp->_req;
        w.stop();
        resp->_tw.stop();

        again:
            ssize_t written = ::writev(resp->_client_fd, resp->_wbuf, resp->_wuse);

            if (written > -1) {
                size_t wr = static_cast<size_t>(written);
                size_t i;
                iovec* iov;
                for (i = 0; i < resp->_wuse; i++) {
                    iov = &(resp->_wbuf[i]);
                    if (wr < iov->iov_len) {
                        memmove(iov->iov_base, (char*) iov->iov_base + wr, iov->iov_len - wr);
                        iov->iov_len -= wr;
                        break;
                    } else {
                        free(iov->iov_base);
                        wr -= iov->iov_len;
                    }
                }
                resp->_wuse -= i;
                if (resp->_wuse == 0) {
                    if (resp->finished()) {
                        if (req->is_keep_alive()) {
                            delete resp;
                            req->clear();
                            req->_keep_alive_w.set<http_server, &http_server::keep_alive_cb>(this);
                            req->start_keep_alive();
                        } else {
                            --_active_connections;
                            delete resp;
                            resp = nullptr;
                            delete req;
                            req = nullptr;
                        }
                    } else {
                        resp->_tw.start();
                    }
                } else {
                    memmove(resp->_wbuf, resp->_wbuf + i, resp->_wuse * sizeof(iovec));
                    w.start();
                    resp->_tw.again();
                    return;
                }

            } else if (written < 0) {
                switch (errno) {
                    case EAGAIN:
                        break;
                    case EINTR:
                        goto again;
                    default:
                        perror("write error");
                        delete resp;
                        delete req;
                }
                return;
            }
    }

    void http_server::read_timeout_cb(ev::timer& w, int revents) {
        std::cout << "read timeout" << std::endl;
        auto req = SELF(w, http_request, _tw);
        delete req->get_http_response();
        delete req;
    }

    void http_server::keep_alive_cb(ev::timer& w, int revents) {
        std::cout << "keep alive timeout" << std::endl;
        w.stop();
        auto req = SELF(w, http_request, _keep_alive_w);
        delete req;
    }

    void http_server::write_timeout_cb(ev::timer& w, int revents) {
        std::cout << "write timeout" << std::endl;
        auto resp = SELF(w, http_response, _tw);
        auto req = resp->_req;
        delete resp;
        delete req;
    }

    void http_server::start_signal_watchers() {
        for (auto it = _signals.begin(); it != _signals.end(); ++it) {
            it->second->start();
        }
    }

    void http_server::stop_signal_watchers() {
        for (auto it = _signals.begin(); it != _signals.end(); ++it) {
            it->second->stop();
        }
    }

    void http_server::add_on_exit_watchers() {
        auto term_cb = [this]() {
            std::cerr << "Shutting down" << std::endl;
            this->stop();
        };
        int signals[] = {
            SIGINT,
            SIGTERM,
            SIGABRT,
#ifdef SIGQUIT
            SIGQUIT,
#endif
        };
        for (auto sig : signals) {
            if (!has_signal_watcher(sig))
                on_signal(sig, term_cb);
        }
    }
}
