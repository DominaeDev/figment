#include <pch.h>

#include "io/XmlReader.h"
#include <tinyxml2.h>

using namespace tinyxml2;

namespace fig::data
{
	XmlReaderAttribute::XmlReaderAttribute(fig::observer_ptr<const tinyxml2::XMLAttribute> pAttribute) noexcept :
		_pAttrib { pAttribute }
	{
	}

	fig::string XmlReaderAttribute::GetName() const noexcept
	{
		if (_pAttrib)
			return fig::string(_pAttrib->Name());
		return "";
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
	std::optional<fig::fixed> XmlReaderAttribute::TryGet<fig::fixed>() const noexcept
	{
		if (_pAttrib)
		{
			const char* pValue = _pAttrib->Value();
			if (pValue)
			{
				auto value = trim(fig::string(pValue));
				return string_to_fixed(value);
			}
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::string> XmlReaderAttribute::TryGet<fig::string>() const noexcept
	{
		if (_pAttrib)
		{
			const char* pValue = _pAttrib->Value();
			if (pValue)
			{
				auto value = trim(fig::string(pValue));
				return std::make_optional(value);
			}
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::path> XmlReaderAttribute::TryGet<fig::path>() const noexcept
	{
		if (_pAttrib)
		{
			const char* pValue = _pAttrib->Value();
			if (pValue)
			{
				auto value = trim(fig::string(pValue));
				return std::make_optional(fig::path(value));
			}
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::bytes> XmlReaderAttribute::TryGet<fig::bytes>() const noexcept
	{
		if (_pAttrib)
		{
			const char* pValue = _pAttrib->Value();
			if (pValue)
			{
				auto value = trim(fig::string(pValue));
				return std::make_optional(Base64Decode(value));
			}
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::uuid> XmlReaderAttribute::TryGet<fig::uuid>() const noexcept
	{
		if (_pAttrib)
		{
			const char* pValue = _pAttrib->Value();
			if (pValue)
			{
				auto value = trim(fig::string(pValue));
				fig::uuid uuid = fig::uuid::from_str(value);
				return not uuid.empty() ? std::make_optional(uuid) : std::nullopt;
			}
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::color> XmlReaderAttribute::TryGet<fig::color>() const noexcept
	{
		if (_pAttrib)
		{
			const char* pValue = _pAttrib->Value();
			if (pValue)
			{
				auto value = trim(fig::string(pValue));
				return std::make_optional(fig::color::FromString(value));
			}
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::string_list> XmlReaderAttribute::TryGet<fig::string_list>() const noexcept
	{
		if (auto str = TryGet<fig::string>(); str.has_value())
			return std::make_optional(decode_csv(str.value()));
		return std::nullopt;
	}

	template<>
	std::optional<fig::handle> XmlReaderAttribute::TryGet<fig::handle>() const noexcept
	{
		if (_pAttrib)
		{
			const char* pValue = _pAttrib->Value();
			if (pValue)
			{
				auto value = trim(fig::string(pValue));
				return std::make_optional(fig::handle(value));
			}
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::timestamp> XmlReaderAttribute::TryGet<fig::timestamp>() const noexcept
	{
		if (_pAttrib)
		{
			int64_t value;
			if (_pAttrib->QueryInt64Value(&value) == XML_SUCCESS)
				return std::make_optional(fig::timestamp(value, fig::timezone::global));
		}
		return std::nullopt;
	}

	template<is_number_range T>
	std::optional<T> XmlReaderAttribute::TryGet() const noexcept
	{
		using TValue = std::ranges::range_value_t<T>;

		if (auto str = TryGet<fig::string>(); str.has_value())
		{
			auto values = decode_csv(str.value());

			T result {};
			result.reserve(values.size());

			for (const auto& v : values)
			{
				TValue value {};
				std::from_chars(v.data(), v.data() + v.size(), value);
				result.push_back(value);
			}

			return std::make_optional(result);
		}
		return std::nullopt;
	}

	XmlReaderElement::XmlReaderElement(fig::observer_ptr<const tinyxml2::XMLElement> pElement, fig::observer_ptr<const tinyxml2::XMLElement> pRoot, XmlReaderOptions options) noexcept :
		_pElement { pElement },
		_pRoot { pRoot },
		_options { options }
	{}

	std::optional<XmlReaderElement> XmlReaderElement::GetFirstElementAny() const noexcept
	{
		auto pElement = _pElement->FirstChildElement(nullptr);
		return pElement ? std::make_optional<XmlReaderElement>({ pElement, _pRoot, _options }) : std::nullopt;
	}

	std::optional<XmlReaderElement> XmlReaderElement::GetFirstElement(const fig::string& name) const noexcept
	{
		auto pElement = _pElement->FirstChildElement(name.c_str());
		return pElement ? std::make_optional<XmlReaderElement>({ pElement, _pRoot, _options }) : std::nullopt;
	}

	std::optional<XmlReaderElement> XmlReaderElement::GetNextSiblingAny() const noexcept
	{
		auto pElement = _pElement->NextSiblingElement();
		return pElement ? std::make_optional<XmlReaderElement>({ pElement, _pRoot, _options }) : std::nullopt;
	}

	std::optional<XmlReaderElement> XmlReaderElement::GetNextSibling() const noexcept
	{
		auto pElement = _pElement->NextSiblingElement(_pElement->Name());
		return pElement ? std::make_optional<XmlReaderElement>({ pElement, _pRoot, _options }) : std::nullopt;
	}

	std::optional<XmlReaderElement> XmlReaderElement::GetNextSibling(const fig::string& name) const noexcept
	{
		auto pElement = _pElement->NextSiblingElement(name.c_str());
		return pElement ? std::make_optional<XmlReaderElement>({ pElement, _pRoot, _options }) : std::nullopt;
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
	std::optional<fig::fixed> XmlReaderElement::TryGetValue<fig::fixed>() const noexcept
	{
		if (auto text = ReadText())
			return string_to_fixed(text.value());
		return std::nullopt;
	}

	template<>
	std::optional<fig::string> XmlReaderElement::TryGetValue<fig::string>() const noexcept
	{
		return ReadText();
	}

	template<>
	std::optional<fig::path> XmlReaderElement::TryGetValue<fig::path>() const noexcept
	{
		if (auto text = ReadText())
			return std::make_optional(fig::path(text.value()));
		return std::nullopt;
	}

	template<>
	std::optional<fig::bytes> XmlReaderElement::TryGetValue<fig::bytes>() const noexcept
	{
		if (auto text = ReadText())
			return std::make_optional(Base64Decode(trim(text.value())));
		return std::nullopt;
	}

	template<>
	std::optional<fig::uuid> XmlReaderElement::TryGetValue<fig::uuid>() const noexcept
	{
		if (auto text = ReadText())
		{
			fig::uuid uuid = fig::uuid::from_str(trim(text.value()));
			return not uuid.empty() ? std::make_optional(uuid) : std::nullopt;
		}
		return std::nullopt;
	}

	template<>
	std::optional<std::vector<fig::uuid>> XmlReaderElement::TryGetValue<std::vector<fig::uuid>>() const noexcept
	{
		if (auto str = TryGetValue<fig::string>(); str.has_value())
		{
			auto values = decode_csv(str.value());
			auto ids = values
				| std::views::transform([](auto&& v) { return fig::uuid::from_str(trim(v)); })
				| std::ranges::to<std::vector>();
			return std::make_optional(ids);
		}
		return std::nullopt;
	}

	template<>
	std::optional<fig::color> XmlReaderElement::TryGetValue<fig::color>() const noexcept
	{
		if (auto text = ReadText())
			return std::make_optional(fig::color::FromString(trim(text.value())));
		return std::nullopt;
	}

	template<>
	std::optional<fig::string_list> XmlReaderElement::TryGetValue<fig::string_list>() const noexcept
	{
		if (auto str = TryGetValue<fig::string>(); str.has_value())
			return std::make_optional(decode_csv(str.value()));
		return std::nullopt;
	}

	template<>
	std::optional<fig::handle> XmlReaderElement::TryGetValue<fig::handle>() const noexcept
	{
		const char* pValue = _pElement->GetText();
		if (pValue)
		{
			auto value = trim(fig::string(pValue));
			return std::make_optional(fig::handle(value));
		}
		return std::nullopt;
	}

	template<>
		std::optional<fig::timestamp> XmlReaderElement::TryGetValue<fig::timestamp>() const noexcept
	{
		int64_t value;
		if (_pElement->QueryInt64Text(&value) == XML_SUCCESS)
			return std::make_optional(fig::timestamp(value, fig::timezone::global));
		return std::nullopt;
	}

	template<is_number_range T>
	std::optional<T> XmlReaderElement::TryGetValue() const noexcept
	{
		using TValue = std::ranges::range_value_t<T>;

		if (auto str = TryGetValue<fig::string>(); str.has_value())
		{
			auto values = decode_csv(str.value());

			T result {};
			result.reserve(values.size());

			for (const auto& v : values)
			{
				TValue value {};
				std::from_chars(v.data(), v.data() + v.size(), value);
				result.push_back(value);
			}

			return std::make_optional(result);
		}
		return std::nullopt;
	}

	XmlReaderAttribute XmlReaderElement::operator[] (const std::string& key) const noexcept
	{
		auto pAttrib = _pElement->FindAttribute(key.c_str());
		return XmlReaderAttribute(pAttrib);
	}

	std::optional<fig::string> XmlReaderElement::ReadText() const noexcept
	{
		const char* pValue = _pElement->GetText();
		if (pValue)
		{
			fig::string text { pValue };
			if (_options.IsSet(XmlReaderOption::Trim))
				trim_inplace(text);
			if (_options.IsSet(XmlReaderOption::Unindent))
				unindent_inplace(text);
			if (_options.IsSet(XmlReaderOption::Unescape))
				unescape_inplace(text);
			return text;
		}
		return std::nullopt;
	}

	const XmlReaderOptions XmlReader::DefaultOptions = { XmlReaderOption::Trim, XmlReaderOption::Unindent, XmlReaderOption::Unescape };

	XmlReader::XmlReader(const fig::path& path, XmlReaderOptions options) :
		_options(options)
	{
		_pDoc = std::make_unique<XMLDocument>();
		if (_pDoc->LoadFile(path.u8string().c_str()) != XML_SUCCESS)
		{
			// Error
			_pDoc.reset();
			_pRoot.reset();
			return;
		}
		_pRoot = _pDoc->RootElement();
	}

	XmlReader::XmlReader(const fig::path& path, const fig::string& root, XmlReaderOptions options) :
		_options(options)
	{
		_pDoc = std::make_unique<XMLDocument>();
		if (_pDoc->LoadFile(path.u8string().c_str()) != XML_SUCCESS 
			or std::strcmp(_pDoc->RootElement()->Name(), root.c_str()) != 0)
		{
			// Error
			_pDoc.reset();
			_pRoot.reset();
			return;
		}
		_pRoot = _pDoc->RootElement();
	}

	XmlReader::XmlReader(const fig::string& document, XmlReaderOptions options) :
		_options(options)
	{
		_pDoc = std::make_unique<XMLDocument>();
		if (_pDoc->Parse(document.c_str()) != XML_SUCCESS)
		{
			// Error
			_pDoc.reset();
			_pRoot.reset();
			return;
		}
		_pRoot = _pDoc->RootElement();
	}

	XmlReader::XmlReader(fig::string_view document, XmlReaderOptions options) :
		_options(options)
	{
		_pDoc = std::make_unique<XMLDocument>();
		if (_pDoc->Parse((char*)document.data(), document.size()) != XML_SUCCESS)
		{
			// Error
			_pDoc.reset();
			_pRoot.reset();
			return;
		}
		_pRoot = _pDoc->RootElement();
	}

	XmlReader::~XmlReader()
	{
	}

	XmlReaderElement XmlReader::GetRoot() const noexcept
	{
		return IsOk() ? XmlReaderElement { fig::observer_ptr<const tinyxml2::XMLElement>(_pRoot), fig::observer_ptr<const tinyxml2::XMLElement>(_pRoot), _options } : XmlReaderElement { nullptr, nullptr, {} };
	}

	std::optional<XmlReaderElement> XmlReader::GetFirstElement(const fig::string& name) const noexcept
	{
		if (not IsOk())
			return std::nullopt;

		auto pElement = _pRoot->FirstChildElement(name.c_str());
		return pElement ? std::make_optional<XmlReaderElement>({ fig::observer_ptr<const tinyxml2::XMLElement>(pElement), fig::observer_ptr<const tinyxml2::XMLElement>(_pRoot), _options }) : std::nullopt;
	}

	// Explicit template instantiation

	template std::optional<uint8_t> XmlReaderAttribute::TryGet<uint8_t>() const noexcept;
	template std::optional<uint16_t> XmlReaderAttribute::TryGet<uint16_t>() const noexcept;
	template std::optional<uint32_t> XmlReaderAttribute::TryGet<uint32_t>() const noexcept;
	template std::optional<uint64_t> XmlReaderAttribute::TryGet<uint64_t>() const noexcept;
	template std::optional<int8_t> XmlReaderAttribute::TryGet<int8_t>() const noexcept;
	template std::optional<int16_t> XmlReaderAttribute::TryGet<int16_t>() const noexcept;
	template std::optional<int32_t> XmlReaderAttribute::TryGet<int32_t>() const noexcept;
	template std::optional<int64_t> XmlReaderAttribute::TryGet<int64_t>() const noexcept;
	template std::optional<std::vector<uint8_t>> XmlReaderAttribute::TryGet<std::vector<uint8_t>>() const noexcept;
	template std::optional<std::vector<uint16_t>> XmlReaderAttribute::TryGet<std::vector<uint16_t>>() const noexcept;
	template std::optional<std::vector<uint32_t>> XmlReaderAttribute::TryGet<std::vector<uint32_t>>() const noexcept;
	template std::optional<std::vector<uint64_t>> XmlReaderAttribute::TryGet<std::vector<uint64_t>>() const noexcept;
	template std::optional<std::vector<int8_t>> XmlReaderAttribute::TryGet<std::vector<int8_t>>() const noexcept;
	template std::optional<std::vector<int16_t>> XmlReaderAttribute::TryGet<std::vector<int16_t>>() const noexcept;
	template std::optional<std::vector<int32_t>> XmlReaderAttribute::TryGet<std::vector<int32_t>>() const noexcept;
	template std::optional<std::vector<int64_t>> XmlReaderAttribute::TryGet<std::vector<int64_t>>() const noexcept;
	template std::optional<std::vector<float>> XmlReaderAttribute::TryGet<std::vector<float>>() const noexcept;
	template std::optional<std::vector<double>> XmlReaderAttribute::TryGet<std::vector<double>>() const noexcept;
	template std::optional<uint8_t> XmlReaderElement::TryGetValue<uint8_t>() const noexcept;
	template std::optional<uint16_t> XmlReaderElement::TryGetValue<uint16_t>() const noexcept;
	template std::optional<uint32_t> XmlReaderElement::TryGetValue<uint32_t>() const noexcept;
	template std::optional<uint64_t> XmlReaderElement::TryGetValue<uint64_t>() const noexcept;
	template std::optional<int8_t> XmlReaderElement::TryGetValue<int8_t>() const noexcept;
	template std::optional<int16_t> XmlReaderElement::TryGetValue<int16_t>() const noexcept;
	template std::optional<int32_t> XmlReaderElement::TryGetValue<int32_t>() const noexcept;
	template std::optional<int64_t> XmlReaderElement::TryGetValue<int64_t>() const noexcept;
	template std::optional<std::vector<uint8_t>> XmlReaderElement::TryGetValue() const noexcept;
	template std::optional<std::vector<uint16_t>> XmlReaderElement::TryGetValue() const noexcept;
	template std::optional<std::vector<uint32_t>> XmlReaderElement::TryGetValue() const noexcept;
	template std::optional<std::vector<uint64_t>> XmlReaderElement::TryGetValue() const noexcept;
	template std::optional<std::vector<int8_t>> XmlReaderElement::TryGetValue() const noexcept;
	template std::optional<std::vector<int16_t>> XmlReaderElement::TryGetValue() const noexcept;
	template std::optional<std::vector<int32_t>> XmlReaderElement::TryGetValue() const noexcept;
	template std::optional<std::vector<int64_t>> XmlReaderElement::TryGetValue() const noexcept;
	template std::optional<std::vector<float>> XmlReaderElement::TryGetValue() const noexcept;
	template std::optional<std::vector<double>> XmlReaderElement::TryGetValue() const noexcept;
}