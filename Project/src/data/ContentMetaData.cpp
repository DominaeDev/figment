#include <pch.h>
#include "data/ContentMetaData.h"

namespace fig::io
{
	bool ContentMetaData::IsNew() const noexcept
	{
		fig::timestamp now = fig::now();
		if ((now - createdAt) > fig::duration::days(3))
			return false;
		return true;
	}

	bool ContentMetaData::IsModified() const noexcept
	{
		return (createdAt != updatedAt or createdAt != lastUsedAt);
	}
}