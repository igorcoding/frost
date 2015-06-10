#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include "http_server.h"

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
        if ((_listenfd = start_listen()) < 0) {
            return -1;
        }
        std::cout << "Listening 0.0.0.0:" << _port << std::endl;
        add_on_exit_watchers();

        _accept_w.set<http_server, &http_server::accept_cb>(this);
        _accept_w.set(_listenfd, ev::READ);

        start_signal_watchers();
        _accept_w.start();
        _state = state::STARTED;
        _loop.run();
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
        } else if (sig == SIGINT || sig == SIGTERM) {
            std::cerr << "Terminating..." << std::endl;
            stop();
        }
    }

    void http_server::accept_cb(ev::io& w, int revents) {
        // w.stop(); // TODO
        if (ev::ERROR & revents) {
            perror("Got invalid event");
            return;
        }

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = ::accept(w.fd, (struct sockaddr *) &client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept error");
            return;
        }
        ++_active_connections;
        http_request* req = new http_request(_loop, client_fd, _router);
        req->set_cb(std::bind(&http_server::request_cb, this, std::placeholders::_1));
        req->start();
    }

    void http_server::request_cb(http_request* req) {
        // TODO: prepare http_response and call appropriate cb or 404 if no cb specified
        http_response* resp = new http_response(_loop, req->_client_fd, req);
        resp->set_cb(std::bind(&http_server::response_cb, this, std::placeholders::_1, std::placeholders::_2));
        resp->start();
//        auto cb = _router.get_route(req->path());
        auto cb = _router.get_route("/");
        if (cb == nullptr) {
            // TODO: 404
        } else {
            // TODO: check if method is allowed
            (*cb)(req, resp);
        }
    }

    void http_server::response_cb(http_request* req, http_response* resp) {
        delete resp;
        delete req;
        --_active_connections;
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