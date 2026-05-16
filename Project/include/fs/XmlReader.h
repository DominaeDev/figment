#ifndef XML_READER_H__
#define XML_READER_H__
#pragma once

#include "Types.h"

namespace tinyxml2
{
	class XMLDocument;
	class XMLElement;
	class XMLAttribute;
}

namespace fig::io
{
	class XmlReaderAttribute
	{
		friend class XmlReaderElement;
		XmlReaderAttribute() = delete;
		XmlReaderAttribute(const tinyxml2::XMLAttribute* pAttribute) noexcept;

	public:
		inline bool IsOk() const noexcept { return (bool)_pAttrib; }

		template<typename T>
		std::optional<T> TryGet() const = delete;

		template<>
		std::optional<bool> TryGet<bool>() const noexcept;

		template<>
		std::optional<float> TryGet<float>() const noexcept;

		template<>
		std::optional<double> TryGet<double>() const noexcept;

		template<>
		std::optional<fig::string> TryGet<fig::string>() const noexcept;

		template<>
		std::optional<fig::path> TryGet<fig::path>() const noexcept;

		template<>
		std::optional<fig::bytes> TryGet<fig::bytes>() const noexcept;

		template<>
		std::optional<fig::uuid> TryGet<fig::uuid>() const noexcept;

		template<>
		std::optional<fig::string_list> TryGet<fig::string_list>() const noexcept;

		template<is_number_range T>
		[[nodiscard]] std::optional<T> TryGet() const noexcept;

		template<typename T>
			requires (std::signed_integral<T> and not std::same_as<T, bool>)
		std::optional<T> TryGet() const noexcept;

		template<typename T>
			requires (std::unsigned_integral<T> and not std::same_as<T, bool>)
		std::optional<T> TryGet() const noexcept;

		template<typename T>
		T Get(const T& default_value) const noexcept
		{
			return TryGet<T>().value_or(default_value);
		}

	private:
		const tinyxml2::XMLAttribute* _pAttrib;
	};

	class XmlReaderElement
	{
		friend class XmlReader;
		XmlReaderElement() = delete;
		XmlReaderElement(const tinyxml2::XMLElement* pElement, const tinyxml2::XMLElement* pRoot) noexcept;

	public:
		fig::string GetName() const noexcept;

		inline bool IsOk() const { return (bool)_pRoot and (bool)_pElement; }
		bool Contains(fig::string_view attributeKey) const noexcept;

		std::optional<XmlReaderElement> GetFirstElement() const noexcept;
		std::optional<XmlReaderElement> GetFirstElement(const fig::string& name) const noexcept;
		std::optional<XmlReaderElement> GetNextSiblingAny() const noexcept;
		std::optional<XmlReaderElement> GetNextSibling() const noexcept;
		std::optional<XmlReaderElement> GetNextSibling(const fig::string& name) const noexcept;

		template<typename T>
		std::optional<T> TryGetValue() const = delete;

		template<>
		[[nodiscard]] std::optional<bool> TryGetValue<bool>() const noexcept;

		template<>
		[[nodiscard]] std::optional<float> TryGetValue<float>() const noexcept;

		template<>
		[[nodiscard]] std::optional<double> TryGetValue<double>() const noexcept;

		template<>
		[[nodiscard]] std::optional<fig::string> TryGetValue<fig::string>() const noexcept;

		template<>
		[[nodiscard]] std::optional<fig::path> TryGetValue<fig::path>() const noexcept;

		template<>
		[[nodiscard]] std::optional<fig::bytes> TryGetValue<fig::bytes>() const noexcept;

		template<>
		[[nodiscard]] std::optional<fig::uuid> TryGetValue<fig::uuid>() const noexcept;

		template<>
		[[nodiscard]] std::optional<fig::string_list> TryGetValue<fig::string_list>() const noexcept;

		template<is_number_range T>
		[[nodiscard]] std::optional<T> TryGetValue() const noexcept;

		template<typename T>
			requires (std::unsigned_integral<T> and not std::same_as<T, bool>)
		[[nodiscard]] std::optional<T> TryGetValue() const noexcept;

		template<typename T>
			requires (std::signed_integral<T> and not std::same_as<T, bool>)
		[[nodiscard]] std::optional<T> TryGetValue() const noexcept;

		template<typename T>
		[[nodiscard]] std::optional<T> TryGetElement(const fig::string& name) const noexcept
		{
			if (auto elem = GetFirstElement(name))
			{
				auto value = elem.value().TryGetValue<T>();
				return value;
			}
			return std::nullopt;
		}

		template<typename T>
		[[nodiscard]] T GetElement(const fig::string& name, const T& default_value) const noexcept
		{
			return TryGetElement<T>(name).value_or(default_value);
		}

		// Attribute accessor
		[[nodiscard]] XmlReaderAttribute operator[] (const std::string& key) const noexcept;

	private:
		const tinyxml2::XMLElement* _pRoot {};
		const tinyxml2::XMLElement* _pElement {};
	};

	class XmlReader
	{
		XmlReader() = delete;
	public:
		XmlReader(const fig::path& path);
		XmlReader(const fig::path& path, const fig::string& root);
		XmlReader(const fig::string& document);
		XmlReader(fig::string_view document);
		~XmlReader();

		inline bool IsOk() const noexcept { return (bool)_pDoc and (bool)_pRoot; }

		XmlReaderElement GetRoot() const noexcept;
		[[nodiscard]] std::optional<XmlReaderElement> GetFirstElement(const fig::string& name) const noexcept;

	private:
		tinyxml2::XMLDocument* _pDoc {};
		tinyxml2::XMLElement* _pRoot {};
	};
}
#endif
