#include <pch.h>
#include "util/Common.h"
#include "util/Xml.h"

#include <tinyxml2.h>
#include <filesystem>
#include <limits>

using namespace tinyxml2;

namespace fig
{
	XmlReaderAttribute::XmlReaderAttribute(const tinyxml2::XMLAttribute* pAttribute) noexcept :
		_pAttrib { pAttribute }
	{
	}

	bool XmlReaderAttribute::IsOk() const noexcept
	{
		return (bool)_pAttrib;
	}

	std::optional<fig::string> XmlReaderAttribute::AsString() const noexcept
	{
		if (_pAttrib)
		{
			const char* value = _pAttrib->Value();
			return value ? std::make_optional(fig::string(value)) : std::nullopt;
		}
		return std::nullopt;
	}

	std::optional<int32_t> XmlReaderAttribute::AsInt() const noexcept
	{
		if (_pAttrib)
		{
			int32_t value;
			if (_pAttrib->QueryIntValue(&value) == XML_SUCCESS)
				return std::make_optional(value);
		}
		return std::nullopt;
	}

	std::optional<float> XmlReaderAttribute::AsFloat() const noexcept
	{
		if (_pAttrib)
		{
			float value;
			if (_pAttrib->QueryFloatValue(&value) == XML_SUCCESS)
				return std::make_optional(value);
		}
		return std::nullopt;
	}

	std::optional<fig::bytes> XmlReaderAttribute::AsBytes() const noexcept
	{
		if (_pAttrib)
		{
			const char* value = _pAttrib->Value();
			return value ? std::make_optional(common_util::Base64Decode(value)) : std::nullopt;
		}
		return std::nullopt;
	}

	inline fig::string XmlReaderAttribute::AsString(const fig::string& default_value) const noexcept
	{
		return AsString().value_or(default_value);
	}

	inline int32_t XmlReaderAttribute::AsInt(int32_t default_value) const noexcept
	{
		return AsInt().value_or(default_value);
	}

	inline float XmlReaderAttribute::AsFloat(float default_value) const noexcept
	{
		return AsFloat().value_or(default_value);
	}

	XmlReaderElement::XmlReaderElement(const tinyxml2::XMLElement* pElement, const tinyxml2::XMLElement* pRoot) noexcept :
		_pElement { pElement },
		_pRoot { pRoot }
	{}

	std::optional<XmlReaderElement> XmlReaderElement::GetFirstElement(const fig::string& name) const noexcept
	{
		auto pElement = _pElement->FirstChildElement(name.c_str());
		return pElement ? std::make_optional<XmlReaderElement>({ pElement, _pRoot }) : std::nullopt;
	}

	std::optional<XmlReaderElement> XmlReaderElement::GetNextSiblingAny() const noexcept
	{
		auto pElement = _pElement->NextSiblingElement();
		return pElement ? std::make_optional<XmlReaderElement>({ pElement, _pRoot }) : std::nullopt;
	}

	std::optional<XmlReaderElement> XmlReaderElement::GetNextSibling() const noexcept
	{
		auto pElement = _pElement->NextSiblingElement(_pElement->Name());
		return pElement ? std::make_optional<XmlReaderElement>({ pElement, _pRoot }) : std::nullopt;
	}

	std::optional<XmlReaderElement> XmlReaderElement::GetNextSibling(const fig::string& name) const noexcept
	{
		auto pElement = _pElement->NextSiblingElement(name.c_str());
		return pElement ? std::make_optional<XmlReaderElement>({ pElement, _pRoot }) : std::nullopt;
	}

	std::optional<fig::string> XmlReaderElement::GetText() const noexcept
	{
		const char* value = _pElement->GetText();
		return value ? std::make_optional(fig::string(value)) : std::nullopt;
	}

	std::optional<int32_t> XmlReaderElement::GetIntText() const noexcept
	{
		int32_t value = _pElement->IntText(std::numeric_limits<int32_t>::min());
		return value != std::numeric_limits<int32_t>::min() ? std::make_optional(value) : std::nullopt;
	}

