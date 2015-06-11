#include "http_request.h"
#include "http_response.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

namespace frost {

    uint32_t http_request::_rlen = 4096;

    void http_request::set_max_buffer_size(uint32_t max_buffer) {
        _rlen = max_buffer;
    }



    http_request::http_request(int client_fd)
        : _rw(),
          _tw(),
          _client_fd(client_fd),
          _rbuf(nullptr),
          _ruse(0),
          _rbuf_ptr(_rbuf),

          _parse_result(parse_result::NEED_MORE),
          _method(http_method::NONE),
          _version({0, 0}),
          _headers(),
          _body_ptr(nullptr),
          _body_size(0) {
        _rbuf = new char[_rlen];

        ::fcntl(_client_fd, F_SETFL, fcntl(_client_fd, F_GETFL, 0) | O_NONBLOCK);
        _rw.set(_client_fd, ev::READ);
        _tw.set(0.01, 5.0);
    }

    http_request::~http_request() {
        delete[] _rbuf;
        _rbuf = nullptr;

//        delete _cb;
//        _cb = nullptr;
        stop();
    }

    void http_request::start() {
        _rw.start();
        _tw.start();
    }

    void http_request::stop() {
        if (_tw.is_active()) {
            _tw.stop();
        }
        if (_rw.is_active()) {
            _rw.stop();
        }
        if (_client_fd > -1) {
            ::close(_client_fd);
            _client_fd = -1;
        }
    }

    parse_result http_request::parse() {
        return parse_result::GOOD;
    }
}