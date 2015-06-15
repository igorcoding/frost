#ifndef FROST_HEADER_H
#define FROST_HEADER_H

#include <stdio.h>
#include <string>

namespace frost {

    struct header {
        const char* const name;
        size_t name_len;
        const char* const value;
        size_t value_len;

        header(const char* const name, size_t name_len, const char* const value, size_t value_len)
                : name(name),
                  name_len(name_len),
                  value(value),
                  value_len(value_len) { }



//        static int build_header_str(char* buf, size_t buf_len, const header& h);
        template<typename VALUE> static int build_header_str(char* buf, size_t buf_len, const char* name, VALUE value);
    };

//    inline int header::build_header_str(char* buf, size_t buf_len, const header& h) {
//        return build_header_str(buf, buf_len, h.name.c_str(), h.value.c_str());
//    }

    template<typename VALUE> inline
    int header::build_header_str(char* buf, size_t buf_len, const char* name, VALUE value) {
        static_assert(sizeof(VALUE) == -1, "This value type is not supported");
        return -1;
    }

    template<> inline
    int header::build_header_str(char* buf, size_t buf_len, const char* name, const char* value) {
        return snprintf(buf, buf_len, "%s: %s\r\n", name, value);
    }

    template<> inline
    int header::build_header_str(char* buf, size_t buf_len, const char* name, const std::string& value) {
        return snprintf(buf, buf_len, "%s: %s\r\n", name, value.c_str());
    }

    template<> inline
    int header::build_header_str(char* buf, size_t buf_len, const char* name, int value) {
        return snprintf(buf, buf_len, "%s: %d\r\n", name, value);
    }

    template<> inline
    int header::build_header_str(char* buf, size_t buf_len, const char* name, unsigned int value) {
        return snprintf(buf, buf_len, "%s: %u\r\n", name, value);
    }

    template<> inline
    int header::build_header_str(char* buf, size_t buf_len, const char* name, long int value) {
        return snprintf(buf, buf_len, "%s: %ld\r\n", name, value);
    }

    template<> inline
    int header::build_header_str(char* buf, size_t buf_len, const char* name, long long int value) {
        return snprintf(buf, buf_len, "%s: %lld\r\n", name, value);
    }

    template<> inline
    int header::build_header_str(char* buf, size_t buf_len, const char* name, size_t value) {
        return snprintf(buf, buf_len, "%s: %zu\r\n", name, value);
    }

    template<> inline
    int header::build_header_str(char* buf, size_t buf_len, const char* name, double value) {
        return snprintf(buf, buf_len, "%s: %f\r\n", name, value);
    }

}

#endif //FROST_HEADER_H
