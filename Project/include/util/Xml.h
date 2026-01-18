#ifndef XML_H__
#define XML_H__
#pragma once

#include "Types.h"

namespace tinyxml2
{
	class XMLDocument;
	class XMLElement;
	class XMLAttribute;
}

namespace fig
{
	class XmlReaderAttribute
	{
		friend class XmlReaderElement;
		XmlReaderAttribute() = delete;
		XmlReaderAttribute(const tinyxml2::XMLAttribute* pAttribute) noexcept;

	public:
		bool IsOk() const noexcept;

		std::optional<fig::string> AsString() const noexcept;
		std::optional<int32_t> AsInt() const noexcept;
		std::optional<float> AsFloat() const noexcept;
		std::optional<fig::bytes> AsBytes() const noexcept;

		inline fig::string AsString(const fig::string& default_value) const noexcept;
		inline int32_t AsInt(int32_t default_value) const noexcept;
		inline float AsFloat(float default_value) const noexcept;

	private:
		const tinyxml2::XMLAttribute* _pAttrib;
	};

	class XmlReaderElement
	{
		friend class XmlReader;
		XmlReaderElement() = delete;
		XmlReaderElement(const tinyxml2::XMLElement* pElement, const tinyxml2::XMLElement* pRoot) noexcept;

	public:
		std::optional<XmlReaderElement> GetFirstElement(const fig::string& name) const noexcept;
		std::optional<XmlReaderElement> GetNextSiblingAny() const noexcept;
		std::optional<XmlReaderElement> GetNextSibling() const noexcept;
		std::optional<XmlReaderElement> GetNextSibling(const fig::string& name) const noexcept;

		std::optional<fig::string> GetText() const noexcept;
		std::optional<int32_t> GetIntText() const noexcept;
		std::optional<float> GetFloatText() const noexcept;
		std::optional<fig::bytes> GetBytesText() const noexcept;

		inline fig::string GetText(const fig::string& default_value) const noexcept;
		inline int32_t GetIntText(int32_t default_value) const noexcept;
		inline float GetFloatText(float default_value) const noexcept;

		std::optional<fig::string> GetElementText(const fig::string& name) const noexcept;
		std::optional<int32_t> GetElementInt(const fig::string& name) const noexcept;
		std::optional<float> GetElementFloat(const fig::string& name) const noexcept;
		std::optional<fig::bytes> GetElementBytes(const fig::string& name) const noexcept;

		XmlReaderAttribute operator[] (const std::string& key) const noexcept;

	private:
		const tinyxml2::XMLElement* _pRoot {};
		const tinyxml2::XMLElement* _pElement {};
	};

	class XmlReader
	{
	public:
		XmlReader(const fig::string& filename);
		XmlReader(const fig::string& filename, const fig::string& root);
		bool IsOk() const noexcept;

		std::optional<XmlReaderElement> GetRootElement() const noexcept;
		std::optional<XmlReaderElement> GetFirstElement(const fig::string& name) const noexcept;

	private:
		std::unique_ptr<tinyxml2::XMLDocument> _pDoc {};
		tinyxml2::XMLElement* _pRoot {};
	};
}
#endif
