#ifndef FROST_HTTP_REQUEST_H
#define FROST_HTTP_REQUEST_H

#include "router.h"
#include "util/util.h"
#include "http/header.h"
#include "http/http_version.h"

#include <ev++.h>
#include <vector>
#include "http/method.h"

namespace frost {
    enum class parse_result {
        NEED_MORE,
        GOOD,
        BAD
    };

    enum class parse_state {
        BEGIN,
        METHOD,
        PATH,
        PROTOCOL,
        PROTOCOL_H,
        PROTOCOL_T1,
        PROTOCOL_T2,
        PROTOCOL_P,
        PROTOCOL_MAJOR,
        PROTOCOL_MINOR,
        STATUS_LINE_BREAK_R,
        STATUS_LINE_BREAK_N,
        HEADER_NAME,
        HEADER_VALUE,
        HEADER_BREAK_R,
        HEADER_BREAK_N,
        PRE_BODY,
        BODY,
        BODY_WAIT
    };


    class http_response;
    class http_request {
        friend class http_server;
    public:
        http_request(int client_fd);
        ~http_request();

        const http_method& method() const;
        const http_version& version() const;
        const std::string& path() const;
        const std::vector<header>& headers() const;
        const char* body() const;
        uint32_t body_size() const;

        bool is_keep_alive() const;

        static void set_max_buffer_size(uint32_t max_buffer);

    private:
        void start();
        void start_keep_alive();
        void stop();
        parse_result parse();
        void clear();

        void set_http_response(http_response* resp);
        http_response* get_http_response();

    private:
        ev::io _rw;
        ev::timer _tw;
        ev::timer _keep_alive_w;
        int _client_fd;
        char* _rbuf;
        uint32_t _ruse;
        char* _rbuf_ptr;
        static uint32_t _rlen;

        parse_result _parse_result;
        parse_state _parse_state;
        http_method _method;
        http_version _version;
        std::string _path;
        std::vector<header> _headers;
        char* _body_ptr;
        uint32_t _body_size;

        bool _keep_alive;

        http_response* __resp;

        static size_t _KEEP_ALIVE_COUNT;
        static constexpr size_t _KEEP_ALIVE_MAX = 100;  // TODO: this is invalid because of fork

        static size_t path_max_size;

        char* _work_buf;
        size_t _work_buf_len;
        size_t _work_buf_use;
        bool _content_length_await;
    };


    inline const http_method& http_request::method() const {
        return _method;
    }

    inline const http_version& http_request::version() const {
        return _version;
    }

    inline const std::string& http_request::path() const {
        return _path;
    }

    inline const std::vector<header>& http_request::headers() const {
        return _headers;
    }

    inline const char* http_request::body() const {
        return _body_ptr;
    }

    inline uint32_t http_request::body_size() const {
        return _body_size;
    }

    inline bool http_request::is_keep_alive() const {
        return _keep_alive;
    }
}

#endif //FROST_HTTP_REQUEST_H
