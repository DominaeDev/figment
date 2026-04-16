#ifndef CARD_META_DATA_H__
#define CARD_META_DATA_H__
#pragma once

#include "Types.h"

namespace fig::io
{
	struct CardMetaData
	{
		enum class Flag
		{
			New			= 1 << 0,
			Imported	= 1 << 1,
			Hidden		= 1 << 2,
			Favorite	= 1 << 3,
		};
		using Flags = EnumFlags<Flag>;

		fig::string name;
		fig::timestamp createdAt {};
		fig::timestamp updatedAt {};
		fig::timestamp lastUsedAt {};
		fig::string borderStyle {};
		uint32_t chatCount {};
		Flags flags {};
	};
}

#endif