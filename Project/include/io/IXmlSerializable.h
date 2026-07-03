#ifndef IXML_SERIALIZABLE_H__
#define IXML_SERIALIZABLE_H__
#pragma once

#include "Figment.h"
#include "io/XmlReader.h"
#include "io/XmlWriter.h"
#include "io/XmlSerializable.h"
#include "util/FixedString.h"

namespace fig::data
{
	template <fixed_string ROOT_NAME, int16_t VERSION = -1>
	class IXmlSerializable 
	{
	public:
		virtual ~IXmlSerializable() = default;

		static constexpr auto format_version { VERSION };

		fig::io::FileError LoadFromXml(this XmlSerializable auto& self, const fig::path& path) noexcept
		{
			XmlReader xml(path, ROOT_NAME.c_str());
			if (not xml.IsOk())
				return fig::io::FileError::UnrecognizedFormat;

			auto root = xml.GetRoot();
			if (root.GetName() != ROOT_NAME.c_str())
				return fig::io::FileError::UnrecognizedFormat;

			self._format_version = root["format"].Get<int16_t>(-1);

			return self.LoadFromXml(xml.GetRoot());
		}

		fig::io::FileError LoadFromXml(this XmlSerializable auto& self, const fig::byte_span& buffer) noexcept
		{
			XmlReader xml(fig::string_view { (const char*)buffer.data(), buffer.size() });
			if (not xml.IsOk())
				return fig::io::FileError::UnrecognizedFormat;

			auto root = xml.GetRoot();
			if (root.GetName() != ROOT_NAME.c_str())
				return fig::io::FileError::UnrecognizedFormat;

			self._format_version = root["format"].Get<int16_t>(-1);

			return self.LoadFromXml(xml.GetRoot());
		}

		fig::io::FileError LoadFromXml(this XmlSerializable auto& self, XmlReaderElement node) noexcept
		{
			if (not XmlDeserialize(node, self))
				return fig::io::FileError::UnrecognizedFormat;
			
			if (not self.OnLoadFromXml(node))
				return fig::io::FileError::ReadError;

			if (not self.Validate())
				return fig::io::FileError::ReadError;

			return fig::io::FileError::NoError;
		}

		fig::io::FileError SaveToXml(this const XmlSerializable auto& self, const fig::path& path) noexcept
		{
			XmlWriter xml { ROOT_NAME.c_str() };
			auto root = xml.GetRoot();
			root["format"] = VERSION;
			self.SaveToXml(root);
			if (not xml.WriteToFile(path))
				return fig::io::FileError::WriteError;
			return fig::io::FileError::NoError;
		}

		void SaveToXml(this const XmlSerializable auto& self, fig::bytes& buffer) noexcept
		{
			XmlWriter xml { ROOT_NAME.c_str() };
			auto root = xml.GetRoot();
			root["format"] = VERSION;
			self.SaveToXml(root);
			xml.WriteToMemory(buffer);
		}

		void SaveToXml(this const XmlSerializable auto& self, XmlWriterElement node) noexcept
		{
			XmlSerialize(node, self);
		}

	protected:
		int16_t _format_version { VERSION };

		bool Validate() const noexcept 
		{ 
			if constexpr (VERSION >= 0)
			{
				if (_format_version < 0 or _format_version > VERSION)
					return false;
			}
			return OnValidate();
		}
		virtual bool OnValidate() const noexcept { return true; }
		virtual bool OnLoadFromXml(XmlReaderElement node) { return true; }
	};
}

#endif