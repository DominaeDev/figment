#ifndef XML_WRITER_H__
#define XML_WRITER_H__
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
	class XmlWriterAttribute
	{
		friend class XmlWriterElement;
		XmlWriterAttribute() = delete;
		XmlWriterAttribute(const fig::string& name, tinyxml2::XMLElement* pParent) noexcept;

	public:
		template<typename T>
		XmlWriterAttribute& operator=(const T& value) noexcept
		{
			Set<T>(value);
			return *this;
		}

		template<typename T> void Set(const T& value) = delete;
		template<> void Set<bool>(const bool& value) noexcept;
		template<> void Set<float>(const float& value) noexcept;
		template<> void Set<double>(const double& value) noexcept;
		template<> void Set<fig::string>(const fig::string& value) noexcept;
		template<> void Set<fig::path>(const fig::path& value) noexcept;
		template<> void Set<fig::byte_span>(const fig::byte_span& value) noexcept;
		template<> void Set<fig::uuid>(const fig::uuid& value) noexcept;
		template<> void Set<fig::string_span>(const fig::string_span& values) noexcept;

		template<typename T> requires (std::signed_integral<T> and not std::same_as<T, bool>)
		void Set(const T& value) noexcept;
		template<typename T> requires (std::unsigned_integral<T> and not std::same_as<T, bool>)
		void Set(const T& value) noexcept;
		
		void Set(const fig::string& name, const fig::string_like auto& value) noexcept
		{
			Set(name, fig::string { value });
		}

	private:
		fig::string _name;
		tinyxml2::XMLElement* _pParent;
	};

	class XmlWriterElement
	{
		friend class XmlWriter;
		XmlWriterElement() = delete;
		XmlWriterElement(tinyxml2::XMLElement* pElement) noexcept;

	public:
		template<typename T>
		void SetAttribute(const fig::string& name, const T& value) noexcept { operator[](name).Set<T>(value); }

		template<typename T> void SetValue(const T& value) = delete;
		template<> void SetValue<bool>(const bool& value) noexcept;
		template<> void SetValue<float>(const float& value) noexcept;
		template<> void SetValue<double>(const double& value) noexcept;
		template<> void SetValue<fig::string>(const fig::string& value) noexcept;
		template<> void SetValue<fig::path>(const fig::path& value) noexcept;
		template<> void SetValue<fig::byte_span>(const fig::byte_span& value) noexcept;
		template<> void SetValue<fig::uuid>(const fig::uuid& value) noexcept;
		template<> void SetValue<fig::string_span>(const fig::string_span& values) noexcept;

		template<typename T> requires (std::integral<T> and not std::same_as<T, bool>)
		void SetValue(const T& value) noexcept;

		void SetValue(const fig::string& name, const fig::string_like auto& value) noexcept
		{
			SetValue(name, fig::string { value });
		}

		template<std::ranges::range TRange>
			requires std::constructible_from<fig::string, std::ranges::range_value_t<TRange>>
		void SetValue(const fig::string& name, const TRange& value) noexcept
		{
			SetValue(std::span { value.cbegin(), value.cend() });
		}

		template <typename T>
		void SetElementValue(const fig::string& name, const T& value) noexcept
		{
			AddChild(name).SetValue<T>(value);
		}

		void SetElementValue(const fig::string& name, const fig::string_like auto& value) noexcept
		{
			AddChild(name).SetValue(fig::string { value });
		}

		template<std::ranges::range TRange>
			requires std::constructible_from<fig::string, std::ranges::range_value_t<TRange>>
		void SetElementValue(const fig::string& name, const TRange& value) noexcept
		{
			AddChild(name).SetValue(std::span { value.cbegin(), value.cend() });
		}

		XmlWriterAttribute operator[] (const std::string& key) noexcept;

		XmlWriterElement AddChild(const fig::string& name) noexcept;
	private:
		void DeleteValue();

	private:
		tinyxml2::XMLElement* _pElement {};
	};

	class XmlWriter
	{
		XmlWriter() = delete;
	public:
		XmlWriter(const fig::string& root);
		~XmlWriter();

		XmlWriterElement GetRoot() noexcept;
		XmlWriterElement AddChild(const fig::string& name) noexcept;

		bool WriteToFile(const fig::path& filename) const;
		void WriteToMemory(fig::bytes& buffer) const;

	private:
		tinyxml2::XMLDocument* _pDoc {};
		tinyxml2::XMLElement* _pRoot {};
	};
}
#endif
