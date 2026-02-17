#ifndef TYPE_ID_H__
#define TYPE_ID_H__
#pragma once

#include <concepts>

using type_id_t = size_t;

template <typename T>
concept HasTypeId = requires(T t)
{
    { T::type_id } -> std::same_as<const type_id_t&>;
};

template <HasTypeId T>
type_id_t type_id()
{
    return T::type_id;
};

#define TYPE_ID(x) inline static constexpr type_id_t type_id { x }


#endif
