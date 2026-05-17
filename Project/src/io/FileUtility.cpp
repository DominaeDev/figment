#include <pch.h>
#include "io/FileUtility.h"
#include "io/Asset.h"
#include <spng.h>
#include <fstream>

namespace fig::io
{
	std::expected<fig::bytes, FileError> ReadFile(fig::path filename)
	{
		try
		{
			std::ifstream file(filename.wstring(), std::ios::binary | std::ios::in | std::ios::ate);
			if (!file)
				return std::unexpected(FileError::NotFound);

			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);
			if (size == 0uz)
				return std::unexpected(FileError::ReadError); // Empty file

			fig::bytes content(size);
			file.read(reinterpret_cast<char*>(content.data()), size);
			return content; // rvo
		}
		catch (...)
		{
			return std::unexpected(FileError::UnknownError);
		}
	}

	FileError WriteFile(fig::path filename, fig::byte_span data)
	{
		try
		{
			std::ofstream file(filename.wstring(), std::ios::binary | std::ios::out | std::ios::trunc);
			if (not file.is_open())
				return FileError::WriteError;

			file.write((const char*)(data.data()), data.size());
			return FileError::NoError;
		}
		catch (...)
		{
			return FileError::WriteError;
		}
	}

	std::expected<fig::string, FileError> ReadTextFile(const fig::path& filename, bool normalizeNewlines)
	{
		try
		{
			std::ifstream file(filename.c_str(), std::ios::binary | std::ios::in | std::ios::ate);
			if (!file)
				return std::unexpected(FileError::NotFound);

			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);

			fig::string content;
			content.reserve(size);
			content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
			if (!content.empty())
				return normalizeNewlines ? normalize_newlines(content) : content;
			return ""; // Empty file
		}
		catch (...)
		{
			return std::unexpected(FileError::UnknownError);
		}
	}

	FileError ReadTextFile(const fig::path& filename, fig::string& out_content, bool normalizeNewlines)
	{
		auto content = ReadTextFile(filename, normalizeNewlines);
		if (content.has_value())
		{
			out_content = content.value();
			return FileError::NoError;
		}
		return content.error();
	}

	FileError WriteTextFile(const fig::path& filename, const fig::string& content, bool append)
	{
		try
		{
			std::ofstream file(filename.c_str(), std::ios::binary | std::ios::out | (append ? std::ios::app : std::ios::trunc));
			if (!file.is_open())
				return FileError::WriteError;

			file.write(content.c_str(), content.length());
			return file.fail() ? FileError::WriteError : FileError::NoError;
		}
		catch (...)
		{
			return FileError::WriteError;
		}
	}

	std::expected<std::vector<fig::path>, FileError> FindFilesInPath(const fig::path& dirPath, const fig::string& extension)
	{
		std::vector<fig::path> matchingFiles;
		fig::path directory(dirPath);

		if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
			return std::unexpected(FileError::DirectoryDoesNotExist);

		for (const auto& entry : std::filesystem::directory_iterator(directory))
		{
			if (std::filesystem::is_regular_file(entry) && (extension.empty() || equals(entry.path().extension().u8string(), extension, true)))
				matchingFiles.push_back(entry.path());
		}

		return matchingFiles;
	}

	fig::string GetFilename(const fig::string& str)
	{
		size_t pos = str.find_last_of("\\/"); //! hmm...
		if (pos == string::npos)
			return str;
		return str.substr(pos + 1);
	}

	fig::string GetFileExt(fig::path filename)
	{
		return lcase(fig::path(filename).extension().u8string());
	}

	using sPngCtx = stdex::c_resource<spng_ctx, spng_ctx_new, spng_ctx_free>;

	std::expected<fig::string, FileError> ReadPNGMeta(fig::path filename, const fig::string& keyword, bool bDecodeBase64) noexcept
	{
#if defined(_MSC_VER)
		FILE* file = nullptr;
		if (_wfopen_s(&file, filename.wstring().c_str(), L"rb") != 0 or !file)
			return std::unexpected(FileError::NotFound);
#else
		FILE* file = fopen(filename.string().c_str(), "rb");
		if (!file)
			return std::unexpected(FileError::NotFound);
#endif
		
		fig::string content;
		try
		{
			sPngCtx ctx { 0 };
			spng_set_png_file(ctx.get(), file);

			struct spng_ihdr ihdr;
			spng_get_ihdr(ctx.get(), &ihdr);

			uint32_t n_text = 0;
			spng_get_text(ctx.get(), nullptr, &n_text);

			std::vector<spng_text> texts(n_text);
			spng_get_text(ctx.get(), texts.data(), &n_text);
			
			fclose(file);

			auto itFind = std::find_if(texts.cbegin(), texts.cend(), [&keyword](auto& t) { return strcmp(t.keyword, keyword.c_str()) == 0; });
			if (itFind == texts.cend())
				return std::unexpected(FileError::ReadError); // Not found
			content = fig::string { itFind->text };
		}
		catch (...)
		{
			fclose(file);
			return std::unexpected(FileError::UnknownError);
		}

		if (bDecodeBase64)
		{
			auto decoded = Base64Decode(content);
			content.assign(reinterpret_cast<const char*>(decoded.data()), decoded.size());
		}

		return content;
	}

	std::expected<fig::string, FileError> ReadPNGMeta(const fig::bytes& buffer, const fig::string& keyword, bool bDecodeBase64) noexcept
	{
		if (buffer.size() == 0)
			return std::unexpected(FileError::ReadError);

		std::vector<spng_text> texts;
		try
		{
			sPngCtx ctx { 0 };
			spng_set_png_buffer(ctx.get(), buffer.data(), buffer.size());

			struct spng_ihdr ihdr;
			spng_get_ihdr(ctx.get(), &ihdr);

			uint32_t n_text = 0;
			spng_get_text(ctx.get(), nullptr, &n_text);

			texts.resize(n_text);
			spng_get_text(ctx.get(), texts.data(), &n_text);
		}
		catch (...)
		{
			return std::unexpected(FileError::UnknownError);
		}

		auto itFind = std::find_if(texts.cbegin(), texts.cend(), [&keyword](auto& t) { return strcmp(t.keyword, keyword.c_str()) == 0; });
		if (itFind == texts.cend())
			return std::unexpected(FileError::ReadError); // Not found

		fig::string content { itFind->text };
		if (bDecodeBase64)
		{
			auto decoded = Base64Decode(content);
			content.assign(reinterpret_cast<const char*>(decoded.data()), decoded.size());
		}

		return content;
	}
}