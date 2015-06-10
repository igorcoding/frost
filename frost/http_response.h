#ifndef FROST_HTTP_RESPONSE_H
#define FROST_HTTP_RESPONSE_H

#include <ev++.h>
#include <functional>

namespace frost {
    class http_request;
    class http_response {
        friend class http_server;
        typedef std::function<void(http_request*, http_response*)> resp_cb_t;
    public:
        http_response(ev::default_loop& loop, int& client_fd, http_request* req);
        ~http_response();

        void start();
    private:
        void write_cb(ev::io& w, int revents);
        void set_cb(resp_cb_t&& cb);
        void stop();

    private:
        ev::default_loop& _loop;
        ev::io _ww;
        int& _client_fd;

        http_request* _req;

        resp_cb_t _cb;
        bool _has_cb;
    };
}

#endif //FROST_HTTP_RESPONSE_H
