#include "util/mapper.h"
#include "method.h"

namespace frost {
    cstr_map_t<http_method> http_method_assist::_desc;
   // frost::unordered_map<std::string, http_method> http_method_assist::_desc
   //         = mapper<std::string, http_method>()
   //                 ("OPTIONS", http_method::OPTIONS)
   //                 ("GET", http_method::GET)
   //                 ("HEAD", http_method::HEAD)
   //                 ("POST", http_method::POST)
   //                 ("PUT", http_method::PUT)
   //                 ("DELETE", http_method::DELETE)
   //                 ("TRACE", http_method::TRACE)
   //                 ("CONNECT", http_method::CONNECT)
   // ;
}
