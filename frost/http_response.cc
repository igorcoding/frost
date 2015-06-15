#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>
#include "http_response.h"
#include "globals.h"

namespace frost {

    http_version http_response::DEFAULT_VERSION = {1, 1};

    http_response::http_response(int& client_fd, http_request* req)
        : _ww(),
          _client_fd(client_fd),

          _req(req),
          _wbuf(nullptr),
          _wuse(0),
          _wlen(0),
          _finished(false) {
        _ww.set(_client_fd, ev::WRITE);
        _tw.set(5.0, 0);
    }

    http_response::~http_response() {
        stop();

        for (size_t i = 0; i < _wuse; ++i) {
            free(_wbuf[i].iov_base);
        }
        free(_wbuf);
    }

    void http_response::write_raw(const char* buf, size_t len) {
        if (len == 0) {
            len = strlen(buf);
        }

        if (_wuse != 0) {
            if (_wuse == _wlen) {
                _wlen += 2; // TODO: maybe _wlen *= 2
                _wbuf = (iovec*) realloc(_wbuf, sizeof(iovec) * (_wlen));
            }
            _wbuf[_wuse].iov_base = memdup(buf, len);
            _wbuf[_wuse].iov_len = len;
            _wuse++;
            return;
        }

        ssize_t written = 0;

//        written = ::write_raw(_client_fd, buf, len, 0);
//        if (written == len) {
//            return;
//        } else if (written < 0) {
//            perror("write_raw error");
//
//            switch (errno) {
//                case EAGAIN:
//                    written = 0;
//                    break;
//                case EINTR:
//                    break;
//                default:
//                    // TODO
//                    return;
//            }
//        } else {
//        }

        _wlen = 2;
        _wbuf = (iovec*) calloc(_wlen, sizeof(iovec));
        _wbuf[0].iov_base = memdup(buf + written, len - written);
        _wbuf[0].iov_len = len - written;
        _wuse = 1;

        _tw.again();
        _ww.start();
    }

    void http_response::write_status(status_code code) {
        write_status(code, DEFAULT_VERSION);
    }

    void http_response::write_status(status_code code, http_version& version) {
        auto& code_str = status_code_assist::desc(code);
        auto& ver_str = version.to_string();
        size_t buf_len = 100;
        char* buf = (char*) calloc(sizeof(char), buf_len);

        again:
            int len = snprintf(buf, buf_len, "%s %d %.*s\r\n", ver_str.c_str(), (int) code, (int) code_str.length(),
                               code_str.c_str());
            if (len < 0) {
                perror("[http_response::write_status]: snprintf failed");
            } else {
                auto len_s = (size_t) len;
                if (len_s > buf_len) {
                    buf_len = len_s;
                    buf = (char*) realloc(buf, buf_len);
                    goto again;
                }
                write_raw(buf, static_cast<size_t>(std::min(len, (int) buf_len)));
            }
        free(buf);
    }

    void http_response::write(status_code code, const char* body, size_t body_len) {
        if (body_len == 0) body_len = strlen(body);
        write_status(code);

        char* buf = new char[HEADER_BUF_LEN];
        write_header(buf, HEADER_BUF_LEN, "Server", VERSION_STR);
        write_header(buf, HEADER_BUF_LEN, "Content-Length", body_len);
        write_header(buf, HEADER_BUF_LEN, "Content-Type", "text/html");
        delete[] buf;
        write_raw("\r\n", 2);
        write_raw(body, body_len);
    }

    void http_response::start() {
        _ww.start();
        _tw.start();
    }

    void http_response::stop() {
        if (_tw.is_active()) {
            _tw.stop();
        }
        if (_ww.is_active()) {
            _ww.stop();
        }
//        if (_client_fd > -1) {
//            printf("closing socket in resp\n");
//            ::close(_client_fd);
//            _client_fd = -1;
//        }
    }
}
