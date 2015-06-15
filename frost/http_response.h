#ifndef FROST_HTTP_RESPONSE_H
#define FROST_HTTP_RESPONSE_H

#include <sys/uio.h>
#include <ev++.h>
#include <functional>
#include <unordered_map>
#include <string.h>

#include "util/util.h"
#include "http/status.h"
#include "http/header.h"
#include "http/http_version.h"

namespace frost {




    class http_request;
    class http_response {
        friend class http_server;
    public:
        static http_version DEFAULT_VERSION;

        http_response(int& client_fd, http_request* req);
        ~http_response();

        bool finished();

        void write_raw(const char* buf, size_t len);
        void write_status(status_code code);
        void write_status(status_code code, http_version& version);
        template <typename VALUE> bool write_header(const char* name, VALUE value);

        void write(status_code code, const char* body, size_t body_len);

        void finish();

    private:
        void start();
        template <typename VALUE> bool write_header(char* buf, size_t buf_len, const char* name, VALUE value);

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

        static constexpr size_t HEADER_BUF_LEN = 50;
    };


    inline bool http_response::finished() {
        return _finished;
    }

    template <typename VALUE> inline
    bool http_response::write_header(const char* name, VALUE value) {
        char* buf = new char[HEADER_BUF_LEN];
        bool status = write_header(buf, HEADER_BUF_LEN, name, value);
        delete[] buf;
        return status;
    }

    template <typename VALUE> inline
    bool http_response::write_header(char* buf, size_t buf_len, const char* name, VALUE value) {
        int len = header::build_header_str(buf, buf_len, name, value);
        if (len < 0) {
            perror("Error in snprintf");
            return false;
        } else {
            write_raw(buf, static_cast<size_t>(len));
            return true;
        }
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
