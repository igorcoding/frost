#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>
#include "http_response.h"

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
        _tw.set(0.01, 5.0);
    }

    http_response::~http_response() {
        stop();

        for (size_t i = 0; i < _wuse; ++i) {
            free(_wbuf[i].iov_base);
        }
        free(_wbuf);
    }

    void http_response::send(const char* buf, size_t len) {
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

//        written = ::send(_client_fd, buf, len, 0);
//        if (written == len) {
//            return;
//        } else if (written < 0) {
//            perror("write error");
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

    void http_response::send_status(status_code code) {
        send_status(code, DEFAULT_VERSION);
    }

    void http_response::send_status(status_code code, const http_version& version) {
        auto& code_str = status_code_assist::desc(code);
        auto& ver_str = DEFAULT_VERSION.to_string();
        size_t len = 8 + 1 + 3 + 1 + code_str.length() + 2;  // HTTP/1.1 200 OK\r\n
        char* buf = new char[len + 1];
        stpncpy(buf, ver_str.c_str(), ver_str.length());
        snprintf(buf + ver_str.length() - 1, len - ver_str.length() + 1, " %d %.*s\r\n", (int) code, (int) code_str.length(), code_str.c_str());
        send(buf, len);
        delete[] buf;
    }

    void http_response::send(status_code code, const char* body, size_t body_len) {
        send_status(code);
        send("\r\n", 2);
        send(body, body_len);
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
        if (_client_fd > -1) {
            ::close(_client_fd);
            _client_fd = -1;
        }
    }
}
