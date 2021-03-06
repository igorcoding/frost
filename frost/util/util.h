#ifndef FROST_UTIL_H
#define FROST_UTIL_H

#include <string>
#include <unordered_map>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <map>

#define memdup(a,b) memcpy(malloc(b),a,b)

#define IS_DIGIT(ch) ((ch) == '1' || (ch) == '2' || (ch) == '3' || (ch) == '4' || (ch) == '5' || (ch) == '6' || (ch) == '7' || (ch) == '8' || (ch) == '9' || (ch) == '0')

namespace frost {
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


    struct cstr_cmp : public std::binary_function<std::pair<const char*, size_t>, std::pair<const char*, size_t>, bool> {
    public:
        bool operator() (const std::pair<const char*, size_t>& str1, const std::pair<const char*, size_t>& str2) const {
            auto len = std::max(str1.second, str2.second);
            return strncmp(str1.first, str2.first, len) < 0;
        }
    };

    template <typename T>
    using cstr_map_t = std::map<std::pair<const char*, size_t>, T, cstr_cmp>;


    int atoi_positive(char* str, size_t len);

}


#endif //FROST_UTIL_H
