#ifndef UTILS_RESOURCE_HPP
#define UTILS_RESOURCE_HPP

#include <functional>
#include <iostream>

template <typename Resource, typename _ = void>
class UniqueResource {
    using Release = std::function<_(Resource)>;

  protected:
    Resource resource;
    Release release;
    bool active;

  public:
    explicit UniqueResource(Resource new_resource, Release new_release)
        : resource(new_resource), release(new_release), active(true) {}

    UniqueResource(UniqueResource const& other) = delete;
    UniqueResource& operator=(UniqueResource const& other) = delete;

    UniqueResource(UniqueResource&& other) noexcept
        : resource(other.resource), release(other.release), active(other.active) {
        other.active = false;
    }
    UniqueResource& operator=(UniqueResource&& other) noexcept {
        if(active) {
            release(resource);
        }
        resource = other.resource;
        release = other.release;
        active = other.active;

        other.active = false;
    }

    Resource const& get() const {
        return resource;
    }

    virtual ~UniqueResource() {
        if(active) {
            release(resource);
        }
    }
};

#endif
