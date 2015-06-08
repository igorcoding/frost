#ifndef FROST_SERVER_H
#define FROST_SERVER_H

#include "http_request.h"
#include "http_response.h"

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
        typedef std::function<void(http_request*, http_response* resp)> cb_t;
        typedef std::function<void()> signal_cb_t;
    public:
        static constexpr int BACKLOG_SIZE = 5;

        http_server(uint16_t port);
        ~http_server();

        uint16_t get_port() const;
        state get_state() const;
        uint32_t get_active_connections() const;

        void on(const char* path, const cb_t& cb);
        void on(const std::string& path, const cb_t& cb);
        void on_signal(int sig, const signal_cb_t& cb);

        int start();
        void stop();

    private:
        int start_listen();
        void signal_cb(ev::sig& w, int revents);
        void accept_cb(ev::io& w, int revents);

        void start_signal_watchers();
        void stop_signal_watchers();

        void add_on_exit_watchers();
        bool has_signal_watcher(int sig);

    private:
        state _state;
        uint16_t _port;
        std::unordered_map<std::string, cb_t> _paths;
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

    inline void http_server::on(const char* path, const http_server::cb_t& cb) {
        _paths[path] = cb;
    }

    inline void http_server::on(const std::string& path, const http_server::cb_t& cb) {
        _paths[path] = cb;
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
}

#endif // FROST_SERVER_H
