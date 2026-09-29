#include <pch.h>
#include "io/ZipArchive.h"

#include <SDL3/SDL_iostream.h>

namespace fig::io
{
	static size_t ReadArchive(void* opaque, mz_uint64 offset, void* buffer, size_t count)
	{
		SDL_IOStream* pStream = static_cast<SDL_IOStream*>(opaque);
		Sint64 position = static_cast<Sint64>(offset);
		Sint64 result = SDL_SeekIO(pStream, position, SDL_IO_SEEK_SET);
		if (result < 0)
			return 0;

		return SDL_ReadIO(pStream, buffer, count);
	}

	static size_t WriteTarget(void* opaque, mz_uint64, const void* buffer, size_t count)
	{
		SDL_IOStream* stream = static_cast<SDL_IOStream*>(opaque);
		return SDL_WriteIO(stream, buffer, count);
	}

	ZipArchive::~ZipArchive()
	{
		Close();
	}

	ZipError ZipArchive::Open(const fig::path& archivePath)
	{
		Close();

		_stream = fig::sdl::FileStream(archivePath.string().c_str(), "rb");
		if (_stream.empty())
			return ZipError::FileReadError;

		Sint64 size = SDL_GetIOSize(_stream.get());
		mz_zip_zero_struct(&_archive);
		_archive.m_pIO_opaque = _stream.get();
		_archive.m_pRead = ReadArchive;

		if (mz_zip_reader_init(&_archive, static_cast<mz_uint64>(size), 0) != MZ_TRUE)
		{
			_stream.reset();
			return ZipError::InvalidArchive;
		}

		mz_uint count = mz_zip_reader_get_num_files(&_archive);
		_entryNames.reserve(count);
		for (mz_uint index = 0; index < count; ++index)
		{
			mz_zip_archive_file_stat fileStatus;
			if (mz_zip_reader_file_stat(&_archive, index, &fileStatus) != MZ_TRUE)
				continue;

			_entryNames.push_back(fileStatus.m_filename);
		}

		return ZipError::NoError;
	}

	void ZipArchive::Close()
	{
		_entryNames.clear();
		if (_stream.empty())
			return;

		mz_zip_reader_end(&_archive);
		_stream.reset();
	}

	size_t ZipArchive::GetEntryCount() const
	{
		if (_stream.empty())
			return 0uz;

		return mz_zip_reader_get_num_files(&_archive);
	}

	std::optional<ZipEntry> ZipArchive::GetEntry(size_t index) const
	{
		if (index >= GetEntryCount())
			return std::nullopt;

		mz_zip_archive_file_stat fileStatus;
		if (mz_zip_reader_file_stat(&_archive, static_cast<uint32_t>(index), &fileStatus) != MZ_TRUE)
			return std::nullopt;

		ZipEntry entry;
		entry.name = fileStatus.m_filename;
		entry.compressedSize = fileStatus.m_comp_size;
		entry.uncompressedSize = fileStatus.m_uncomp_size;
		entry.isDirectory = fileStatus.m_is_directory != 0;
		return entry;
	}

	std::optional<ZipEntry> ZipArchive::GetEntry(const fig::string& name) const
	{
		if (_stream.empty())
			return std::nullopt;

		int index = mz_zip_reader_locate_file(&_archive, name.c_str(), nullptr, 0);
		if (index < 0)
			return std::nullopt;

		return GetEntry(static_cast<size_t>(index));
	}

	const std::vector<fig::string>& ZipArchive::GetEntryNames() const
	{
		return _entryNames;
	}

	ZipError ZipArchive::Extract(const fig::string& name, const fig::path& targetPath) const
	{
		if (_stream.empty())
			return ZipError::EntryNotFound;

		int32_t entryIndex = mz_zip_reader_locate_file(&_archive, name.c_str(), nullptr, 0);
		if (entryIndex < 0)
			return ZipError::EntryNotFound;

		if (auto writeFile = fig::sdl::FileStream(targetPath.string().c_str(), "wb"))
		{
			if (mz_zip_reader_extract_to_callback(&_archive, static_cast<mz_uint>(entryIndex), WriteTarget, writeFile.get(), 0) != MZ_TRUE)
				return ZipError::FileWriteError;

			SDL_FlushIO(writeFile.get());
			return ZipError::NoError;
		}
		return ZipError::FileWriteError;
	}
}