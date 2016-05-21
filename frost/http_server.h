#ifndef FROST_SERVER_H
#define FROST_SERVER_H

#include "http_request.h"
#include "http_response.h"
#include "router.h"

#include <evsrv++.h>
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

    class http_server : public ev::srv {
        typedef std::function<void()> signal_cb_t;
    public:
        static constexpr int BACKLOG_SIZE = 100;

        http_server(ev::loop_ref loop, const char* host, const char* port);
        http_server(ev::loop_ref loop, const std::string& host, const std::string& port);

        http_server(const char* host, const char* port);
        http_server(const std::string& host, const std::string& port);
        http_server(uint16_t port);

        virtual ~http_server();

        void on(const std::string& path, const cb_t& cb);
        void on_signal(int sig, const signal_cb_t& cb);

        virtual int listen() override;
        virtual int accept() override;
        virtual void stop() override;

    private:
        void signal_cb(ev::sig& w, int revents);
        void read_cb(ev::srv_conn& conn, ssize_t nread);
        ev::srv_conn* conn_create_cb(ev::srv& srv, evsrv_conn_info* info);
        void conn_destroy_cb(ev::srv_conn& conn, int err);

        http_response* create_response(http_request& req);

        void start_signal_watchers();
        void stop_signal_watchers();

        void add_on_exit_watchers();
        bool has_signal_watcher(int sig);

    private:
        std::string _host;
        std::string _port;
        path_router_t _router;
        std::unordered_map<int, signal_cb_t> _signals_cb;

        std::unordered_map<int, ev::sig*> _signals;
    };


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

    inline http_response* http_server::create_response(http_request& req) {
        http_response* resp = new http_response(req);
        req.set_http_response(resp);
        return resp;
    }
}

#endif // FROST_SERVER_H
