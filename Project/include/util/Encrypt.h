#ifndef ENCRYPT_H__
#define ENCRYPT_H__
#pragma once

#include "Types.h"

namespace fig::encrypt
{
	using Bit128 = std::array<std::byte, 16>;
	using Bit256 = std::array<std::byte, 32>;
	using Key = Bit256;

	// Data block with padding
	class DataBlock
	{
	public:
		DataBlock(const unsigned char* pData, size_t length);
		DataBlock(const fig::bytes& data);
		DataBlock(const DataBlock& other) = default;
		DataBlock(DataBlock&& other) noexcept;
		~DataBlock() = default;

		constexpr unsigned char* data() noexcept { return _data.data(); }
		constexpr size_t data_length() const noexcept { return _data_length; }
		constexpr size_t size() const noexcept { return _data.size(); }

		void Encrypt(const Key& key);
		void Decrypt(const Key& key);

	private:
		std::vector<unsigned char> _data;
		size_t _data_length {};
	};
}

namespace fig::common_util
{
	void Encrypt(fig::bytes& data, const fig::encrypt::Key& key);
	void Decrypt(fig::bytes& data, const fig::encrypt::Key& key);
}

#endif
