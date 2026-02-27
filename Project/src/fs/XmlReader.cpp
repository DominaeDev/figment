#include <pch.h>

#include "fs/Xml.h"
#include "util/Common.h"
#include <tinyxml2.h>

using namespace tinyxml2;

namespace fig::fs
{
	XmlReaderAttribute::XmlReaderAttribute(const tinyxml2::XMLAttribute* pAttribute) noexcept :
		_pAttrib { pAttribute }
	{
	}

	bool XmlReaderAttribute::IsOk() const noexcept
	{
		return (bool)_pAttrib;
	}

	std::optional<bool> XmlReaderAttribute::AsBool() const noexcept
	{
		if (_pAttrib)
		{
			bool value;
			if (_pAttrib->QueryBoolValue(&value) == XML_SUCCESS)
				return std::make_optional(value);
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

	std::optional<fig::string> XmlReaderAttribute::AsText() const noexcept
	{
		if (_pAttrib)
		{
			const char* value = _pAttrib->Value();
			return value ? std::make_optional(fig::string(value)) : std::nullopt;
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

	std::optional<fig::uuid> XmlReaderAttribute::AsUUID() const noexcept
	{
		if (_pAttrib)
		{
			const char* value = _pAttrib->Value();
			if (value)
			{
				fig::uuid uuid {};
				uuid.fromStr(value);
				return not uuid.empty() ? std::make_optional(uuid) : std::nullopt;
			}
		}
		return std::nullopt;
	}

	bool XmlReaderAttribute::AsBool(bool default_value) const noexcept
	{
		return AsBool().value_or(default_value);
	}

	int32_t XmlReaderAttribute::AsInt(int32_t default_value) const noexcept
	{
		return AsInt().value_or(default_value);
	}

	float XmlReaderAttribute::AsFloat(float default_value) const noexcept
	{
		return AsFloat().value_or(default_value);
	}

	fig::string XmlReaderAttribute::AsText(const fig::string& default_value) const noexcept
	{
		return AsText().value_or(default_value);
	}

	XmlReaderElement::XmlReaderElement(const tinyxml2::XMLElement* pElement, const tinyxml2::XMLElement* pRoot) noexcept :
		_pElement { pElement },
		_pRoot { pRoot }
	{}

	std::optional<XmlReaderElement> XmlReaderElement::GetFirstElement() const noexcept
	{
		auto pElement = _pElement->FirstChildElement(nullptr);
		return pElement ? std::make_optional<XmlReaderElement>({ pElement, _pRoot }) : std::nullopt;
	}

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

	fig::string XmlReaderElement::GetName() const noexcept
	{
		if (_pElement)
			return fig::string(_pElement->Name());
		return "";
	}

	std::optional<bool> XmlReaderElement::GetBool() const noexcept
	{
		bool value;
		if (_pElement->QueryBoolText(&value) == XML_SUCCESS)
			return std::make_optional(value);
		return std::nullopt;
	}

	std::optional<int32_t> XmlReaderElement::GetInt() const noexcept
	{
		int32_t value;
		if (_pElement->QueryIntText(&value) == XML_SUCCESS)
			return std::make_optional(value);
		return std::nullopt;
	}

	std::optional<float> XmlReaderElement::GetFloat() const noexcept
	{
		float value;
		if (_pElement->QueryFloatText(&value) == XML_SUCCESS)
			return std::make_optional(value);
		return std::nullopt;
	}

	std::optional<fig::string> XmlReaderElement::GetText() const noexcept
	{
		//! @todo: Resolve mix of CDATA, XMLText, and XMLComment.
		const char* value = _pElement->GetText();
		return value ? std::make_optional(fig::string(value)) : std::nullopt;
	}

	std::optional<fig::bytes> XmlReaderElement::GetBytes() const noexcept
	{
		const char* value = _pElement->GetText();
		return value ? std::make_optional(common_util::Base64Decode(value)) : std::nullopt;
	}

	std::optional<fig::uuid> XmlReaderElement::GetUUID() const noexcept
	{
		const char* value = _pElement->GetText();
		if (value)
		{
			fig::uuid uuid {};
			uuid.fromStr(value);
			return not uuid.empty() ? std::make_optional(uuid) : std::nullopt;
		}
		return std::nullopt;
	}

	bool XmlReaderElement::GetBool(bool default_value) const noexcept
	{
		return GetBool().value_or(default_value);
	}

	int32_t XmlReaderElement::GetInt(int32_t default_value) const noexcept
	{
		return GetInt().value_or(default_value);
	}

	float XmlReaderElement::GetFloat(float default_value) const noexcept
	{
		return GetFloat().value_or(default_value);
	}

	fig::string XmlReaderElement::GetText(const fig::string& default_value) const noexcept
	{
		return GetText().value_or(default_value);
	}

	XmlReaderAttribute XmlReaderElement::operator[] (const std::string& key) const noexcept
	{
		auto pAttrib = _pElement->FindAttribute(key.c_str());
		return XmlReaderAttribute(pAttrib);
	}

	std::optional<bool> XmlReaderElement::GetElementBool(const fig::string& name) const noexcept
	{
		if (auto elem = GetFirstElement(name))
			return elem.value().GetBool();
		return std::nullopt;
	}

	std::optional<int32_t> XmlReaderElement::GetElementInt(const fig::string& name) const noexcept
	{
		if (auto elem = GetFirstElement(name))
			return elem.value().GetInt();
		return std::nullopt;
	}

	std::optional<float> XmlReaderElement::GetElementFloat(const fig::string& name) const noexcept
	{
		if (auto elem = GetFirstElement(name))
			return elem.value().GetFloat();
		return std::nullopt;
	}

	std::optional<fig::string> XmlReaderElement::GetElementText(const fig::string& name) const noexcept
	{
		if (auto elem = GetFirstElement(name))
			return elem.value().GetText();
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

	std::optional<fig::uuid> XmlReaderElement::GetElementUUID(const fig::string& name) const noexcept
	{
		if (auto elem = GetFirstElement(name))
		{
			auto value = elem.value().GetText();
			if (value.has_value())
			{
				fig::uuid uuid {};
				uuid.fromStr(value.value().c_str());
				return not uuid.empty() ? std::make_optional(uuid) : std::nullopt;
			}
		}
		return std::nullopt;
	}

	bool XmlReaderElement::GetElementBool(const fig::string& name, bool default_value) const noexcept
	{
		return GetElementBool(name).value_or(default_value);
	}

	int32_t XmlReaderElement::GetElementInt(const fig::string& name, int32_t default_value) const noexcept
	{
		return GetElementInt(name).value_or(default_value);
	}

	float XmlReaderElement::GetElementFloat(const fig::string& name, float default_value) const noexcept
	{
		return GetElementFloat(name).value_or(default_value);
	}

	fig::string XmlReaderElement::GetElementText(const fig::string& name, const fig::string& default_value) const noexcept
	{
		return GetElementText(name).value_or(default_value);
	}

	XmlReader::XmlReader(const fig::path& path)
	{
		_pDoc = new XMLDocument();
		if (_pDoc->LoadFile(path.u8string().c_str()) != XML_SUCCESS)
		{
			// Error
			delete _pDoc;
			_pDoc = nullptr;
			_pRoot = nullptr;
			return;
		}
		_pRoot = _pDoc->RootElement();
	}

	XmlReader::XmlReader(const fig::path& path, const fig::string& root)
	{
		_pDoc = new XMLDocument();
		if (_pDoc->LoadFile(path.u8string().c_str()) != XML_SUCCESS 
			or std::strcmp(_pDoc->RootElement()->Name(), root.c_str()) != 0)
		{
			delete _pDoc;
			_pDoc = nullptr;
			_pRoot = nullptr;
			return;
		}
		_pRoot = _pDoc->RootElement();
	}

	XmlReader::XmlReader(const fig::string& document)
	{
		_pDoc = new XMLDocument();
		if (_pDoc->Parse(document.c_str()) != XML_SUCCESS)
		{
			// Error
			delete _pDoc;
			_pDoc = nullptr;
			_pRoot = nullptr;
			return;
		}
		_pRoot = _pDoc->RootElement();
	}


	XmlReader::~XmlReader()
	{
		delete _pDoc;
	}

	bool XmlReader::IsOk() const noexcept
	{
		return (bool)_pDoc and (bool)_pRoot;
	}

	XmlReaderElement XmlReader::GetRootElement() const noexcept
	{
		return IsOk() ? XmlReaderElement { _pRoot, _pRoot } : XmlReaderElement { nullptr, nullptr };
	}

	std::optional<XmlReaderElement> XmlReader::GetFirstElement(const fig::string& name) const noexcept
	{
		if (not IsOk())
			return std::nullopt;

		auto pElement = _pRoot->FirstChildElement(name.c_str());
		return pElement ? std::make_optional<XmlReaderElement>({ pElement, _pRoot }) : std::nullopt;
	}
}