	std::optional<float> XmlReaderElement::GetFloatText() const noexcept
	{
		float value = _pElement->FloatText(std::numeric_limits<float>::min());
		return value != std::numeric_limits<float>::min() ? std::make_optional(value) : std::nullopt;
	}

	std::optional<fig::bytes> XmlReaderElement::GetBytesText() const noexcept
	{
		const char* value = _pElement->GetText();
		return value ? std::make_optional(common_util::Base64Decode(value)) : std::nullopt;
	}

	inline fig::string XmlReaderElement::GetText(const fig::string& default_value) const noexcept
	{
		return GetText().value_or(default_value);
	}

	inline int32_t XmlReaderElement::GetIntText(int32_t default_value) const noexcept
	{
		return GetIntText().value_or(default_value);
	}

	inline float XmlReaderElement::GetFloatText(float default_value) const noexcept
	{
		return GetFloatText().value_or(default_value);
	}

	XmlReaderAttribute XmlReaderElement::operator[] (const std::string& key) const noexcept
	{
		auto pAttrib = _pElement->FindAttribute(key.c_str());
		return XmlReaderAttribute(pAttrib);
	}

	std::optional<fig::string> XmlReaderElement::GetElementText(const fig::string& name) const noexcept
	{
		if (auto elem = GetFirstElement(name))
			return elem.value().GetText();
		return std::nullopt;
	}

	std::optional<int32_t> XmlReaderElement::GetElementInt(const fig::string& name) const noexcept
	{
		if (auto elem = GetFirstElement(name))
			return elem.value().GetIntText();
		return std::nullopt;
	}

	std::optional<float> XmlReaderElement::GetElementFloat(const fig::string& name) const noexcept
	{
		if (auto elem = GetFirstElement(name))
			return elem.value().GetFloatText();
		return std::nullopt;
	}

	std::optional<fig::bytes> XmlReaderElement::GetElementBytes(const fig::string& name) const noexcept
	{
		if (auto elem = GetFirstElement(name))
		{
			auto value = elem.value().GetText();
			if (value.has_value())
				return std::make_optional(common_util::Base64Decode(value.value()));
		}
		return std::nullopt;
	}

	XmlReader::XmlReader(const fig::string& filename)
	{
		auto const path = std::filesystem::path(filename.c_str());
		
		_pDoc = std::make_unique<XMLDocument>();
		if (_pDoc->LoadFile(path.generic_u8string().c_str()) != XML_SUCCESS)
		{
			// Error
			_pDoc.reset();
			_pRoot = nullptr;
			return;
		}
		_pRoot = _pDoc->RootElement();
	}

	XmlReader::XmlReader(const fig::string& filename, const fig::string& root)
	{
		auto const path = std::filesystem::path(filename.c_str());
		
		_pDoc = std::make_unique<XMLDocument>();
		if (_pDoc->LoadFile(path.generic_u8string().c_str()) != XML_SUCCESS 
			or std::strcmp(_pDoc->RootElement()->Name(), root.c_str()) != 0)
		{
			// Error
			_pDoc.reset();
			_pRoot = nullptr;
			return;
		}
		_pRoot = _pDoc->RootElement();
	}

	bool XmlReader::IsOk() const noexcept
	{
		return (bool)_pDoc and (bool)_pRoot;
	}

	std::optional<XmlReaderElement> XmlReader::GetRootElement() const noexcept
	{
		return IsOk() ? std::make_optional<XmlReaderElement>({ _pRoot, _pRoot }) : std::nullopt;
	}

	std::optional<XmlReaderElement> XmlReader::GetFirstElement(const fig::string& name) const noexcept
	{
		if (not IsOk())
			return std::nullopt;

		auto pElement = _pRoot->FirstChildElement(name.c_str());
		return pElement ? std::make_optional<XmlReaderElement>({ pElement, _pRoot }) : std::nullopt;
	}
}