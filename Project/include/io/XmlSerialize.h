#pragma once

#include "io/Xml.h"
#include <cassert>

namespace fig::data
{
	namespace xml
	{
		template<typename T>
		struct MemberPointer;

		template<typename TClass, typename TValue>
		struct MemberPointer<TValue TClass::*>
		{
			using Type = TValue;
		};

		template<typename T>
		concept SerializableMap = requires {
			typename T::key_type;
			typename T::mapped_type;
		};

		template<typename T>
		concept HasSaveToXml =
			requires (const T t, XmlWriterElement x) { { t.SaveToXml(x) } -> std::same_as<void>; };

		template<typename T>
		concept HasLoadFromXml =
			(requires (T t, XmlReaderElement x) { { t.LoadFromXml(x) } -> std::same_as<std::remove_cvref_t<bool>>; }
		or requires (T t, XmlReaderElement x) { { t.LoadFromXml(x) } -> std::same_as<fig::io::FileError>; });

		template<typename T>
		concept IsStringConvertible =
			std::constructible_from<T, fig::string> and
			std::constructible_from<fig::string, T>;

		template<typename T>
		struct InnerValueType { using Type = T; };

		template<typename T> requires SerializableMap<T>
		struct InnerValueType<T> { using Type = typename T::value_type; };

		template<typename TMemberPointer, typename TSerializer = std::identity, typename TDeserializer = std::identity>
		struct Attribute
		{
			using ValueType = MemberPointer<TMemberPointer>::Type;
			using SerializedType = std::remove_cvref_t<std::invoke_result_t<TSerializer, const ValueType&>>;
			using TValidator = std::function<bool(const ValueType&)>;

			const char* name;
			TMemberPointer member_ptr;
			TSerializer custom_serializer {};
			TDeserializer custom_deserializer {};
			ValueType default_value {};
			TValidator validator {};
			bool must_exist {};

			Attribute& Name(const char* name)
			{
				this->name = name;
				return *this;
			}

			Attribute& Default(ValueType default_value)
			{
				this->default_value = default_value;
				return *this;
			}

			Attribute& Validator(TValidator validator)
			{
				this->validator = validator;
				return *this;
			}

			Attribute& MustExist()
			{
				this->must_exist = true;
				return *this;
			}
		};

		template<typename TMemberPointer, typename TSerializer = std::identity, typename TDeserializer = std::identity>
		struct Element
		{
			using ValueType = MemberPointer<TMemberPointer>::Type;
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
			const char* collection_name {};

			Element& Name(const char* name)
			{
				this->name = name;
				return *this;
			}

			Element& Default(ValueType default_value)
			{
				this->default_value = default_value;
				return *this;
			}

			Element& Validator(TValidator validator)
			{
				this->validator = validator;
				return *this;
			}

			Element& MustExist()
			{
				this->must_exist = true;
				return *this;
			}

			Element& Collection(const char* name)
			{
				this->collection_name = name;
				return *this;
			}
		};

		template<typename TMemberPointer, typename TSerializer = std::identity, typename TDeserializer = std::identity>
		struct Text
		{
			using ValueType = MemberPointer<TMemberPointer>::Type;
			using SerializedType = std::remove_cvref_t<std::invoke_result_t<TSerializer, const ValueType&>>;
			using TValidator = std::function<bool(const ValueType&)>;

			TMemberPointer member_ptr;
			TSerializer custom_serializer {};
			TDeserializer custom_deserializer {};
			ValueType default_value {};
			TValidator validator {};
			bool must_exist {};

			Text& Default(ValueType default_value)
			{
				this->default_value = default_value;
				return *this;
			}

			Text& Validator(TValidator validator)
			{
				this->validator = validator;
				return *this;
			}

			Text& MustExist()
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
		concept Serializable = requires {
			{ T::XmlFields() } -> IsTuple;
		};

		template<typename T>
		concept SerializableRange = std::ranges::range<T> && Serializable<std::ranges::range_value_t<T>>;

