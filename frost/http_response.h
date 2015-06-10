#ifndef FROST_HTTP_RESPONSE_H
#define FROST_HTTP_RESPONSE_H

#include <sys/uio.h>
#include <ev++.h>
#include <functional>

namespace frost {
    class http_request;
    class http_response {
        friend class http_server;
    public:
        http_response(int& client_fd, http_request* req);
        ~http_response();

    private:
        void start();
        void stop();

    private:
        ev::io _ww;
        ev::timer _tw;
        int& _client_fd;

        http_request* _req;

        struct iovec _wbuf;
        uint32_t _wuse;
        uint32_t _wlen;
    };
}

#endif //FROST_HTTP_RESPONSE_H
