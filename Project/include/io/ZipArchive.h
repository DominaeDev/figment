#pragma once

#include "Figment.h"
#include "miniz.h"

struct SDL_IOStream;

namespace fig::io
{
	enum class ZipError
	{
		NoError = 0,
		FileReadError,
		FileWriteError,
		InvalidArchive,
		EntryNotFound,
		TargetFailed,
		ExtractFailed
	};

	struct ZipEntry
	{
		fig::string name;
		uint64_t compressedSize;
		uint64_t uncompressedSize;
		bool isDirectory;
	};

	class ZipArchive
	{
	public:
		~ZipArchive();

		ZipError Open(const fig::path& archivePath);
		void Close();

		size_t GetEntryCount() const;
		std::optional<ZipEntry> GetEntry(size_t index) const;
		std::optional<ZipEntry> GetEntry(const fig::string& name) const;
		const std::vector<fig::string>& GetEntryNames() const;

		ZipError Extract(const fig::string& name, const fig::path& targetPath) const;

	private:
		mutable mz_zip_archive _archive {};
		fig::sdl::FileStream _stream {};
		std::vector<fig::string> _entryNames;
	};
}