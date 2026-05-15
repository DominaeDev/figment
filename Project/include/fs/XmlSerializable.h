#ifndef XML_SERIALIZABLE_H__
#define XML_SERIALIZABLE_H__
#pragma once

#include "fs/Xml.h"
#include <cassert>

namespace fig::io
{
	template<typename T>
	struct XmlMemberPointer;

	template<typename TClass, typename TValue>
	struct XmlMemberPointer<TValue TClass::*>
	{
		using Type = TValue;
	};

	template<typename TMemberPointer, typename TSerializer = std::identity, typename TDeserializer = std::identity>
	struct AsAttribute
	{
		using ValueType = XmlMemberPointer<TMemberPointer>::Type;
		using SerializedType = std::remove_cvref_t<std::invoke_result_t<TSerializer, const ValueType&>>;

		TMemberPointer member_ptr;
		const char* name;
		ValueType default_value {};
		TDeserializer custom_deserializer {};
		TSerializer custom_serializer {};
	};

	template<typename TMemberPointer, typename TSerializer = std::identity, typename TDeserializer = std::identity>
	struct AsElement
	{
		using ValueType = XmlMemberPointer<TMemberPointer>::Type;
		using SerializedType = std::remove_cvref_t<std::invoke_result_t<TSerializer, const ValueType&>>;

		TMemberPointer member_ptr;
		const char* name;
		ValueType default_value {};
		TDeserializer custom_deserializer {};
		TSerializer custom_serializer {};
	};

	template<typename T, template<typename...> typename Template>
	struct IsSpecializationOf : std::false_type {};

	template<template<typename...> typename Template, typename... Args>
	struct IsSpecializationOf<Template<Args...>, Template> : std::true_type {};

	template<typename T>
	concept IsTuple = IsSpecializationOf<T, std::tuple>::value;

	template<typename T>
	concept XmlSerializable = requires {
		{ T::XmlFields() } -> IsTuple;
	};

	template<XmlSerializable T>
	void XmlSerialize(XmlWriterElement& element, const T& object)
	{
		std::apply([&](auto&&... field) {
			([&] {
				using FieldType = std::decay_t<decltype(field)>;
				if constexpr (IsSpecializationOf<FieldType, AsAttribute>::value)
					element.SetAttribute(field.name, field.custom_serializer(object.*(field.member_ptr)));
				else if constexpr (IsSpecializationOf<FieldType, AsElement>::value)
					element.SetElementValue(field.name, field.custom_serializer(object.*(field.member_ptr)));
				else
					static_assert(false, "Serializable field must be either an element or an attribute");

			}(), ...);
		}, T::XmlFields());
	}

	template<XmlSerializable T>
	void XmlDeserialize(const XmlReaderElement& element, T& object)
	{
		std::apply([&](auto&&... field) {
			([&] {
				using FieldType = std::decay_t<decltype(field)>;
				using SerializedType = FieldType::SerializedType;

				if constexpr (IsSpecializationOf<FieldType, AsAttribute>::value)
				{
					if (auto value = element[field.name].TryGet<SerializedType>())
						object.*(field.member_ptr) = field.custom_deserializer(*value);
					else
						object.*(field.member_ptr) = field.default_value;
				}
				else if constexpr (IsSpecializationOf<FieldType, AsElement>::value)
				{
					if (auto value = element.TryGetElement<SerializedType>(field.name))
						object.*(field.member_ptr) = field.custom_deserializer(*value);
					else
						object.*(field.member_ptr) = field.default_value;
				}
				else
					static_assert(false, "Serializable field must be either an element or an attribute");
			}(), ...);
		}, T::XmlFields());
	}
}
#endif
