#pragma once

#include "Figment.h"
#include "io/Xml.h"

namespace fig::chat
{
	template <typename T>
	struct _UserDefinedOption
	{
		fig::handle id;
		fig::string label;
		fig::string hint;
		T defaultValue {};
	};

	using UserDefinedToggle = _UserDefinedOption<bool>;
	using UserDefinedNumber = _UserDefinedOption<int32_t>;
	using UserDefinedText = _UserDefinedOption<fig::string>;
	using UserDefinedOption = std::variant<UserDefinedToggle, UserDefinedNumber, UserDefinedText>;

	struct UserDefinedOptions
	{
		bool LoadFromXml(fig::data::XmlReaderElement xml);
		void SaveToXml(fig::data::XmlWriterElement xml) const;

		inline size_t size() const noexcept { return options.size(); }
		inline auto begin() const noexcept { return std::begin(options); }
		inline auto end() const noexcept { return std::end(options); }
		
		inline UserDefinedOption& operator[](size_t index) { return options[index]; }
		inline const UserDefinedOption& operator[](size_t index) const { return options[index]; }

		std::vector<UserDefinedOption> options;
	};

}