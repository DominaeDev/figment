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
	struct XmlAttribute
	{
		using ValueType = XmlMemberPointer<TMemberPointer>::Type;
		using SerializedType = std::remove_cvref_t<std::invoke_result_t<TSerializer, const ValueType&>>;
		using TValidator = std::function<bool(const ValueType&)>;

		TMemberPointer member_ptr;
		const char* name;
		TDeserializer custom_deserializer {};
		TSerializer custom_serializer {};
		ValueType default_value {};
		TValidator validator {};

		XmlAttribute& Default(ValueType default_value)
		{
			this->default_value = default_value;
			return *this;
		}

		XmlAttribute& Validator(TValidator validator)
		{
			this->validator = validator;
			return *this;
		}
	};

	template<typename TMemberPointer, typename TSerializer = std::identity, typename TDeserializer = std::identity>
	struct XmlElement
	{
		using ValueType = XmlMemberPointer<TMemberPointer>::Type;
		using SerializedType = std::remove_cvref_t<std::invoke_result_t<TSerializer, const ValueType&>>;
		using TValidator = std::function<bool(const ValueType&)>;

		TMemberPointer member_ptr;
		const char* name;
		TDeserializer custom_deserializer {};
		TSerializer custom_serializer {};
		ValueType default_value {};
		TValidator validator {};

		XmlElement& Default(ValueType default_value)
		{
			this->default_value = default_value;
			return *this;
		}

		XmlElement& Validator(TValidator validator)
		{
			this->validator = validator;
			return *this;
		}
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

	template<typename T>
	concept XmlSerializableRange = std::ranges::range<T> && XmlSerializable<std::ranges::range_value_t<T>>;

	template<XmlSerializable T>
	void XmlSerialize(XmlWriterElement& element, const T& object)
	{
		std::apply([&](auto&&... field) {
			([&] {
				using FieldType = std::decay_t<decltype(field)>;
				if constexpr (IsSpecializationOf<FieldType, XmlAttribute>::value)
					element.SetAttribute(field.name, field.custom_serializer(object.*(field.member_ptr)));
				else if constexpr (IsSpecializationOf<FieldType, XmlElement>::value)
				{
					if constexpr (XmlSerializableRange<typename FieldType::ValueType>)
					{
						for (const auto& item : object.*(field.member_ptr))
						{
							auto child = element.AddChild(field.name);
							XmlSerialize(child, item);
						}
					}
					else if constexpr (XmlSerializable<typename FieldType::ValueType>)
					{
						auto child = element.AddChild(field.name);
						XmlSerialize(child, (object.*(field.member_ptr)));
					}
					else
					{
						element.SetElementValue(field.name, field.custom_serializer(object.*(field.member_ptr)));
					}
				}
				else
					static_assert(false, "Serializable field must be either an element or an attribute");

			}(), ...);
		}, T::XmlFields());
	}

	template<XmlSerializable T>
	bool XmlDeserialize(const XmlReaderElement& element, T& object)
	{
		bool bValid = true;
		std::apply([&](auto&&... field) {
			([&] {
				using FieldType = std::decay_t<decltype(field)>;
				if constexpr (IsSpecializationOf<FieldType, XmlAttribute>::value)
				{
					if (auto value = element[field.name].TryGet<FieldType::SerializedType>())
						object.*(field.member_ptr) = field.custom_deserializer(*value);
					else
						object.*(field.member_ptr) = field.default_value;
				}
				else if constexpr (IsSpecializationOf<FieldType, XmlElement>::value)
				{
					// Nested object
					if constexpr (XmlSerializable<typename FieldType::ValueType>)
					{
						static_assert(std::same_as<decltype(field.custom_serializer), std::identity>);
						static_assert(std::same_as<decltype(field.custom_deserializer), std::identity>);

						auto child = element.GetFirstElement(field.name);
						if (child)
							XmlDeserialize(*child, (object.*(field.member_ptr)));
					}
					// Nested objects
					else if constexpr (XmlSerializableRange<typename FieldType::ValueType>)
					{
						using ItemType = std::ranges::range_value_t<typename FieldType::ValueType>;
						auto& container = object.*(field.member_ptr);
						container.clear();
						auto child = element.GetFirstElement(field.name);
						while (child)
						{
							XmlDeserialize(*child, container.emplace_back());
							child = child->GetNextSibling();
						}
					}
					else
					{
						if (auto value = element.TryGetElement<FieldType::SerializedType>(field.name))
							object.*(field.member_ptr) = field.custom_deserializer(*value);
						else
							object.*(field.member_ptr) = field.default_value;
					}
				}
				else
					static_assert(false, "Serializable field must be either an element or an attribute");

				if (field.validator)
				{
					bValid &= field.validator(object.*(field.member_ptr));
				}
			}(), ...);
		}, T::XmlFields());
		return bValid;
	}
}
#endif
