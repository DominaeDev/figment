#include <pch.h>

#include "fs/XmlReader.h"
#include "util/Common.h"
#include <tinyxml2.h>

using namespace tinyxml2;

namespace fig::io
{
	XmlReaderAttribute::XmlReaderAttribute(const tinyxml2::XMLAttribute* pAttribute) noexcept :
		_pAttrib { pAttribute }
	{
	}

	template<>
	std::optional<bool> XmlReaderAttribute::TryGet<bool>() const noexcept
	{
		if (_pAttrib)
		{
			bool value;
			if (_pAttrib->QueryBoolValue(&value) == XML_SUCCESS)
				return std::make_optional(value);
		}
		return std::nullopt;
	}

	template<typename T> requires (std::signed_integral<T> and not std::same_as<T, bool> )
	std::optional<T> XmlReaderAttribute::TryGet() const noexcept
	{
		if (_pAttrib)
		{
			int64_t value;
			if (_pAttrib->QueryInt64Value(&value) == XML_SUCCESS)
			{
				value = std::clamp(value, static_cast<int64_t>(std::numeric_limits<T>::min()), static_cast<int64_t>(std::numeric_limits<T>::max()));
				return std::make_optional(static_cast<T>(value));
			}
		}
		return std::nullopt;
	}

	template<typename T>
		requires (std::unsigned_integral<T> and not std::same_as<T, bool> )
	std::optional<T> XmlReaderAttribute::TryGet() const noexcept
	{
		if (_pAttrib)
		{
			uint64_t value;
			if (_pAttrib->QueryUnsigned64Value(&value) == XML_SUCCESS)
			{
				value = std::clamp(value, static_cast<uint64_t>(std::numeric_limits<T>::min()), static_cast<uint64_t>(std::numeric_limits<T>::max()));
				return std::make_optional(static_cast<T>(value));
			}
		}
		return std::nullopt;
	}

	template<>
	std::optional<float> XmlReaderAttribute::TryGet<float>() const noexcept
	{
		if (_pAttrib)
		{
			float value;
			if (_pAttrib->QueryFloatValue(&value) == XML_SUCCESS)
				return std::make_optional(value);
		}
		return std::nullopt;
	}

