#ifndef FROST_MAPPER_H
#define FROST_MAPPER_H

#include <map>
#include "util.h"

namespace frost {

    template<typename T, typename U>
    class mapper {
    typedef frost::unordered_map<T, U> MAP;
    public:
        mapper() {
        }

        mapper<T, U>& operator()(const T& key, const U& val) {
            _map[key] = val;
            return *this;
        }

        operator MAP() {
            return _map;
        }

    private:
        MAP _map;
    };

}

#endif //FROST_MAPPER_H
