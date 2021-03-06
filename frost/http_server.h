#ifndef FROST_SERVER_H
#define FROST_SERVER_H

#include "http_request.h"
#include "http_response.h"
#include "router.h"

#include <stdint.h>
#include <functional>
#include <unordered_map>
#include <ev++.h>

namespace frost {
    enum class state {
        IDLE,
        STARTED,
        STOPPED
    };

    class http_server {
        typedef std::function<void()> signal_cb_t;
    public:
        static constexpr int BACKLOG_SIZE = 100;

        http_server(uint16_t port);
        ~http_server();

        uint16_t get_port() const;
        state get_state() const;
        uint32_t get_active_connections() const;

        void on(const std::string& path, const cb_t& cb);
        void on_signal(int sig, const signal_cb_t& cb);

        int start();
        int listen();
        void accept();
        void stop();

        void notify_fork_child();

    private:
        int start_listen();
        void signal_cb(ev::sig& w, int revents);
        void accept_cb(ev::io& w, int revents);
        void read_cb(ev::io& w, int revents);
        void write_cb(ev::io& w, int revents);

        void read_timeout_cb(ev::timer& w, int revents);
        void keep_alive_cb(ev::timer& w, int revents);
        void write_timeout_cb(ev::timer& w, int revents);

        http_request* create_request(int client_fd);
        http_response* create_response(http_request* req);

        void start_signal_watchers();
        void stop_signal_watchers();

        void add_on_exit_watchers();
        bool has_signal_watcher(int sig);

    private:
        state _state;
        uint16_t _port;
        path_router_t _router;
        std::unordered_map<int, signal_cb_t> _signals_cb;

        std::unordered_map<int, ev::sig*> _signals;
        int _listenfd;
        ev::default_loop _loop;
        ev::io _accept_w;

        uint32_t _active_connections;
    };


    inline uint16_t http_server::get_port() const {
        return _port;
    }

    inline state http_server::get_state() const {
        return _state;
    }

    inline uint32_t http_server::get_active_connections() const {
        return _active_connections;
    }

    inline void http_server::on(const std::string& path, const cb_t& cb) {
        _router.add_route(path, cb);
    }

    inline void http_server::on_signal(int sig, const signal_cb_t& cb) {
        _signals_cb[sig] = cb;
        ev::sig* w = new ev::sig;
        w->set(sig);
        w->set<http_server, &http_server::signal_cb>(this);

        auto it = _signals.find(sig);
        if (it != _signals.end()) {
            delete it->second;
        }
        _signals[sig] = w;
    }

    inline bool http_server::has_signal_watcher(int sig) {
        auto it = _signals.find(sig);
        return it != _signals.end();
    }

    inline http_request* http_server::create_request(int client_fd) {
        http_request* req = new http_request(client_fd);
        req->_rw.set<http_server, &http_server::read_cb>(this);
        req->_tw.set<http_server, &http_server::read_timeout_cb>(this);
        req->start();
        return req;
    }

    inline http_response* http_server::create_response(http_request* req) {
        http_response* resp = new http_response(req->_client_fd, req);
        resp->_ww.set<http_server, &http_server::write_cb>(this);
        resp->_tw.set<http_server, &http_server::write_timeout_cb>(this);
        req->set_http_response(resp);
        return resp;
    }
}

#endif // FROST_SERVER_H
