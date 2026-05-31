#ifndef STRING_HANDLE_H__
#define STRING_HANDLE_H__
#pragma once

#include <string>
#include <string_view>
#include <cwctype>

namespace fig
{
    std::string normalize_handle(std::string_view value, size_t max_length);
    std::wstring normalize_handle(std::wstring_view value, size_t max_length);

    template <typename T, size_t N>
    class basic_handle
    {
        static constexpr size_t MaxLength = N;
    public:
        static_assert(std::is_same_v<T, char> || std::is_same_v<T, wchar_t>, "basic_handle requires either char or wchar_t type.");

        using string_type = std::basic_string<T>;
        using string_view_type = std::basic_string_view<T>;

        basic_handle() = default;

        basic_handle(const T* value)
            : _value(normalize_handle(value, MaxLength))
        {
        }

        basic_handle(string_view_type value)
            : _value(normalize_handle(value, MaxLength))
        {
        }

        basic_handle(string_type value)
            : _value(normalize_handle(value, MaxLength))
        {
        }

        basic_handle(const basic_handle&) = default;
        basic_handle(basic_handle&&) = default;
        basic_handle& operator=(const basic_handle&) = default;
        basic_handle& operator=(basic_handle&&) = default;

        inline const string_type& to_string() const noexcept { return _value; }
        inline const char* c_str() const noexcept { return _value.c_str(); }
        inline bool empty() const noexcept { return _value.empty(); }

        operator string_type() const { return _value; }

        auto operator<=>(const basic_handle&) const = default;
        bool operator==(const basic_handle&) const = default;

        bool operator==(string_view_type other) const { return _value == normalize_handle(other, MaxLength); }
        auto operator<=>(string_view_type other) const { return _value <=> normalize_handle(other, MaxLength); }
        
    private:
        string_type _value;
    };

} // namespace

template<typename T, size_t N>
struct std::hash<fig::basic_handle<T, N>>
{
    size_t operator()(const fig::basic_handle<T, N>& basic_handle) const noexcept
    {
        return std::hash<std::basic_string<T>>{}(basic_handle.to_string());
    }
};

#endif
