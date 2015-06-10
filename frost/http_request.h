#ifndef FROST_HTTP_REQUEST_H
#define FROST_HTTP_REQUEST_H

#include "router.h"

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

    struct http_version {
        int major_ver;
        int minor_ver;

        http_version(int major_ver, int minor_ver)
            : major_ver(major_ver),
              minor_ver(minor_ver)
        { }
    };

    struct header {
        std::string name;
        std::string value;

        header(const std::string& name, const std::string& value)
            : name(name),
              value(value)
        { }
    };


    class http_request {
        friend class http_server;
        typedef std::function<void(http_request*)> req_cb_t;
    public:
        http_request(ev::default_loop& loop, int client_fd, const path_router_t& router);
        ~http_request();

        const http_method& method() const;
        const http_version& version() const;
        const std::string& path() const;
        const std::vector<header>& headers() const;
        const char* body() const;
        uint32_t body_size() const;

    private:
        void read_cb(ev::io& w, int revents);
        void start();
        void set_cb(req_cb_t&& cb);
        void stop();
        parse_result parse();

    private:
        ev::default_loop& _loop;
        ev::io _rw;
        int _client_fd;
        char* _rbuf;
        uint32_t _ruse;
        char* _rbuf_ptr;
        static constexpr uint32_t BUF_SIZE = 4096;

        req_cb_t _cb;
        bool _has_cb;

        parse_result _parse_result;
        http_method _method;
        http_version _version;
        std::string _path;
        std::vector<header> _headers;
        char* _body_ptr;
        uint32_t _body_size;

        const path_router_t& _router;
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
