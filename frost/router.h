#ifndef FROST_ROUTER_H
#define FROST_ROUTER_H

#include <unordered_map>
#include <functional>

namespace frost {

    template <typename C>
    class router {
    public:
        void add_route(const std::string& path, const C& cb);
        void add_route(const char* path, const C& cb);

        const C* get_route(const std::string& path) const;

    private:
        std::unordered_map<std::string, C> _paths;
    };

    class http_request;
    class http_response;
    typedef std::function<void(http_request&, http_response&)> cb_t;
    typedef router<cb_t> path_router_t;

    template <typename C> inline
    void router<C>::add_route(const std::string& path, const C& cb) {
        _paths[path] = std::move(cb);
    }

    template <typename C> inline
    void router<C>::add_route(const char* path, const C& cb) {
        _paths[path] = std::move(cb);
    }

    template <typename C> inline
    const C* router<C>::get_route(const std::string& path) const {
        auto it = _paths.find(path);
        if (it != _paths.end()) {
            return &it->second;
        }
        return nullptr;
    }
}

#endif //FROST_ROUTER_H
