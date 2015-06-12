#ifndef FROST_HTTP_REQUEST_H
#define FROST_HTTP_REQUEST_H

#include "router.h"
#include "util/util.h"
#include "http/header.h"

#include <ev++.h>
#include <vector>

namespace frost {
    enum class parse_result {
        NEED_MORE,
        GOOD,
        BAD
    };

    enum class http_method {
        NONE,
        OPTIONS,
        GET,
        HEAD,
        POST,
        PUT,
        DELETE,
        TRACE,
        CONNECT
    };


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

        static void set_max_buffer_size(uint32_t max_buffer);

    private:
        void start();
        void stop();
        parse_result parse();

    private:
        ev::io _rw;
        ev::timer _tw;
        int _client_fd;
        char* _rbuf;
        uint32_t _ruse;
        char* _rbuf_ptr;
        static uint32_t _rlen;

        parse_result _parse_result;
        http_method _method;
        http_version _version;
        std::string _path;
        std::vector<header> _headers;
        char* _body_ptr;
        uint32_t _body_size;
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
}

#endif //FROST_HTTP_REQUEST_H