	template<>
	std::optional<double> XmlReaderAttribute::TryGet<double>() const noexcept
	{
		if (_pAttrib)
		{
			double value;
			if (_pAttrib->QueryDoubleValue(&value) == XML_SUCCESS)
				return std::make_optional(value);
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::string> XmlReaderAttribute::TryGet<fig::string>() const noexcept
	{
		if (_pAttrib)
		{
			const char* value = _pAttrib->Value();
			return value ? std::make_optional(fig::string(value)) : std::nullopt;
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::path> XmlReaderAttribute::TryGet<fig::path>() const noexcept
	{
		if (_pAttrib)
		{
			const char* value = _pAttrib->Value();
			return value ? std::make_optional(fig::path(value)) : std::nullopt;
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::bytes> XmlReaderAttribute::TryGet<fig::bytes>() const noexcept
	{
		if (_pAttrib)
		{
			const char* value = _pAttrib->Value();
			return value ? std::make_optional(util::Base64Decode(value)) : std::nullopt;
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::uuid> XmlReaderAttribute::TryGet<fig::uuid>() const noexcept
	{
		if (_pAttrib)
		{
			const char* value = _pAttrib->Value();
			if (value)
			{
				fig::uuid uuid = fig::uuid::from_str(value);
				return not uuid.empty() ? std::make_optional(uuid) : std::nullopt;
			}
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::string_list> XmlReaderAttribute::TryGet<fig::string_list>() const noexcept
	{
		if (auto str = TryGet<fig::string>(); str.has_value())
			return std::make_optional(fig::util::decode_csv(str.value()));
		return std::nullopt;
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

	bool XmlReaderElement::Contains(fig::string_view attributeKey) const noexcept
	{
		if (_pElement)
			return _pElement->FindAttribute(fig::string(attributeKey).c_str()) != nullptr;
		return false;
	}

	template<>
	std::optional<bool> XmlReaderElement::TryGetValue<bool>() const noexcept
	{
		bool value;
		if (_pElement->QueryBoolText(&value) == XML_SUCCESS)
			return std::make_optional(value);
		return std::nullopt;
	}

	template<typename T> requires (std::signed_integral<T> and not std::same_as<T, bool>)
		std::optional<T> XmlReaderElement::TryGetValue() const noexcept
	{
		int64_t value;
		if (_pElement->QueryInt64Text(&value) == XML_SUCCESS)
		{
			value = std::clamp(value, static_cast<int64_t>(std::numeric_limits<T>::min()), static_cast<int64_t>(std::numeric_limits<T>::max()));
			return std::make_optional(static_cast<T>(value));
		}
		return std::nullopt;
	}

	template<typename T>
		requires (std::unsigned_integral<T> and not std::same_as<T, bool>)
	std::optional<T> XmlReaderElement::TryGetValue() const noexcept
	{
		uint64_t value;
		if (_pElement->QueryUnsigned64Text(&value) == XML_SUCCESS)
		{
			value = std::clamp(value, static_cast<uint64_t>(std::numeric_limits<T>::min()), static_cast<uint64_t>(std::numeric_limits<T>::max()));
			return std::make_optional(static_cast<T>(value));
		}
		return std::nullopt;
	}

	template<>
	std::optional<float> XmlReaderElement::TryGetValue<float>() const noexcept
	{
		float value;
		if (_pElement->QueryFloatText(&value) == XML_SUCCESS)
			return std::make_optional(value);
		return std::nullopt;
	}

	template<>
	std::optional<double> XmlReaderElement::TryGetValue<double>() const noexcept
	{
		double value;
		if (_pElement->QueryDoubleText(&value) == XML_SUCCESS)
			return std::make_optional(value);
		return std::nullopt;
	}

	template<>
	std::optional<fig::string> XmlReaderElement::TryGetValue<fig::string>() const noexcept
	{
		//! @todo: Resolve mix of CDATA, XMLText, and XMLComment.
		const char* value = _pElement->GetText();
		return value ? std::make_optional(fig::string(value)) : std::nullopt;
	}

	template<>
	std::optional<fig::path> XmlReaderElement::TryGetValue<fig::path>() const noexcept
	{
		//! @todo: Resolve mix of CDATA, XMLText, and XMLComment.
		const char* value = _pElement->GetText();
		return value ? std::make_optional(fig::path(value)) : std::nullopt;
	}

	template<>
	std::optional<fig::bytes> XmlReaderElement::TryGetValue<fig::bytes>() const noexcept
	{
		const char* value = _pElement->GetText();
		return value ? std::make_optional(util::Base64Decode(value)) : std::nullopt;
	}

	template<>
	std::optional<fig::uuid> XmlReaderElement::TryGetValue<fig::uuid>() const noexcept
	{
		const char* value = _pElement->GetText();
		if (value)
		{
			fig::uuid uuid = fig::uuid::from_str(value);
			return not uuid.empty() ? std::make_optional(uuid) : std::nullopt;
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::string_list> XmlReaderElement::TryGetValue<fig::string_list>() const noexcept
	{
		if (auto str = TryGetValue<fig::string>(); str.has_value())
			return std::make_optional(fig::util::decode_csv(str.value()));
		return std::nullopt;
	}

	XmlReaderAttribute XmlReaderElement::operator[] (const std::string& key) const noexcept
	{
		auto pAttrib = _pElement->FindAttribute(key.c_str());
		return XmlReaderAttribute(pAttrib);
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

	XmlReader::XmlReader(fig::string_view document)
	{
		_pDoc = new XMLDocument();
		if (_pDoc->Parse((char*)document.data(), document.size()) != XML_SUCCESS)
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

	// Explicit template instantiations
	template std::optional<uint8_t> XmlReaderAttribute::TryGet<uint8_t>() const noexcept;
	template std::optional<uint16_t> XmlReaderAttribute::TryGet<uint16_t>() const noexcept;
	template std::optional<uint32_t> XmlReaderAttribute::TryGet<uint32_t>() const noexcept;
	template std::optional<uint64_t> XmlReaderAttribute::TryGet<uint64_t>() const noexcept;
	template std::optional<int8_t> XmlReaderAttribute::TryGet<int8_t>() const noexcept;
	template std::optional<int16_t> XmlReaderAttribute::TryGet<int16_t>() const noexcept;
	template std::optional<int32_t> XmlReaderAttribute::TryGet<int32_t>() const noexcept;
	template std::optional<int64_t> XmlReaderAttribute::TryGet<int64_t>() const noexcept;
	template std::optional<uint8_t> XmlReaderElement::TryGetValue<uint8_t>() const noexcept;
	template std::optional<uint16_t> XmlReaderElement::TryGetValue<uint16_t>() const noexcept;
	template std::optional<uint32_t> XmlReaderElement::TryGetValue<uint32_t>() const noexcept;
	template std::optional<uint64_t> XmlReaderElement::TryGetValue<uint64_t>() const noexcept;
	template std::optional<int8_t> XmlReaderElement::TryGetValue<int8_t>() const noexcept;
	template std::optional<int16_t> XmlReaderElement::TryGetValue<int16_t>() const noexcept;
	template std::optional<int32_t> XmlReaderElement::TryGetValue<int32_t>() const noexcept;
	template std::optional<int64_t> XmlReaderElement::TryGetValue<int64_t>() const noexcept;

}