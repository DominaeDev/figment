#ifndef XML_SERIALIZABLE_H__
#define XML_SERIALIZABLE_H__
#pragma once

#include "io/Xml.h"
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

	template<typename T>
	struct InnerValueType { using Type = T; };

	template<typename T> requires XmlSerializableMap<T>
	struct InnerValueType<T> { using Type = typename T::value_type; };

	template<typename TMemberPointer, typename TSerializer = std::identity, typename TDeserializer = std::identity>
	struct XmlAttribute
	{
		using ValueType = XmlMemberPointer<TMemberPointer>::Type;
		using SerializedType = std::remove_cvref_t<std::invoke_result_t<TSerializer, const ValueType&>>;
		using TValidator = std::function<bool(const ValueType&)>;

		const char* name;
		TMemberPointer member_ptr;
		TSerializer custom_serializer {};
		TDeserializer custom_deserializer {};
		ValueType default_value {};
		TValidator validator {};
		bool must_exist {};

		XmlAttribute& Name(const char* name)
		{
			this->name = name;
			return *this;
		}

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

		XmlAttribute& MustExist()
		{
			this->must_exist = true;
			return *this;
		}
	};

	template<typename TMemberPointer, typename TSerializer = std::identity, typename TDeserializer = std::identity>
	struct XmlElement
	{
		using ValueType = XmlMemberPointer<TMemberPointer>::Type;
		using SerializerInputType = typename InnerValueType<ValueType>::Type;
		using SerializedType = std::remove_cvref_t<std::invoke_result_t<TSerializer, const SerializerInputType&>>;
		using TValidator = std::function<bool(const ValueType&)>;

		const char* name;
		TMemberPointer member_ptr;
		TSerializer custom_serializer {};
		TDeserializer custom_deserializer {};
		ValueType default_value {};
		TValidator validator {};
		bool must_exist {};

		XmlElement& Name(const char* name)
		{
			this->name = name;
			return *this;
		}

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

		XmlElement& MustExist()
		{
			this->must_exist = true;
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
		{ T::GetXmlFields() } -> IsTuple;
	};

	template<typename T>
	concept XmlSerializableRange = std::ranges::range<T> && XmlSerializable<std::ranges::range_value_t<T>>;

	template<typename T>
	concept XmlSerializableMap = requires {
		typename T::key_type;
		typename T::mapped_type;
	};

	template<XmlSerializable T>
	void XmlSerialize(XmlWriterElement& element, const T& object)
	{
		std::apply([&](auto&&... field) {
			([&] {
				using FieldType = std::decay_t<decltype(field)>;
				auto& member = object.*(field.member_ptr);

				// Attribute
				if constexpr (IsSpecializationOf<FieldType, XmlAttribute>::value)
				{
					element.SetAttribute(field.name, field.custom_serializer(member));
				}
				else if constexpr (IsSpecializationOf<FieldType, XmlElement>::value)
				{
					// Nested object
					if constexpr (XmlSerializable<typename FieldType::ValueType>)
					{
						auto child = element.AddChild(field.name);
						XmlSerialize(child, member);
					}
					// List of objects
					else if constexpr (XmlSerializableRange<typename FieldType::ValueType>)
					{
						for (const auto& item : member)
						{
							auto child = element.AddChild(field.name);
							XmlSerialize(child, item);
						}
					}
					// Associative container
					else if constexpr (XmlSerializableMap<typename FieldType::ValueType>)
					{
						auto child = element.AddChild(field.name);
						for (const auto& kvp : member)
						{
							auto item = child.AddChild("Item");

							auto serialized_kvp = field.custom_serializer(kvp);
							item.SetAttribute("key", serialized_kvp.first);

							if constexpr (XmlSerializable<decltype(serialized_kvp.second)>)
								XmlSerialize(item, serialized_kvp.second);
							else
								item.SetValue(serialized_kvp.second);
						}
					}
					// Single value
					else
					{
						element.SetElementValue(field.name, field.custom_serializer(member));
					}
				}
				else
					static_assert(false, "Serializable field must be either an element or an attribute");

			}(), ...);
		}, T::GetXmlFields());
	}

	template<XmlSerializable T>
	bool XmlDeserialize(const XmlReaderElement& element, T& object)
	{
		bool bValid = true;

		std::apply([&](auto&&... field) {
			([&] {
				using FieldType = std::decay_t<decltype(field)>;
				auto& member = object.*(field.member_ptr);

				// Attribute
				if constexpr (IsSpecializationOf<FieldType, XmlAttribute>::value)
				{
					if (auto value = element[field.name].TryGet<FieldType::SerializedType>())
						member = field.custom_deserializer(*value);
					else
					{
						member = field.default_value;
						bValid &= !field.must_exist;
					}
				}
				else if constexpr (IsSpecializationOf<FieldType, XmlElement>::value)
				{
					// Nested object
					if constexpr (XmlSerializable<typename FieldType::ValueType>)
					{
						// Mustn't have converters
						static_assert(std::same_as<decltype(field.custom_serializer), std::identity>);
						static_assert(std::same_as<decltype(field.custom_deserializer), std::identity>);

						auto child = element.GetFirstElement(field.name);
						if (child)
							bValid &= XmlDeserialize(*child, member);
						else 
							bValid &= !field.must_exist;
					}
					// List of objects
					else if constexpr (XmlSerializableRange<typename FieldType::ValueType>)
					{
						using ItemType = std::ranges::range_value_t<typename FieldType::ValueType>;
						member.clear();
						if (auto child = element.GetFirstElement(field.name))
						{
							while (child)
							{
								bValid &= XmlDeserialize(*child, member.emplace_back());
								child = child->GetNextSibling();
							}
						}
						else
							bValid &= !field.must_exist;
					}
					// Associative container
					else if constexpr (XmlSerializableMap<typename FieldType::ValueType>)
					{
						using MappedType = FieldType::ValueType::mapped_type;
						using SerializedKeyType = FieldType::SerializedType::first_type;
						using SerializedMappedType = FieldType::SerializedType::second_type;
						
						member.clear();

						if (auto child = element.GetFirstElement(field.name))
						{
							std::optional<XmlReaderElement> item = child.value().GetFirstElement("Item");
							while (item)
							{
								if (auto try_key = (*item)["key"].TryGet<std::remove_cvref_t<SerializedKeyType>>())
								{
									// Map of objects
									if constexpr (XmlSerializable<MappedType>)
									{
										auto& value = member[field.custom_deserializer(typename FieldType::SerializedType { *try_key, {} }).first];
										XmlDeserialize(*item, value);
									}
									// Map of trivial types
									else
									{
										if (auto try_value = (*item).TryGetValue<std::remove_cvref_t<SerializedMappedType>>())
										{
											auto deserialized_kvp = field.custom_deserializer(typename FieldType::SerializedType { *try_key, *try_value });
											member[deserialized_kvp.first] = deserialized_kvp.second;
										}
									}
								}
								item = item->GetNextSibling();
							}
						}
					}
					// Single values
					else
					{
						if (auto value = element.TryGetElement<FieldType::SerializedType>(field.name))
							member = field.custom_deserializer(*value);
						else
						{
							member = field.default_value;
							bValid &= !field.must_exist;
						}
					}
				}
				else
					static_assert(false, "Serializable field must be either an element or an attribute");

				if (field.validator)
					bValid &= field.validator(member);
			}(), ...);
		}, T::GetXmlFields());

		return bValid;
	}

	template<XmlSerializable T>
	[[nodiscard]] XmlWriter XmlSerialize(const fig::string& rootName, const T& object)
	{
		XmlWriter writer(rootName);
		auto root = writer.GetRoot();
		XmlSerialize<T>(root, object);
		return writer;
	}

	template<XmlSerializable T>
	[[nodiscard]] std::optional<T> XmlDeserialize(fig::string_view const doc, const fig::string& rootName = {})
	{
		XmlReader reader(doc);
		if (not reader.IsOk())
			return std::nullopt;

		auto& root = reader.GetRoot();
		if (not rootName.empty() and root.GetName() != rootName)
			return std::nullopt;

		T object {};
		if (XmlDeserialize<T>(root, object))
			return object;
		return std::nullopt;
	}

	template<XmlSerializable T>
	[[nodiscard]] std::optional<T> XmlDeserialize(const fig::byte_span& doc, const fig::string& rootName = {})
	{
		return XmlDeserialize<T>(fig::string_view { reinterpret_cast<const char*>(doc.data()), doc.size() }, rootName);
	}

	template <typename T>
	static fig::string XmlConvertString(const T& value)
	{
		return fig::string { value };
	}

	template <typename T>
	static int32_t XmlConvertInteger(const T& value)
	{
		return static_cast<int32_t>(value);
	}

	template <typename T, typename U = std::underlying_type_t<T>> requires std::is_enum_v<T>
	static U XmlConvertEnum(const T& value)
	{
		return static_cast<U>(value);
	}

	template <typename T, typename U = std::underlying_type_t<T>> requires std::is_enum_v<T>
	static T XmlParseEnum(const U& value)
	{
		return static_cast<T>(value);
	}

	template<typename... TFields>
	[[nodiscard]] auto XmlFields(TFields&&... fields)
	{
		return std::make_tuple(std::forward<TFields>(fields)...);
	}
}
#endif
