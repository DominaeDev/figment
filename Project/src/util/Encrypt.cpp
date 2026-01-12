#include <pch.h>
#include <cassert>
#include "util/Encrypt.h"
#include "AES.h"

static void encrypt_data(unsigned char* pData, size_t length, const fig::encrypt::Key& key)
{
	static_assert(sizeof(unsigned char) == sizeof(std::byte));
	static_assert(sizeof(key) == 32);
	assert(length % 16 == 0);

	unsigned char u8Key[32];
	std::memcpy(u8Key, key.data(), key.size());

	Cipher::Aes<256> aes(u8Key);

	constexpr size_t stride = 16;
	for (size_t i = 0; i < length; i += stride)
		aes.encrypt_block(pData + i);
}

static void decrypt_data(unsigned char* pData, size_t length, const fig::encrypt::Key& key)
{
	static_assert(sizeof(unsigned char) == sizeof(std::byte));
	static_assert(sizeof(key) == 32);
	assert(length % 16 == 0);

	unsigned char u8Key[32];
	std::memcpy(u8Key, key.data(), key.size());

	Cipher::Aes<256> aes(u8Key);

	constexpr size_t stride = 16;
	for (size_t i = 0; i < length; i += stride)
		aes.decrypt_block(pData + i);
}

namespace fig::encrypt
{
	DataBlock::DataBlock(const unsigned char* pData, size_t length)
	{
		_data_length = length;
		auto padded_size = _data_length % 16 == 0 ? _data_length : (1 + _data_length / 16) * 16;
		_data.resize(padded_size, 0);
		std::memcpy(_data.data(), pData, _data_length);
	}

	DataBlock::DataBlock(const fig::bytes& data)
	{
		_data_length = data.size();
		auto padded_size = _data_length % 16 == 0 ? _data_length : (1 + _data_length / 16) * 16;
		_data.resize(padded_size, 0);
		std::memcpy(_data.data(), data.data(), _data_length);
	}

	DataBlock::DataBlock(DataBlock&& other) noexcept
	{
		_data = std::move(other._data);
		_data_length = other._data_length;
		other._data.resize(0);
	}

	void DataBlock::Encrypt(const Key& key)
	{
		encrypt_data(reinterpret_cast<unsigned char*>(_data.data()), _data.size(), key);
	}

	void DataBlock::Decrypt(const Key& key)
	{
		decrypt_data(reinterpret_cast<unsigned char*>(_data.data()), _data.size(), key);
	}
}

namespace fig::common_util
{
	void Encrypt(fig::bytes& data, const fig::encrypt::Key& key)
	{
		fig::encrypt::DataBlock block(data);
		block.Encrypt(key);
		data.resize(block.size());
		std::memcpy(data.data(), block.data(), data.size());
	}

	void Decrypt(fig::bytes& data, const fig::encrypt::Key& key)
	{
		fig::encrypt::DataBlock block(data);
		block.Decrypt(key);
		data.resize(block.size());
		std::memcpy(data.data(), block.data(), block.size());
	}
}