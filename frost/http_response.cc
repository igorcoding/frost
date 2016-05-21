#include "http_response.h"

#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>
#include <util.h>
#include "http_request.h"
#include "globals.h"

#define RESP_BASE_SIZE 1024


namespace frost {

    http_version http_response::DEFAULT_VERSION = {1, 1};

    http_response::http_response(http_request& req)
        : _req(req),
          _finished(false) {
    }

    http_response::~http_response() {
    }

    void http_response::write_raw(const void* buf, size_t len) {
        this->_req.write(buf, len);
    }

    void http_response::write(status_code code, const void* body, size_t body_len) {
        if (body_len == 0) body_len = strlen((char*) body);

        buffer* b = new buffer(RESP_BASE_SIZE);

        write_status(*b, code);

        write_header(*b, "Server", VERSION_STR);
        write_header(*b, "Content-Length", body_len);
        write_header(*b, "Content-Type", "text/html");

        b->add_new_line();

        write_raw(b->buf, b->use);
        write_raw(body, body_len);

        delete b;
    }

    int http_response::write_status(buffer& b, status_code code) {
        auto& code_str = status_code_assist::desc(code);
        auto& ver_str = DEFAULT_VERSION.to_string();

        again:
            int len = snprintf(b.current(), b.len, "%s %d %.*s\r\n", ver_str.c_str(),
                               (int) code, (int) code_str.length(), code_str.c_str());
            if (len < 0) {
                cerror("[http_response::write_status]: snprintf failed");
            } else {
                if (len > b.left()) {
                    b.grow();
                    goto again;
                }
                b.use += len;
            }
        return len;
    }


}
