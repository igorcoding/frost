#ifndef FROST_HTTP_RESPONSE_H
#define FROST_HTTP_RESPONSE_H

#include <sys/uio.h>
#include <ev++.h>
#include <functional>
#include <unordered_map>
#include <string.h>
#include <util.h>

#include "http_request.h"
#include "util/util.h"
#include "http/status.h"
#include "http/header.h"
#include "http/http_version.h"


namespace frost {

    struct buffer {
        char* buf;
        size_t len;
        size_t use;


        buffer(size_t buf_len)
            : buf((char*) malloc(sizeof(char) * buf_len)),
              len(buf_len),
              use(0)
        { }

        ~buffer() {
            free(buf);
        }

        char* current() const {
            return buf + use;
        }

        ssize_t left() const {
            return len - use;
        }

        void grow() {
            len <<= 1;
            buf = (char*) realloc(buf, len);
        }

        void add_new_line() {
            if (unlikely(left() < 2)) {
                grow();
            }
            buf[use++] = '\r';
            buf[use++] = '\n';
        }
    };

    class http_response {
        friend class http_server;
    public:
        static http_version DEFAULT_VERSION;

        ~http_response();

        bool finished();

        void write_raw(const void* buf, size_t len);

        void write(status_code code, const void* body, size_t body_len);

        void finish();

    private:
        http_response(http_request& req);
        int write_status(buffer& b, status_code code);
        template <typename VALUE> int write_header(buffer& b, const char* name, VALUE value);
    private:
        http_request& _req;
        bool _finished;
    };


    inline bool http_response::finished() {
        return _finished;
    }

    template <typename VALUE> inline
    int http_response::write_header(buffer& b, const char* name, VALUE value) {
        again:
        int len = header::build_header_str(b.current(), b.len, name, value);
        if (len < 0) {
            perror("Error in snprintf");
            return len;
        } else {
            if (len > b.left()) {
                b.grow();
                goto again;
            }
            b.use += len;
            return len;
        }
    }

    inline void http_response::finish() {
        _finished = true;
        _req.ruse() = 0;
    }
}

#endif //FROST_HTTP_RESPONSE_H
