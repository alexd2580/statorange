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
    bool active = false;

  public:
    // Annotate with `enable_if` for primitives?
    explicit UniqueResource() : resource(), release() {}
    explicit UniqueResource(Resource new_resource, Release new_release = [](Resource) {})
        : resource(new_resource), release(new_release), active(true) {}

    UniqueResource(UniqueResource const& other) = delete;
    UniqueResource& operator=(UniqueResource const& other) = delete;

    UniqueResource(UniqueResource&& other) noexcept
        : resource(std::move(other.resource)), release(std::move(other.release)), active(other.active) {
        other.active = false;
    }
    UniqueResource& operator=(UniqueResource&& other) noexcept {
        if(active) {
            release(resource);
        }
        resource = std::move(other.resource);
        release = std::move(other.release);
        active = other.active;

        other.active = false;
        return *this;
    }

    bool is_active() const { return active; }
    Resource const& get() const { return resource; }

    virtual ~UniqueResource() {
        if(active) {
            release(resource);
        }
    }
};

#endif
