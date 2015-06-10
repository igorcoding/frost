#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>
#include "http_response.h"

namespace frost {

    http_response::http_response(ev::default_loop& loop, int& client_fd, http_request* req)
        : _loop(loop),
          _ww(),
          _client_fd(client_fd),

          _req(req),

          _cb(),
          _has_cb(false) {
    }

    http_response::~http_response() {
        stop();
    }

    void http_response::write_cb(ev::io& w, int revents) {
        if (ev::ERROR & revents) {
            perror("got invalid event");
            return;
        }
        w.stop();

        char msg[] = "HTTP/1.1 200 OK\r\nContent-Length: 18\r\n\r\nHello from server!";
        size_t s = strlen(msg);

        ssize_t written = ::send(w.fd, msg, s, 0);
//        std::cout << s << " " << written << std::endl;
        if (written < 0) {
            perror("write error");
            return;
        }

        if (_has_cb) {
            _cb(_req, this);
        }

//        std::cout << "Ended write_cb" << std::endl;
//        stop();
    }

    void http_response::start() {
        _ww.set<http_response, &http_response::write_cb>(this);
        _ww.set(_client_fd, ev::WRITE);
        _ww.start();
    }

    void http_response::set_cb(http_response::resp_cb_t&& cb) {
        _cb = std::move(cb);
        _has_cb = true;
    }

    void http_response::stop() {
        if (_client_fd > -1) {
            if (_ww.is_active()) {
                _ww.stop();
            }
            ::close(_client_fd);
            _client_fd = -1;
        }
    }
}
