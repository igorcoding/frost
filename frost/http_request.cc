#include "http_request.h"
#include "http_response.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

namespace frost {

    http_request::http_request(ev::default_loop& loop, int client_fd, const path_router_t& router)
        : _loop(loop),
          _rw(),
          _client_fd(client_fd),
          _rbuf(new char[BUF_SIZE]),
          _ruse(0),
          _rbuf_ptr(_rbuf),

          _cb(),
          _has_cb(false),

          _parse_result(parse_result::NEED_MORE),
          _method(http_method::NONE),
          _version({0, 0}),
          _headers(),
          _body_ptr(nullptr),
          _body_size(0),

          _router(router) {
        ::fcntl(_client_fd, F_SETFL, fcntl(_client_fd, F_GETFL, 0) | O_NONBLOCK);
    }

    http_request::~http_request() {
        delete[] _rbuf;
        _rbuf = nullptr;

//        delete _cb;
//        _cb = nullptr;
        stop();
    }

    void http_request::read_cb(ev::io& w, int revents) {
        if (ev::ERROR & revents) {
            perror("got invalid event");
            return;
        }

        ssize_t nread = ::recv(w.fd, _rbuf + _ruse, BUF_SIZE - _ruse, 0);
        if (nread > 0) {
            _ruse += nread;
            if (_ruse >= BUF_SIZE) {
                // TODO: somehow react to buffer exceeding
                // errno = ENOBUFS;
                // perror("buffer exceeded");
            }

            auto p = parse();
            switch (p) {
                case parse_result::NEED_MORE:
                    break;
                case parse_result::GOOD: {
                    _rw.stop();
                    if (_has_cb) {
                        _cb(this);
                    }
                    break;
                }
                case parse_result::BAD:
                    // TODO: respond in bad request
                    break;
            }

        } else if (nread < 0) {
            perror("read error");

            switch (errno) {
                case EAGAIN:
                    break;
                case EINTR:
                    break;
                default:
                    break;
            }

            return;
        } else {
//            perror("EOF");
            stop();
            delete this;
        }
    }

    void http_request::start() {
        _rw.set<http_request, &http_request::read_cb>(this);
        _rw.set(_client_fd, ev::READ);
        _rw.start();
    }

    void http_request::set_cb(req_cb_t&& cb) {
//        delete _cb;
        _cb = std::move(cb);
        _has_cb = true;
    }

    void http_request::stop() {
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