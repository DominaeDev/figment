#include <pch.h>
#include "io/XmlWriter.h"
#include <tinyxml2.h>
#include <limits>
#include <format>

using namespace tinyxml2;

namespace fig::data
{
	XmlWriterAttribute::XmlWriterAttribute(const fig::string& name, fig::observer_ptr<tinyxml2::XMLElement> pParent) noexcept :
		_name { name },
		_pParent { pParent }
	{}

	template<>
	void XmlWriterAttribute::Set<float>(const float& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), std::format("{:g}", value).c_str());
	}

	template<>
	void XmlWriterAttribute::Set<double>(const double& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), std::format("{:g}", value).c_str());
	}

	template<>
	void XmlWriterAttribute::Set<fig::string>(const fig::string& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), value.c_str());
	}

	template<>
	void XmlWriterAttribute::Set<fig::path>(const fig::path& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), value.u8string().c_str());
	}

	template<>
	void XmlWriterAttribute::Set<fig::byte_span>(const fig::byte_span& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), Base64Encode(value).c_str());
	}

	template<>
	void XmlWriterAttribute::Set<fig::uuid>(const fig::uuid& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), value.to_str().c_str());
	}

	template<>
	void XmlWriterAttribute::Set<fig::gui::Color>(const fig::gui::Color& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), value.ToString().c_str());
	}

	template<>
	void XmlWriterAttribute::Set<fig::string_span>(const fig::string_span& values) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), encode_csv(values).c_str());
	}

	template<>
	void XmlWriterAttribute::Set<fig::handle>(const fig::handle& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), value.c_str());
	}

	template<>
	void XmlWriterAttribute::Set<bool>(const bool& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), value);
	}

	template<typename T>
		requires (std::signed_integral<T> and not std::same_as<T, bool>)
	void XmlWriterAttribute::Set(const T& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), static_cast<int64_t>(value));
	}

	template<typename T>
		requires (std::unsigned_integral<T> and not std::same_as<T, bool>)
	void XmlWriterAttribute::Set(const T& value) noexcept
	{
		_pParent->SetAttribute(_name.c_str(), static_cast<uint64_t>(value));
	}

	XmlWriterElement::XmlWriterElement(fig::observer_ptr<tinyxml2::XMLElement> pElement) noexcept :
		_pElement { pElement }
	{
	}

	void XmlWriterElement::DeleteValue()
	{
		_pElement->DeleteChildren();
	}

	template<> 
	void XmlWriterElement::SetValue<bool>(const bool& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(value ? "true" : "false");
	}

	template<typename T> requires (std::integral<T> and not std::same_as<T, bool>)
	void XmlWriterElement::SetValue(const T& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(std::format("{}", value).c_str());
	}

	template<>
	void XmlWriterElement::SetValue<float>(const float& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(std::format("{:g}", value).c_str());
	}

	template<>
	void XmlWriterElement::SetValue<double>(const double& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(std::format("{:g}", value).c_str());
	}

	template<>
	void XmlWriterElement::SetValue<fig::string>(const fig::string& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(value.c_str());
	}

	template<>
	void XmlWriterElement::SetValue<fig::path>(const fig::path& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(value.u8string().c_str());
	}

	template<>
	void XmlWriterElement::SetValue<fig::byte_span>(const fig::byte_span& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(Base64Encode(value).c_str());
	}

	template<>
	void XmlWriterElement::SetValue<fig::uuid>(const fig::uuid& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(value.to_str().c_str());
	}

	template<>
	void XmlWriterElement::SetValue<fig::gui::Color>(const fig::gui::Color& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(value.ToString().c_str());
	}

	template<>
	void XmlWriterElement::SetValue<fig::string_span>(const fig::string_span& values) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(encode_csv(values).c_str());
	}

	template<>
	void XmlWriterElement::SetValue<fig::handle>(const fig::handle& value) noexcept
	{
		DeleteValue();
		_pElement->InsertNewText(value.c_str());
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
		_pDoc = std::make_unique<XMLDocument>();
		auto pDecl = _pDoc->NewDeclaration(nullptr);
		_pDoc->InsertFirstChild(pDecl);

		_pRoot = _pDoc->NewElement(root.c_str());
		_pDoc->InsertEndChild(_pRoot);
	}

	XmlWriter::~XmlWriter()
	{
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

	bool XmlWriter::WriteToFile(const fig::path& path) const
	{
		return _pDoc->SaveFile(path.u8string().c_str()) == XML_SUCCESS;
	}

	void XmlWriter::WriteToMemory(fig::bytes& buffer) const
	{
		XMLPrinter printer;
		_pDoc->Print(&printer);
		buffer.resize(printer.CStrSize());
		std::memcpy(buffer.data(), printer.CStr(), buffer.size());
	}

	// Explicit template instantiation

	template void XmlWriterAttribute::Set<bool>(const bool&) noexcept;
	template void XmlWriterAttribute::Set<uint8_t>(const uint8_t&) noexcept;
	template void XmlWriterAttribute::Set<uint16_t>(const uint16_t&) noexcept;
	template void XmlWriterAttribute::Set<uint32_t>(const uint32_t&) noexcept;
	template void XmlWriterAttribute::Set<uint64_t>(const uint64_t&) noexcept;
	template void XmlWriterAttribute::Set<int8_t>(const int8_t&) noexcept;
	template void XmlWriterAttribute::Set<int16_t>(const int16_t&) noexcept;
	template void XmlWriterAttribute::Set<int32_t>(const int32_t&) noexcept;
	template void XmlWriterAttribute::Set<int64_t>(const int64_t&) noexcept;
	template void XmlWriterElement::SetValue<uint8_t>(const uint8_t& value) noexcept;
	template void XmlWriterElement::SetValue<uint16_t>(const uint16_t& value) noexcept;
	template void XmlWriterElement::SetValue<uint32_t>(const uint32_t& value) noexcept;
	template void XmlWriterElement::SetValue<uint64_t>(const uint64_t& value) noexcept;
	template void XmlWriterElement::SetValue<int8_t>(const int8_t& value) noexcept;
	template void XmlWriterElement::SetValue<int16_t>(const int16_t& value) noexcept;
	template void XmlWriterElement::SetValue<int32_t>(const int32_t& value) noexcept;
	template void XmlWriterElement::SetValue<int64_t>(const int64_t& value) noexcept;
}