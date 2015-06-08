#ifndef FROST_HTTP_REQUEST_H
#define FROST_HTTP_REQUEST_H

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
    public:
        http_request(ev::default_loop& loop, int client_fd);
        ~http_request();

        const http_method& method();
        const http_version& version();
        const std::string& path();
        const std::vector<header>& headers();
        const char* body();
        uint32_t body_size();

    private:
        void read_cb(ev::io& w, int revents);
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

        parse_result _parse_result;
        http_method _method;
        http_version _version;
        std::string _path;
        std::vector<header> _headers;
        char* _body_ptr;
        uint32_t _body_size;
    };



    inline const http_method& http_request::method() {
        return _method;
    }

    inline const http_version& http_request::version() {
        return _version;
    }

    inline const std::string& http_request::path() {
        return _path;
    }

    inline const std::vector<header>& http_request::headers() {
        return _headers;
    }

    inline const char* http_request::body() {
        return _body_ptr;
    }

    inline uint32_t http_request::body_size() {
        return _body_size;
    }
}

#endif //FROST_HTTP_REQUEST_H
