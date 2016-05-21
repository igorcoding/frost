#ifndef FROST_HTTP_REQUEST_H
#define FROST_HTTP_REQUEST_H

#include "router.h"
#include "util/util.h"
#include "http/header.h"
#include "http/http_version.h"

#include <ev++.h>
#include <vector>
#include <evsrv_conn++.h>
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


    class http_server;
    class http_response;
    class http_request : public ev::srv_conn {
        friend class http_server;
    public:
        http_request(http_server& srv, evsrv_conn_info* info);
        virtual ~http_request();

        const http_method& method() const;
        const http_version& version() const;
        const char* path() const;
        size_t path_len() const;
        const std::vector<header>& headers() const;
        const char* body() const;
        uint32_t body_size() const;

//        static void set_max_buffer_size(uint32_t max_buffer);

    private:
        parse_result parse();
        virtual void clean() override;

        void set_http_response(http_response* resp);
        http_response* get_http_response();

    private:
        char* _rbuf_ptr;

        parse_result _parse_result;
        parse_state _parse_state;
        http_method _method;
        http_version _version;
        const char* _path;
        size_t _path_len;
        std::vector<header> _headers;
        char* _body_ptr;
        uint32_t _body_size;

        http_response* __resp;

        static size_t path_max_size;

        size_t _work_buf_use;
        const char* _current_header_name;
        size_t _current_header_len;
        bool _content_length_coming;
    };


    inline const http_method& http_request::method() const {
        return _method;
    }

    inline const http_version& http_request::version() const {
        return _version;
    }

    inline const char* http_request::path() const {
        return _path;
    }

    inline size_t http_request::path_len() const {
        return _path_len;
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
