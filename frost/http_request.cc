#include "http_request.h"
#include "http_response.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

namespace frost {

    uint32_t http_request::_rlen = 4096;
    size_t http_request::_KEEP_ALIVE_COUNT = 0;

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
          _body_size(0),

          _keep_alive(false),
          __resp(nullptr) {
        _rbuf = new char[_rlen];

        ::fcntl(_client_fd, F_SETFL, ::fcntl(_client_fd, F_GETFL, 0) | O_NONBLOCK);
        _rw.set(_client_fd, ev::READ);
        _tw.set(5.0, 0);
    }

    http_request::~http_request() {
        if (_keep_alive) {
            --_KEEP_ALIVE_COUNT;
        }
        delete[] _rbuf;
        _rbuf = nullptr;
        stop();
    }

    void http_request::start() {
        _rw.start();
        _tw.start();
    }

    void http_request::start_keep_alive() {
        _rw.start();
        _keep_alive_w.set(10.0, 0);
        _keep_alive_w.start();
    }

    void http_request::stop() {
        if (_tw.is_active()) {
            _tw.stop();
        }
        if (_keep_alive_w.is_active()) {
            _keep_alive_w.stop();
        }
        if (_rw.is_active()) {
            _rw.stop();
        }
        if (_client_fd > -1) {
//            printf("closing socket\n");
            ::close(_client_fd);
            _client_fd = -1;
        }
    }

    void http_request::clear() {
        _parse_result = parse_result::NEED_MORE;
        _method = http_method::NONE;
        _version.clear();
        _path.clear();
        _headers.clear();
        _body_ptr = nullptr;
        _body_size = 0;

        delete[] _rbuf;
        _rbuf = new char[_rlen];
        _rbuf_ptr = _rbuf;
        _ruse = 0;

        __resp = nullptr;
    }

    void http_request::set_http_response(http_response* resp) {
        __resp = resp;
    }

    http_response* http_request::get_http_response() {
        return __resp;
    }

    parse_result http_request::parse() {
        // TODO: if this requests a keep-alive connection check for _KEEP_ALIVE_COUNT < _KEEP_ALIVE_MAX
        return parse_result::GOOD;
    }
}