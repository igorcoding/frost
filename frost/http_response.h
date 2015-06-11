#ifndef FROST_HTTP_RESPONSE_H
#define FROST_HTTP_RESPONSE_H

#include <sys/uio.h>
#include <ev++.h>
#include <functional>
#include <unordered_map>

#include "util/util.h"
#include "http/status.h"

namespace frost {




    class http_request;
    class http_response {
        friend class http_server;
    public:
        static http_version DEFAULT_VERSION;

        http_response(int& client_fd, http_request* req);
        ~http_response();

        bool finished();

        void send(const char* buf, size_t len);
        void send_status(status_code code);
        void send_status(status_code code, const http_version& version);
        void finish();

        void send(status_code code, const char* body, size_t body_len);
    private:
        void start();

        void stop();
    private:
        ev::io _ww;
        ev::timer _tw;

        int& _client_fd;

        http_request* _req;
        iovec* _wbuf;
        uint32_t _wuse;
        uint32_t _wlen;

        bool _finished;
    };


    inline bool http_response::finished() {
        return _finished;
    }

    inline void http_response::finish() {
        _finished = true;
//        if (!_ww.is_active()) {
//            auto req = _req;
//            delete this;
//            delete req;
//        }
    }
}

#endif //FROST_HTTP_RESPONSE_H
