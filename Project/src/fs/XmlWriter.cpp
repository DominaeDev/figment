#include <pch.h>
#include "fs/Xml.h"
#include "util/Common.h"
#include <tinyxml2.h>
#include <limits>
#include <format>

using namespace tinyxml2;
using namespace fig::util;

namespace fig::io
{
	XmlWriterAttribute::XmlWriterAttribute(const fig::string& name, tinyxml2::XMLElement* pParent) noexcept :
		_name { name },
		_pParent { pParent }
	{}

	XmlWriterAttribute& XmlWriterAttribute::operator=(bool value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), value);
		return *this;
	}

	XmlWriterAttribute& XmlWriterAttribute::operator=(int32_t value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), value);
		return *this;
	}

	XmlWriterAttribute& XmlWriterAttribute::operator=(float value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), std::format("{:g}", value).c_str());
		return *this;
	}

	XmlWriterAttribute& XmlWriterAttribute::operator=(fig::string value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), value.c_str());
		return *this;
	}

	XmlWriterAttribute& XmlWriterAttribute::operator=(const fig::byte_span& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), util::Base64Encode(value).c_str());
		return *this;
	}

	XmlWriterAttribute& XmlWriterAttribute::operator=(const fig::uuid& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), value.str().c_str());
		return *this;
	}

	XmlWriterElement::XmlWriterElement(tinyxml2::XMLElement* pElement) noexcept :
		_pElement { pElement }
	{
	}

	void XmlWriterElement::DeleteValue()
	{
		_pElement->DeleteChildren();
	}

	void XmlWriterElement::SetValue(bool value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(value ? "true" : "false");
	}

	void XmlWriterElement::SetValue(int32_t value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(std::format("{}", value).c_str());
	}

	void XmlWriterElement::SetValue(float value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(std::format("{:g}", value).c_str());
	}

	void XmlWriterElement::SetValue(const fig::string& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(value.c_str());
	}

	void XmlWriterElement::SetValue(const fig::byte_span& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(util::Base64Encode(value).c_str());
	}

	void XmlWriterElement::SetValue(const fig::uuid& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(value.str().c_str());
	}

	void XmlWriterElement::SetValue(std::span<const fig::string> values) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(encode_csv(values).c_str());
	}

	void XmlWriterElement::SetAttribute(const fig::string& name, bool value) noexcept
	{
		_pElement->SetAttribute(name.c_str(), value);
	}

	void XmlWriterElement::SetAttribute(const fig::string& name, int32_t value) noexcept
	{
		_pElement->SetAttribute(name.c_str(), value);
	}

	void XmlWriterElement::SetAttribute(const fig::string& name, float value) noexcept
	{
		_pElement->SetAttribute(name.c_str(), std::format("{:g}", value).c_str());
	}

	void XmlWriterElement::SetAttribute(const fig::string& name, const fig::string& value) noexcept
	{
		_pElement->SetAttribute(name.c_str(), value.c_str());
	}

	void XmlWriterElement::SetAttribute(const fig::string& name, const fig::byte_span& value) noexcept
	{
		_pElement->SetAttribute(name.c_str(), util::Base64Encode(value).c_str());
	}

	void XmlWriterElement::SetAttribute(const fig::string& name, const fig::uuid& value) noexcept
	{
		_pElement->SetAttribute(name.c_str(), value.str().c_str());
	}

	void XmlWriterElement::SetAttribute(const fig::string& name, std::span<const fig::string> values) noexcept
	{
		_pElement->SetAttribute(name.c_str(), encode_csv(values).c_str());
	}

	void XmlWriterElement::SetElementValue(const fig::string& name, bool value) noexcept
	{
		AddChild(name).SetValue(value);
	}

	void XmlWriterElement::SetElementValue(const fig::string& name, int32_t value) noexcept
	{
		AddChild(name).SetValue(value);
	}

	void XmlWriterElement::SetElementValue(const fig::string& name, float value) noexcept
	{
		AddChild(name).SetValue(value);
	}

	void XmlWriterElement::SetElementValue(const fig::string& name, const fig::string& value) noexcept
	{
		AddChild(name).SetValue(value);
	}

	void XmlWriterElement::SetElementValue(const fig::string& name, const fig::byte_span& value) noexcept
	{
		AddChild(name).SetValue(value);
	}

	void XmlWriterElement::SetElementValue(const fig::string& name, const fig::uuid& value) noexcept
	{
		AddChild(name).SetValue(value);
	}

	void XmlWriterElement::SetElementValue(const fig::string& name, std::span<const fig::string> values) noexcept
	{
		AddChild(name).SetValue(encode_csv(values));
	}

	XmlWriterElement XmlWriterElement::AddChild(const fig::string& name) noexcept
	{
		auto pElement = _pElement->InsertNewChildElement(name.c_str());
		return XmlWriterElement(pElement);
	}

	XmlWriterAttribute XmlWriterElement::operator[] (const std::string& key) noexcept
	{
		return XmlWriterAttribute(key, _pElement);
	}

	XmlWriter::XmlWriter(const fig::string& root)
	{
		_pDoc = new XMLDocument();
		auto pDecl = _pDoc->NewDeclaration(nullptr);
		_pDoc->InsertFirstChild(pDecl);

		_pRoot = _pDoc->NewElement(root.c_str());
		_pDoc->InsertEndChild(_pRoot);
	}

	XmlWriter::~XmlWriter()
	{
		delete _pDoc;
	}

	XmlWriterElement XmlWriter::GetRoot() noexcept
	{
		return XmlWriterElement(_pRoot);
	}

	XmlWriterElement XmlWriter::AddChild(const fig::string& name) noexcept
	{
		auto pElement = _pRoot->InsertNewChildElement(name.c_str());
		return XmlWriterElement(pElement);
	}

	bool XmlWriter::Save(const fig::path& path) const
	{
		return _pDoc->SaveFile(path.u8string().c_str()) == XML_SUCCESS;
	}

	void XmlWriter::SaveToMemory(fig::bytes& buffer) const
	{
		XMLPrinter printer;
		_pDoc->Print(&printer);
		buffer.resize(printer.CStrSize());
		std::memcpy(buffer.data(), printer.CStr(), buffer.size());
	}
}