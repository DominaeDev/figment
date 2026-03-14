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

namespace fig::io
{
	class XmlReaderAttribute
	{
		friend class XmlReaderElement;
		XmlReaderAttribute() = delete;
		XmlReaderAttribute(const tinyxml2::XMLAttribute* pAttribute) noexcept;

	public:
		bool IsOk() const noexcept;

		std::optional<bool> AsBool() const noexcept;
		std::optional<int32_t> AsInt() const noexcept;
		std::optional<float> AsFloat() const noexcept;
		std::optional<fig::string> AsText() const noexcept;
		std::optional<fig::bytes> AsBytes() const noexcept;
		std::optional<fig::uuid> AsUUID() const noexcept;
		std::optional<std::vector<fig::string>> AsList() const noexcept;

		bool AsBool(bool default_value) const noexcept;
		int32_t AsInt(int32_t default_value) const noexcept;
		float AsFloat(float default_value) const noexcept;
		fig::string AsText(const fig::string& default_value) const noexcept;
		std::vector<fig::string> AsList(const std::vector<fig::string>& default_value) const noexcept;

	private:
		const tinyxml2::XMLAttribute* _pAttrib;
	};

	class XmlReaderElement
	{
		friend class XmlReader;
		XmlReaderElement() = delete;
		XmlReaderElement(const tinyxml2::XMLElement* pElement, const tinyxml2::XMLElement* pRoot) noexcept;

	public:
		std::optional<XmlReaderElement> GetFirstElement() const noexcept;
		std::optional<XmlReaderElement> GetFirstElement(const fig::string& name) const noexcept;
		std::optional<XmlReaderElement> GetNextSiblingAny() const noexcept;
		std::optional<XmlReaderElement> GetNextSibling() const noexcept;
		std::optional<XmlReaderElement> GetNextSibling(const fig::string& name) const noexcept;

		std::optional<bool> GetBool() const noexcept;
		std::optional<int32_t> GetInt() const noexcept;
		std::optional<float> GetFloat() const noexcept;
		std::optional<fig::string> GetText() const noexcept;
		std::optional<fig::bytes> GetBytes() const noexcept;
		std::optional<fig::uuid> GetUUID() const noexcept;
		std::optional<std::vector<fig::string>> GetList() const noexcept;

		fig::string GetName() const noexcept;
		bool GetBool(bool default_value) const noexcept;
		int32_t GetInt(int32_t default_value) const noexcept;
		float GetFloat(float default_value) const noexcept;
		fig::string GetText(const fig::string& default_value) const noexcept;
		std::vector<fig::string> GetList(const std::vector<fig::string>& default_value) const noexcept;

		std::optional<bool> GetElementBool(const fig::string& name) const noexcept;
		std::optional<int32_t> GetElementInt(const fig::string& name) const noexcept;
		std::optional<float> GetElementFloat(const fig::string& name) const noexcept;
		std::optional<fig::string> GetElementText(const fig::string& name) const noexcept;
		std::optional<fig::bytes> GetElementBytes(const fig::string& name) const noexcept;
		std::optional<fig::uuid> GetElementUUID(const fig::string& name) const noexcept;
		std::optional<std::vector<fig::string>> GetElementList(const fig::string& name) const noexcept;

		bool GetElementBool(const fig::string& name, bool default_value) const noexcept;
		int32_t GetElementInt(const fig::string& name, int32_t default_value) const noexcept;
		float GetElementFloat(const fig::string& name, float default_value) const noexcept;
		fig::string GetElementText(const fig::string& name, const fig::string& default_value) const noexcept;
		std::vector<fig::string> GetElementList(const fig::string& name, const std::vector<fig::string>& default_value) const noexcept;

		XmlReaderAttribute operator[] (const std::string& key) const noexcept;

		bool IsOk() const { return (bool)_pRoot and (bool)_pElement; }
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
		~XmlReader();

		bool IsOk() const noexcept;

		XmlReaderElement GetRootElement() const noexcept;
		std::optional<XmlReaderElement> GetFirstElement(const fig::string& name) const noexcept;

	private:
		tinyxml2::XMLDocument* _pDoc {};
		tinyxml2::XMLElement* _pRoot {};
	};

	class XmlWriterAttribute
	{
		friend class XmlWriterElement;
		XmlWriterAttribute() = delete;
		XmlWriterAttribute(const fig::string& name, tinyxml2::XMLElement* pParent) noexcept;

	public:
		XmlWriterAttribute& operator=(bool value) noexcept;
		XmlWriterAttribute& operator=(int32_t value) noexcept;
		XmlWriterAttribute& operator=(float value) noexcept;
		XmlWriterAttribute& operator=(fig::string value) noexcept;
		XmlWriterAttribute& operator=(const fig::byte_span& value) noexcept;
		XmlWriterAttribute& operator=(const fig::uuid& value) noexcept;

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
		void SetValue(bool value) noexcept;
		void SetValue(int32_t value) noexcept;
		void SetValue(float value) noexcept;
		void SetValue(const fig::string& value) noexcept;
		void SetValue(const fig::byte_span& value) noexcept;
		void SetValue(const fig::uuid& value) noexcept;
		void SetValue(std::span<const fig::string> values) noexcept;

		void SetAttribute(const fig::string& name, bool value) noexcept;
		void SetAttribute(const fig::string& name, int32_t value) noexcept;
		void SetAttribute(const fig::string& name, float value) noexcept;
		void SetAttribute(const fig::string& name, const fig::string& value) noexcept;
		void SetAttribute(const fig::string& name, const fig::byte_span& value) noexcept;
		void SetAttribute(const fig::string& name, const fig::uuid& value) noexcept;
		void SetAttribute(const fig::string& name, std::span<const fig::string> values) noexcept;
		void SetAttribute(const fig::string& name, const fig::string_like auto& value) noexcept
		{
			SetAttribute(name, fig::string(value));
		}

		void SetElementValue(const fig::string& name, bool value) noexcept;
		void SetElementValue(const fig::string& name, const fig::string& value) noexcept;
		void SetElementValue(const fig::string& name, int32_t value) noexcept;
		void SetElementValue(const fig::string& name, float value) noexcept;
		void SetElementValue(const fig::string& name, const fig::byte_span& value) noexcept;
		void SetElementValue(const fig::string& name, const fig::uuid& value) noexcept;
		void SetElementValue(const fig::string& name, std::span<const fig::string> values) noexcept;
		void SetElementValue(const fig::string& name, const fig::string_like auto& value) noexcept
		{
			SetElementValue(name, fig::string(value));
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

		bool Save(const fig::path& filename) const;
		void SaveToMemory(fig::bytes& buffer) const;

	private:
		tinyxml2::XMLDocument* _pDoc {};
		tinyxml2::XMLElement* _pRoot {};
	};
}
#endif