		template<Serializable T>
		void Serialize(XmlWriterElement& element, const T& object)
		{
			std::apply([&](auto&&... field) {
				([&] {
					using FieldType = std::decay_t<decltype(field)>;
					auto& member = object.*(field.member_ptr);

					// Attribute
					if constexpr (IsSpecializationOf<FieldType, Attribute>::value)
					{
						if constexpr (IsStringConvertible<typename FieldType::ValueType>)
							element.SetAttribute(field.name, (fig::string)member);
						else
							element.SetAttribute(field.name, field.custom_serializer(member));
					}
					// Text
					else if constexpr (IsSpecializationOf<FieldType, Text>::value)
					{
						if constexpr (IsStringConvertible<typename FieldType::ValueType>)
							element.SetValue((fig::string)member);
						else
							element.SetValue(field.custom_serializer(member));
					}
					else if constexpr (IsSpecializationOf<FieldType, Element>::value)
					{
						// Nested object
						if constexpr (Serializable<typename FieldType::ValueType>)
						{
							auto child = element.AddChild(field.name);
							xml::Serialize(child, member);
						}
						// List of objects
						else if constexpr (SerializableRange<typename FieldType::ValueType>)
						{
							XmlWriterElement& parent = element;
							if (field.collection_name)
								parent = element.AddChild(field.collection_name);

							for (const auto& item : member)
							{
								auto child = parent.AddChild(field.name);
								xml::Serialize(child, item);
							}
						}
						// Associative container
						else if constexpr (SerializableMap<typename FieldType::ValueType>)
						{
							auto child = element.AddChild(field.name);
							for (const auto& kvp : member)
							{
								auto item = child.AddChild("Item");

								auto serialized_kvp = field.custom_serializer(kvp);
								item.SetAttribute("key", serialized_kvp.first);

								if constexpr (Serializable<decltype(serialized_kvp.second)>)
									xml::Serialize(item, serialized_kvp.second);
								else
									item.SetValue(serialized_kvp.second);
							}
						}
						// Single value
						else
						{
							if constexpr (HasSaveToXml<typename FieldType::ValueType>)
							{
								auto child = element.AddChild(field.name);
								member.SaveToXml(child);
							}
							else if constexpr (IsStringConvertible<typename FieldType::ValueType>)
							{
								element.SetElementValue(field.name, (fig::string)member);
							}
							else
							{
								element.SetElementValue(field.name, field.custom_serializer(member));
							}
						}
					}
					else
						static_assert(false, "Serializable field must be either an element or an attribute");

				}(), ...);
			}, T::XmlFields());
		}

		template<Serializable T>
		[[nodiscard]] XmlWriter Serialize(const fig::string& rootName, const T& object)
		{
			XmlWriter writer(rootName);
			auto root = writer.GetRoot();
			xml::Serialize<T>(root, object);
			return writer;
		}

