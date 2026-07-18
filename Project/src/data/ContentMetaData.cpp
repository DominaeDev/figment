#include <pch.h>
#include "data/ContentMetaData.h"

namespace fig::io
{
	bool ContentMetaData::IsNew() const noexcept
	{
		if (createdAt != updatedAt or createdAt != lastUsedAt)
			return false;
		
		fig::timestamp now = fig::now();
		if ((now - createdAt) > fig::duration::days(3))
			return false;
		return true;
	}
}