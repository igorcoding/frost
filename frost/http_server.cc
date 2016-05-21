#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <stddef.h>
#include "http_server.h"

#define MAX_BUF_SIZE 16384

#define SELF(w, type, f) (type*) ( (char *) &(w) - offsetof(type, f))

namespace frost {

    http_server::http_server(ev::loop_ref loop, const char* host, const char* port)
            : ev::srv(loop, host, port) {
        set_on_conn<http_server, &http_server::conn_create_cb, &http_server::conn_destroy_cb>(this);
    }

    http_server::http_server(ev::loop_ref loop, const std::string& host, const std::string& port)
            : http_server(loop, host.c_str(), port.c_str())
    { }

    http_server::http_server(const char* host, const char* port)
            : http_server(ev::get_default_loop(), host, port)
    { }

    http_server::http_server(const std::string& host, const std::string& port)
            : http_server(ev::get_default_loop(), host.c_str(), port.c_str())
    { }

    http_server::http_server(uint16_t port)
            : http_server("0.0.0.0", std::to_string(port))
    { }

    ev::srv_conn* http_server::conn_create_cb(ev::srv& srv, evsrv_conn_info* info) {
        http_request* c = new http_request((http_server&) srv, info);

        char* buf = new char[MAX_BUF_SIZE];
        c->set_rbuf(buf, MAX_BUF_SIZE);
        c->set_on_read<http_server, &http_server::read_cb>(this);
        c->set_write_now(true);

        return c;
    }

    void http_server::conn_destroy_cb(ev::srv_conn& conn, int err) {
        http_request& c = (http_request&) conn;
        delete[] c.rbuf();
        c.set_rbuf(NULL, 0);
        delete &c;
    }

    http_server::~http_server() {
        this->stop();
        for (auto it = _signals.begin(); it != _signals.end(); ++it) {
            delete it->second;
            it->second = nullptr;
        }
    }

    int http_server::listen() {
        add_on_exit_watchers();
        return ev::srv::listen();
    }

    int http_server::accept() {
        auto res = ev::srv::accept();
        start_signal_watchers();
        return res;
    }

    void http_server::stop() {
        stop_signal_watchers();
        ev::srv::stop();
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

    void http_server::read_cb(ev::srv_conn& conn, ssize_t nread) {
        if (!nread) {
            return;
        }
        auto& req = (http_request&) conn;

        auto p = req.parse();
        // auto p = parse_result::GOOD;
        switch (p) {
            case parse_result::NEED_MORE: {
                if (req.ruse() >= req.rlen()) {
                    auto resp = create_response(req);
                    resp->write(status_code::REQUEST_ENTITY_TOO_LARGE, "", 0);
                    resp->finish();
                } else {
                    req.read_timer_again();
                }
                return;
            }
            case parse_result::GOOD: {
                auto resp = create_response(req);
                // auto cb = _router.get_route(req->path());
                auto cb = _router.get_route("/");
                if (cb == nullptr) {
                    char* buf = new char[1024];
                    int len = snprintf(buf, 1024, "Path \'%.*s\' not found on the server",
                                       (int) req.path_len(), req.path());
                    if (len >= 0) {
                        resp->write(status_code::NOT_FOUND, buf, static_cast<size_t>(len));
                    } else {
                        perror("[http_server::read_cb] snprintf failed");
                    }
                    resp->finish();
                    delete[] buf;
                } else {
                    // TODO: check if method is allowed
                    (*cb)(req, *resp);
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
