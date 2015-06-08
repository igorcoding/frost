#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include "http_request.h"

namespace frost {

    http_request::http_request(ev::default_loop& loop, int client_fd)
        : _loop(loop),
          _rw(),
          _client_fd(client_fd),
          _rbuf(new char[BUF_SIZE]),
          _ruse(0),
          _rbuf_ptr(_rbuf),

          _parse_result(parse_result::NEED_MORE),
          _method(http_method::NONE),
          _version({0, 0}),
          _headers(),
          _body_ptr(nullptr),
          _body_size(0) {
        ::fcntl(_client_fd, F_SETFL, fcntl(_client_fd, F_GETFL, 0) | O_NONBLOCK);

        _rw.set<http_request, &http_request::read_cb>(this);
        _rw.set(_client_fd, ev::READ);
    }

    http_request::~http_request() {
        delete[] _rbuf;
        _rbuf = nullptr;
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
                case parse_result::GOOD:
                    // TODO: prepare http_response and call appropriate cb or 404 if no cb specified
                    break;
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
            perror("EOF");
            stop();
            delete this;
        }
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
        return parse_result::NEED_MORE;
    }
}