		template<Serializable T>
		bool Deserialize(const XmlReaderElement& element, T& object)
		{
			bool bValid = true;

			std::apply([&](auto&&... field) {
				([&] {
					using FieldType = std::decay_t<decltype(field)>;
					auto& member = object.*(field.member_ptr);

					// Attribute
					if constexpr (IsSpecializationOf<FieldType, Attribute>::value)
					{
						if constexpr (IsStringConvertible<typename FieldType::ValueType>)
						{
							if (auto value = element[field.name].TryGet<fig::string>())
								member = typename FieldType::ValueType(*value);
							else
							{
								member = field.default_value;
								bValid &= !field.must_exist;
							}
						}
						else
						{
							if (auto value = element[field.name].TryGet<FieldType::SerializedType>())
							{
								member = field.custom_deserializer(*value);
							}
							else
							{
								member = field.default_value;
								bValid &= !field.must_exist;
							}
						}
					}
					// Text
					else if constexpr (IsSpecializationOf<FieldType, Text>::value)
					{
						if constexpr (IsStringConvertible<typename FieldType::ValueType>)
						{
							if (auto value = element.TryGetValue<fig::string>())
								member = typename FieldType::ValueType(*value);
							else
							{
								member = field.default_value;
								bValid &= !field.must_exist;
							}
						}
						else
						{
							if (auto value = element.TryGetValue<FieldType::SerializedType>())
							{
								member = field.custom_deserializer(*value);
							}
							else
							{
								member = field.default_value;
								bValid &= !field.must_exist;
							}
						}
					}
					else if constexpr (IsSpecializationOf<FieldType, Element>::value)
					{
						// Nested object
						if constexpr (Serializable<typename FieldType::ValueType>)
						{
							// Mustn't have converters
							static_assert(std::same_as<decltype(field.custom_serializer), std::identity>);
							static_assert(std::same_as<decltype(field.custom_deserializer), std::identity>);

							auto child = element.GetFirstElement(field.name);
							if (child)
								bValid &= xml::Deserialize(*child, member);
							else
								bValid &= !field.must_exist;
						}
						// List of objects
						else if constexpr (SerializableRange<typename FieldType::ValueType>)
						{
							using ItemType = std::ranges::range_value_t<typename FieldType::ValueType>;
							member.clear();
							const XmlReaderElement& parent = field.collection_name ?
								element.GetFirstElement(field.collection_name).value_or(element) :
								element;

							if (auto child = parent.GetFirstElement(field.name))
							{
								while (child)
								{
									bValid &= xml::Deserialize(*child, member.emplace_back());
									child = child->GetNextSibling();
								}
							}
							else
								bValid &= !field.must_exist;
						}
						// Associative container
						else if constexpr (SerializableMap<typename FieldType::ValueType>)
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
										if constexpr (Serializable<MappedType>)
										{
											auto& value = member[field.custom_deserializer(typename FieldType::SerializedType { *try_key, {} }).first];
											xml::Deserialize(*item, value);
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
							else
								bValid &= !field.must_exist;
						}
						// Single values
						else
						{
							if constexpr (HasLoadFromXml<typename FieldType::ValueType>)
							{
								if (auto child = element.GetFirstElement(field.name))
									bValid &= Success(member.LoadFromXml(child.value())) or !field.must_exist;
								else
									bValid &= !field.must_exist;
							}
							else if constexpr (IsStringConvertible<typename FieldType::ValueType>)
							{
								if (auto value = element.TryGetElement<fig::string>(field.name))
									member = typename FieldType::ValueType(*value);
								else
								{
									member = field.default_value;
									bValid &= !field.must_exist;
								}
							}
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
					}
					else
						static_assert(false, "Serializable field must be either an element or an attribute");

					if (bValid && field.validator)
						bValid &= field.validator(member);
				}(), ...);
			}, T::XmlFields());

			return bValid;
		}

		template<Serializable T>
		[[nodiscard]] std::optional<T> Deserialize(fig::string_view const doc, const fig::string& rootName = {})
		{
			XmlReader reader(doc);
			if (not reader.IsOk())
				return std::nullopt;

			auto& root = reader.GetRoot();
			if (not rootName.empty() and root.GetName() != rootName)
				return std::nullopt;

			T object {};
			if (xml::Deserialize<T>(root, object))
				return object;
			return std::nullopt;
		}

		template<Serializable T>
		[[nodiscard]] std::optional<T> Deserialize(const fig::byte_span& doc, const fig::string& rootName = {})
		{
			return xml::Deserialize<T>(fig::string_view { reinterpret_cast<const char*>(doc.data()), doc.size() }, rootName);
		}

		template<typename... TFields>
		[[nodiscard]] auto Fields(TFields&&... fields)
		{
			return std::make_tuple(std::forward<TFields>(fields)...);
		}
	}

	template<typename TMemberPointer, typename TSerializer = std::identity, typename TDeserializer = std::identity>
	using Attribute = xml::Attribute<TMemberPointer, TSerializer, TDeserializer>;

	template<typename TMemberPointer, typename TSerializer = std::identity, typename TDeserializer = std::identity>
	using Element = xml::Element<TMemberPointer, TSerializer, TDeserializer>;

	template<typename TMemberPointer, typename TSerializer = std::identity, typename TDeserializer = std::identity>
	using Text = xml::Text<TMemberPointer, TSerializer, TDeserializer>;

	template<typename T>
	concept IsXmlSerializable = xml::Serializable<T>;

	template<xml::Serializable T>
	inline void Serialize(XmlWriterElement& element, const T& object) { return xml::Serialize(element, object); }

	template<xml::Serializable T>
	[[nodiscard]] inline XmlWriter Serialize(const fig::string& rootName, const T& object) { return xml::Serialize(rootName, object); }

	template<xml::Serializable T>
	[[nodiscard]] inline bool Deserialize(const XmlReaderElement& element, T& object) noexcept { return xml::Deserialize(element, object); }

	template<xml::Serializable T>
	[[nodiscard]] inline std::optional<T> Deserialize(fig::string_view const doc, const fig::string& rootName = {}) noexcept { return xml::Deserialize(doc, rootName); }

	template<xml::Serializable T>
	[[nodiscard]] inline std::optional<T> Deserialize(const fig::byte_span& doc, const fig::string& rootName = {}) noexcept { return xml::Deserialize(doc, rootName); }
}
