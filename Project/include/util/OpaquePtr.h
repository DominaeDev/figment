#pragma once

#include <string>

namespace fig
{
    class opaque_ptr
    {
        friend struct std::hash<opaque_ptr>;
    public:
        opaque_ptr() : _ptr(0) {}
        template <typename T>
        opaque_ptr(T* ptr) : _ptr(ptr) {}

        explicit operator bool() const noexcept { return (bool)_ptr; }

        bool operator==(const opaque_ptr&) const = default;
        std::strong_ordering operator<=>(const opaque_ptr&) const = default;

    private:
        const void* _ptr;
    };
}

template <>
struct std::hash<fig::opaque_ptr>
{
    std::size_t operator()(const fig::opaque_ptr& id) const noexcept
    {
        return std::hash<const void*>{}(id._ptr);
    }
};