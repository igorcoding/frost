#include <util/mapper.h>
#include "status.h"

namespace frost
{

    frost::unordered_map<status_code, std::string> status_code_assist::_desc
                                = mapper<status_code, std::string>()
                    (status_code::OK, "OK")
                    (status_code::BAD_REQUEST, "Bad Request")
                    (status_code::FORBIDDEN, "Forbidden")
                    (status_code::NOT_FOUND, "Not Found")
                    (status_code::REQUEST_ENTITY_TOO_LARGE, "Request Entity Too Large")
    ;

}