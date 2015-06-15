#include "http_request.h"
#include "http_response.h"
#include "util/util.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

#define RECREATE(buf, buf_len, buf_len_value, buf_use) do {\
    free(buf);\
    buf_len = buf_len_value;\
    buf = (char*) calloc(sizeof(char), buf_len_value + 1);\
    bzero(buf, buf_len_value + 1);\
    buf_use = 0;\
} while(0);

#define CLEAN(buf, buf_len, buf_use) do {\
    free(buf);\
    buf = nullptr;\
    buf_len = 0;\
    buf_use = 0;\
} while(0);


namespace frost {

    uint32_t http_request::_rlen = 4096;
    size_t http_request::_KEEP_ALIVE_COUNT = 0;
    size_t http_request::path_max_size = 100;

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
          _parse_state(parse_state::BEGIN),
          _method(http_method::NONE),
          _version({0, 0}),
          _path(nullptr),
          _path_len(0),
          _headers(),
          _body_ptr(nullptr),
          _body_size(0),

          _keep_alive(false),
          __resp(nullptr),

          _work_buf_use(0),
          _content_length_coming(false) {
        _rbuf = new char[_rlen];
        _rbuf_ptr = _rbuf;
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
        _path = nullptr;
        _path_len = 0;
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

        while (true) {
            if (_rbuf_ptr == (_rbuf + _ruse) && _parse_state != parse_state::BODY) {
                return parse_result::NEED_MORE;
            }

            switch (_parse_state) {

                case parse_state::BEGIN: {
                    if (_ruse == 0) {
                        return parse_result::NEED_MORE;
                    } else {
                        _work_buf_use = 0;
                        _parse_state = parse_state::METHOD;
                        continue;
                    }

                    break;
                }
                case parse_state::METHOD: {
                    auto ch = *_rbuf_ptr;
                    if (ch == ' ' && _work_buf_use == 0) {
                        return parse_result::BAD;
                    }
                    if (ch != ' ') {
                        _work_buf_use++;
                    } else {
                        #ifdef ALLOW_PRINT
                        printf("method: %.*s\n", (int) _work_buf_use, _work_buf);
                        #endif
//                        _method = http_method_assist::from_str(std::string(_work_buf, _work_buf_use));
                        _work_buf_use = 0;
                        _parse_state = parse_state::PATH;
                    }
                    ++_rbuf_ptr;
                    continue;

                    break;
                }
                case parse_state::PATH: {
                    auto ch = *_rbuf_ptr;
                    if (ch == ' ' && _work_buf_use == 0) {
                        return parse_result::BAD;
                    }
                    // TODO: discard special characters maybe?
                    if (ch != ' ') {
                        _work_buf_use++;
                    } else {
                        #ifdef ALLOW_PRINT
                        printf("path: %.*s\n", (int) _work_buf_use, _work_buf);
                        #endif
                        _path = _rbuf_ptr - _work_buf_use;
                        _path_len = _work_buf_use;
                        _work_buf_use = 0;
                        _parse_state = parse_state::PROTOCOL;
                    }
                    ++_rbuf_ptr;
                    continue;
                    break;
                }
                case parse_state::PROTOCOL: {
                    auto ch = *_rbuf_ptr;
                    if (ch != 'H') {
                        return parse_result::BAD;
                    } else {
                        _parse_state = parse_state::PROTOCOL_H;
                    }
                    ++_rbuf_ptr;
                    continue;
                    break;
                }
                case parse_state::PROTOCOL_H: {
                    auto ch = *_rbuf_ptr;
                    if (ch != 'T') {
                        return parse_result::BAD;
                    } else {
                        _parse_state = parse_state::PROTOCOL_T1;
                    }
                    ++_rbuf_ptr;
                    continue;
                    break;
                }
                case parse_state::PROTOCOL_T1: {
                    auto ch = *_rbuf_ptr;
                    if (ch != 'T') {
                        return parse_result::BAD;
                    } else {
                        _parse_state = parse_state::PROTOCOL_T2;
                    }
                    ++_rbuf_ptr;
                    continue;
                    break;
                }
                case parse_state::PROTOCOL_T2: {
                    auto ch = *_rbuf_ptr;
                    if (ch != 'P') {
                        return parse_result::BAD;
                    } else {
                        _parse_state = parse_state::PROTOCOL_P;
                    }
                    ++_rbuf_ptr;
                    continue;
                    break;
                }
                case parse_state::PROTOCOL_P: {
                    auto ch = *_rbuf_ptr;
                    if (ch != '/') {
                        return parse_result::BAD;
                    } else {
                        _work_buf_use = 0;
                        _parse_state = parse_state::PROTOCOL_MAJOR;
                    }
                    ++_rbuf_ptr;
                    continue;
                    break;
                }
                case parse_state::PROTOCOL_MAJOR: {
                    auto ch = *_rbuf_ptr;
                    if (IS_DIGIT(ch)) {
                        _work_buf_use++;
                    } else {
                        if (ch == '.') {
                            _version.set_major_ver(frost::atoi_positive(_rbuf_ptr - _work_buf_use, _work_buf_use));
                            _work_buf_use = 0;
                            _parse_state = parse_state::PROTOCOL_MINOR;
                        } else {
                            return parse_result::BAD;
                        }
                    }
                    ++_rbuf_ptr;
                    continue;
                    break;
                }
                case parse_state::PROTOCOL_MINOR: {
                    auto ch = *_rbuf_ptr;
                    if (IS_DIGIT(ch)) {
                        _work_buf_use++;
                    } else {
                        if (ch == '\r') {
                            _version.set_minor_ver(frost::atoi_positive(_rbuf_ptr - _work_buf_use, _work_buf_use));
                            #ifdef ALLOW_PRINT
                            printf("version: %s\n", _version.to_string().c_str());
                            #endif
                            _work_buf_use = 0;
                            _parse_state = parse_state::STATUS_LINE_BREAK_R;
                        } else {
                            return parse_result::BAD;
                        }
                    }
                    ++_rbuf_ptr;
                    continue;
                    break;
                }
                case parse_state::STATUS_LINE_BREAK_R: {
                    auto ch = *_rbuf_ptr;
                    if (ch != '\n') {
                        return parse_result::BAD;
                    } else {
                        _parse_state = parse_state::STATUS_LINE_BREAK_N;
                    }
                    ++_rbuf_ptr;
                    continue;
                    break;
                }
                case parse_state::STATUS_LINE_BREAK_N: { // TODO: rethink about ++_rbuf_ptr;
                    auto ch = *_rbuf_ptr;
                    if (ch == ':') {
                        ++_rbuf_ptr;
                        return parse_result::BAD;
                    } else if (ch == '\r') {
                        _parse_state = parse_state::PRE_BODY;
                        ++_rbuf_ptr;
                    } else {  // header starts
                        _work_buf_use = 0;
                        _parse_state = parse_state::HEADER_NAME;
                    }
                    continue;
                    break;
                }
                case parse_state::HEADER_NAME: {
                    auto ch = *_rbuf_ptr;
                    if (ch == '\r' || ch == '\n') {
                        return parse_result::BAD;
                    } else if (ch != ':') {
                        _work_buf_use++;
                    } else {
                        _current_header_name = _rbuf_ptr - _work_buf_use;
                        _current_header_len = _work_buf_use;
                        if (strncasecmp(_current_header_name, "Content-Length", _current_header_len) == 0) {
                            _content_length_coming = true;
                        }
                        #ifdef ALLOW_PRINT
                        printf("header_name: %.*s\n", (int) _work_buf_use, _work_buf);
                        #endif
                        _work_buf_use = 0;
                        _parse_state = parse_state::HEADER_VALUE;
                    }
                    ++_rbuf_ptr;
                    continue;
                    break;
                }
                case parse_state::HEADER_VALUE: {
                    auto ch = *_rbuf_ptr;
                    if (ch == ' ') {

                    } else if (ch == '\n') {
                        return parse_result::BAD;
                    } else if (ch == '\r') {
                        auto current_header_value = _rbuf_ptr - _work_buf_use;
                        auto current_header_value_len = _work_buf_use;
                        if (_content_length_coming) {
                            _content_length_coming = false;
                            int body_size = frost::atoi_positive(current_header_value, current_header_value_len);
                            if (body_size > 0) {
                                _body_size = static_cast<uint32_t>(body_size);
                            }
                        }
                        #ifdef ALLOW_PRINT
                        printf("header_value: %.*s\n", (int) _work_buf_use, _work_buf);
                        #endif
                        _headers.emplace_back(header(_current_header_name, _current_header_len,
                                                     current_header_value, current_header_value_len));
                        _work_buf_use = 0;
                        _parse_state = parse_state::HEADER_BREAK_R;
                    } else {
                        _work_buf_use++;
                    }
                    ++_rbuf_ptr;
                    continue;

                    break;
                }
                case parse_state::HEADER_BREAK_R: {
                    auto ch = *_rbuf_ptr;
                    if (ch == '\n') {
                        _parse_state = parse_state::HEADER_BREAK_N;
                    } else {
                        return parse_result::BAD;
                    }
                    ++_rbuf_ptr;
                    continue;
                    break;
                }
                case parse_state::HEADER_BREAK_N: { // TODO: rethink about ++_rbuf_ptr;
                    auto ch = *_rbuf_ptr;
                    if (ch == '\r') {
                        _parse_state = parse_state::PRE_BODY;
                        ++_rbuf_ptr;
                    } else {
                        _work_buf_use = 0;
                        _parse_state = parse_state::HEADER_NAME;
                    }
                    continue;
                    break;
                }
                case parse_state::PRE_BODY: {
                    auto ch = *_rbuf_ptr;
                    if (ch == '\n') {
                        _work_buf_use = 0;
                        _parse_state = parse_state::BODY;
                    } else {
                        return parse_result::BAD;
                    }
                    ++_rbuf_ptr;
                    continue;
                    break;
                }
                case parse_state::BODY: {
                    if (_body_size == 0) {
                        _body_ptr = nullptr;
                        return parse_result::GOOD;
                    } else {
                        _body_ptr = _rbuf_ptr;
                        _parse_state = parse_state::BODY_WAIT;
                    }
                    continue;
                    break;
                }
                case parse_state::BODY_WAIT: {
                    if (_body_size == 0) {
                        #ifdef ALLOW_PRINT
                        printf("body size = 0\n");
                        #endif
                        return parse_result::GOOD;
                    } else {
                        auto possible_body_size = _rbuf + _ruse - _rbuf_ptr;
                        _rbuf_ptr += possible_body_size;
                        auto s = _rbuf_ptr - _body_ptr;
                        if (s == _body_size) {
                            #ifdef ALLOW_PRINT
                            printf("body: %.*s\n", _body_size, _body_ptr);
                            #endif
                            return parse_result::GOOD;
                        } else if (_rbuf_ptr == (_rbuf + _ruse)) {
                            return parse_result::NEED_MORE;
                        } else {
                            return parse_result::BAD;
                        }
                    }
                    break;
                }
            }
        }
    }
}
