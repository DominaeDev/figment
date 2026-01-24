#include <pch.h>
#include <sstream>
#include "model/Asset.h"
#include "model/UserProfile.h"
#include "util/Security.h"
#include "util/Common.h"

#include <chrono>

namespace fig::fs
{
	void Asset::SetData(fig::bytes&& data)
	{
		this->data = std::move(data);
		needSave = true;
	}

	void Asset::SetData(fig::byte_span data)
	{
		this->data.resize(data.size());
		std::memcpy(this->data.data(), data.data(), data.size());
		needSave = true;
	}

	constexpr fig::string Asset::AsString() const
	{
		fig::string str;
		str.assign(reinterpret_cast<const char*>(data.data()), data.size());
		return str;
	}

	AssetFile Asset::ToFile() const noexcept
	{
		return AssetFile {
			.assetID = id,
			.parentID = parentID,
			.createdAt = common_util::utc_now(),
			.updatedAt = common_util::utc_now(),
			.data = data.data(),
			.data_length = data.size(),
			.meta = parameters,
		};
	}
}