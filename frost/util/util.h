#ifndef FROST_UTIL_H
#define FROST_UTIL_H

#include <string>
#include <unordered_map>
#include <stdlib.h>

#define memdup(a,b) memcpy(malloc(b),a,b)

namespace frost {
    struct http_version {
    public:
        const int major_ver;
        const int minor_ver;

        http_version(int major_ver, int minor_ver)
                : major_ver(major_ver),
                  minor_ver(minor_ver),
                  _ver_str(nullptr) { }

        ~http_version() {
            delete _ver_str;
            _ver_str = nullptr;
        }

        const std::string& to_string() {
            if (_ver_str == nullptr) {
                const uint8_t max = 9;
                char* buf = new char[max];
                snprintf(buf, max, "HTTP/%d.%d", major_ver, minor_ver);
                _ver_str = new std::string(buf, max);
                delete[] buf;
            }
            return *_ver_str;
        }

    private:
        std::string* _ver_str;
    };

    struct header {
        std::string name;
        std::string value;

        header(const std::string& name, const std::string& value)
                : name(name),
                  value(value) { }
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
