#ifndef FROST_UTIL_H
#define FROST_UTIL_H

#include <string>
#include <unordered_map>
#include <stdlib.h>
#include <iostream>

#define memdup(a,b) memcpy(malloc(b),a,b)

namespace frost {
    class http_version {
    public:
        http_version(int major_ver, int minor_ver)
                : _major_ver(major_ver),
                  _minor_ver(minor_ver),
                  _ver_str(nullptr) { }

        ~http_version() {
            delete _ver_str;
            _ver_str = nullptr;
        }

        int major_ver() const { return _major_ver; }
        int minor_ver() const { return _minor_ver; }

        void clear() {
            _major_ver = 0;
            _minor_ver = 0;
        }

        const std::string& to_string() {
            if (_ver_str == nullptr) {
                const uint8_t max = 9;
                char* buf = new char[max];
                snprintf(buf, max, "HTTP/%d.%d", _major_ver, _minor_ver);
                _ver_str = new std::string(buf);
                delete[] buf;
            }
            return *_ver_str;
        }

    private:
        int _major_ver;
        int _minor_ver;
        std::string* _ver_str;
    };

    struct enum_class_hash
    {
        template <typename T>
        std::size_t operator()(T t) const
        {
            return static_cast<std::size_t>(t);
        }
    };

    template <typename Key>
    using HashType = typename std::conditional<std::is_enum<Key>::value, enum_class_hash, std::hash<Key>>::type;

    template <typename Key, typename T>
    using unordered_map = std::unordered_map<Key, T, HashType<Key>>;

}


#endif //FROST_UTIL_H
