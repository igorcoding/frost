#ifndef FROST_HTTP_VERSION_H
#define FROST_HTTP_VERSION_H

#include <string>

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


        void set_major_ver(int major_ver) {
            _major_ver = major_ver;
        }

        void set_minor_ver(int minor_ver) {
            _minor_ver = minor_ver;
        }

        void clear() {
            _major_ver = 0;
            _minor_ver = 0;
            delete _ver_str;
            _ver_str = nullptr;
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
}

#endif //FROST_HTTP_VERSION_H
