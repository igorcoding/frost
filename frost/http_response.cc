#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>
#include "http_response.h"

namespace frost {

    http_response::http_response(int& client_fd, http_request* req)
        : _ww(),
          _client_fd(client_fd),

          _req(req) {
        _ww.set(_client_fd, ev::WRITE);
        _tw.set(0.01, 5.0);
    }

    http_response::~http_response() {
        stop();
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
