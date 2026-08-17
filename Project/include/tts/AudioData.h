#pragma once

#include "Figment.h"

namespace fig::tts
{
	struct AudioData
	{
		fig::bytes bytes;
		fig::string base64;

		const fig::string& AsBase64() noexcept
		{
			if (base64.empty())
				base64 = Base64Encode(bytes);
			return base64;
		}

		fig::byte_span AsBytes() noexcept
		{
			if (bytes.empty())
				bytes = Base64Decode(base64);
			return bytes;
		}

		fig::string AsBase64() const noexcept
		{
			if (not base64.empty())
				return base64;
			return Base64Encode(bytes);
		}

		fig::bytes AsBytes() const noexcept
		{
			if (not bytes.empty())
				return bytes;
			return Base64Decode(base64);
		}

		inline bool empty() const noexcept
		{
			return bytes.empty() and base64.empty();
		}

		static AudioData FromBase64(fig::string_view base64)
		{
			AudioData audioData;
			audioData.base64 = fig::string { base64 };
			return audioData;
		}

		static AudioData FromBytes(fig::byte_span bytes)
		{
			AudioData audioData;
			audioData.bytes = fig::bytes { bytes.cbegin(), bytes.cend() };
			return audioData;
		}

		static AudioData FromBytes(fig::bytes&& bytes)
		{
			AudioData audioData;
			audioData.bytes = std::move(bytes);
			return audioData;
		}
	};
}