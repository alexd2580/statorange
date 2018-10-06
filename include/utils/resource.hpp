#ifndef UTILS_RESOURCE_HPP
#define UTILS_RESOURCE_HPP

#include <functional>

template <typename Resource, typename _ = void>
class UniqueResource {
    using Release = std::function<_(Resource)>;

  protected:
    Resource resource;
    Release release;

  public:
    explicit UniqueResource(Resource new_resource, Release new_release)
        : resource(new_resource), release(new_release) {}

    UniqueResource(UniqueResource const& other) = delete;
    UniqueResource& operator=(UniqueResource const& other) = delete;

    UniqueResource(UniqueResource&& other) noexcept : resource(other.resource), release(other.release) {}
    UniqueResource& operator=(UniqueResource&& other) noexcept {
        release(resource);
        resource = other.resource;
        release = other.release;
    }

    virtual ~UniqueResource() { release(resource); }
};

#endif
