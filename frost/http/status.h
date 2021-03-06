#ifndef FROST_STATUS_H
#define FROST_STATUS_H

#include "../util/util.h"

#include <string>

namespace frost
{
    enum status_code {
        OK = 200,

        BAD_REQUEST = 400,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        METHOD_NOT_ALLOWED = 405,
        REQUEST_ENTITY_TOO_LARGE = 413,
    };

    struct status_code_assist
    {
    public:
        static const std::string& desc(status_code s) {
            auto i = _desc.find(s);
            if (i == _desc.end()) {
                return _empty;
            } else {
                return i->second;
            }
        }

    private:
        static const std::string _empty;
        static frost::unordered_map<status_code, std::string> _desc;
    };
}

#endif //FROST_STATUS_H
