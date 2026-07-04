#pragma once

#include "Figment.h"
#include "io/XmlReader.h"
#include "io/XmlWriter.h"
#include "io/XmlSerialize.h"
#include "util/FixedString.h"

namespace fig::data
{
	template <fixed_string ROOT_NAME, int16_t VERSION = -1>
	class XmlData 
	{
	public:
		virtual ~XmlData() = default;

		static constexpr auto root_name { ROOT_NAME.c_str() };
		static constexpr auto format_version { VERSION };

		fig::io::FileError LoadFromXml(this IsXmlSerializable auto& self, const fig::path& path) noexcept
		{
			XmlReader xml(path, root_name);
			if (not xml.IsOk())
				return fig::io::FileError::UnrecognizedFormat;

			auto root = xml.GetRoot();
			if (root.GetName() != root_name)
				return fig::io::FileError::UnrecognizedFormat;

			self._format_version = root["format"].Get<int16_t>(-1);

			return self.LoadFromXml(xml.GetRoot());
		}

		fig::io::FileError LoadFromXml(this IsXmlSerializable auto& self, const fig::byte_span& buffer) noexcept
		{
			XmlReader xml(fig::string_view { (const char*)buffer.data(), buffer.size() });
			if (not xml.IsOk())
				return fig::io::FileError::UnrecognizedFormat;

			auto root = xml.GetRoot();
			if (root.GetName() != root_name)
				return fig::io::FileError::UnrecognizedFormat;

			self._format_version = root["format"].Get<int16_t>(-1);

			return self.LoadFromXml(xml.GetRoot());
		}

		fig::io::FileError LoadFromXml(this IsXmlSerializable auto& self, XmlReaderElement node) noexcept
		{
			if (not Deserialize(node, self))
				return fig::io::FileError::UnrecognizedFormat;
			
			if (not self.OnLoadFromXml(node))
				return fig::io::FileError::ReadError;

			if (not self.Validate())
				return fig::io::FileError::ReadError;

			return fig::io::FileError::NoError;
		}

		fig::io::FileError SaveToXml(this const IsXmlSerializable auto& self, const fig::path& path) noexcept
		{
			XmlWriter xml { root_name };
			auto root = xml.GetRoot();
			root["format"] = format_version;
			self.SaveToXml(root);
			if (not xml.WriteToFile(path))
				return fig::io::FileError::WriteError;
			return fig::io::FileError::NoError;
		}

		void SaveToXml(this const IsXmlSerializable auto& self, fig::bytes& buffer) noexcept
		{
			XmlWriter xml { root_name };
			auto root = xml.GetRoot();
			root["format"] = format_version;
			self.SaveToXml(root);
			xml.WriteToMemory(buffer);
		}

		void SaveToXml(this const IsXmlSerializable auto& self, XmlWriterElement node) noexcept
		{
			Serialize(node, self);
		}

	protected:
		int16_t _format_version { format_version };

		bool Validate() const noexcept 
		{ 
			if constexpr (format_version >= 0)
			{
				if (_format_version < 0 or _format_version > format_version)
					return false;
			}
			return OnValidate();
		}
		virtual bool OnValidate() const noexcept { return true; }
		virtual bool OnLoadFromXml(XmlReaderElement node) { return true; }
	};
}
