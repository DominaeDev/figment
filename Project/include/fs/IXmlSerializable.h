#ifndef IXML_SERIALIZABLE_H__
#define IXML_SERIALIZABLE_H__
#pragma once

#include "Types.h"
#include "fs/XmlReader.h"
#include "fs/XmlWriter.h"
#include "fs/XmlSerializable.h"

namespace fig::io
{
	class IXmlSerializable 
	{
		IXmlSerializable() = delete;
	public:
		IXmlSerializable(const fig::string& rootName) : _rootName { rootName } {}
		virtual ~IXmlSerializable() = default;

		FileError LoadFromXml(this XmlSerializable auto& self, const fig::path& path) noexcept
		{
			if (not (std::filesystem::exists(path) and std::filesystem::is_regular_file(path)))
				return FileError::NotFound;

			XmlReader xml(path, self._rootName);
			if (not xml.IsOk())
				return FileError::UnrecognizedFormat;

			return self.LoadFromXml(xml.GetRoot());
		}

		FileError LoadFromXml(this XmlSerializable auto& self, const fig::byte_span& buffer) noexcept
		{
			XmlReader xml(fig::string_view { (const char*)buffer.data(), buffer.size() });
			if (not xml.IsOk() or xml.GetRoot().GetName() != self._rootName)
				return FileError::UnrecognizedFormat;

			return self.LoadFromXml(xml.GetRoot());
		}

		FileError LoadFromXml(this XmlSerializable auto& self, const XmlReaderElement& node) noexcept
		{
			if (not XmlDeserialize(node, self))
				return FileError::UnrecognizedFormat;
			
			if (not self.Validate())
				return FileError::UnrecognizedFormat;

			return FileError::NoError;
		}

		FileError SaveToXml(this const XmlSerializable auto& self, const fig::path& path) noexcept
		{
			XmlWriter xml { self._rootName };
			auto root = xml.GetRoot();
			self.SaveToXml(root);
			if (not xml.WriteToFile(path))
				return FileError::WriteError;
			return FileError::NoError;
		}

		void SaveToXml(this const XmlSerializable auto& self, fig::bytes& buffer) noexcept
		{
			XmlWriter xml { self._rootName };
			auto root = xml.GetRoot();
			self.SaveToXml(root);
			xml.WriteToMemory(buffer);
		}

		void SaveToXml(this const XmlSerializable auto& self, XmlWriterElement& node) noexcept
		{
			XmlSerialize(node, self);
		}

	protected:
		virtual bool Validate() const noexcept { return true; }

	private:
		fig::string _rootName;
	};
}

#endif