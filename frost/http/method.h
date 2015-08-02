#ifndef FROST_METHOD_H
#define FROST_METHOD_H

#include <string>
#include "../util/util.h"

namespace frost {
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

    struct http_method_assist
    {
    public:
        static http_method from_str(const char* str, size_t len) {
            auto i = _desc.find(std::make_pair(str, len));
            if (i == _desc.end()) {
                return http_method::NONE;
            } else {
                return i->second;
            }
        }

    private:
        static cstr_map_t<http_method> _desc;
    };
}

#endif //FROST_METHOD_H
