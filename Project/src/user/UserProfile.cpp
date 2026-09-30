#include <pch.h>
#include "user/UserProfile.h"
#include "io/FileUtility.h"

using namespace fig::io;

namespace fig::user
{
	fig::path UserProfile::GetPath() const
	{
		return GetProfilesFolder() / filename_from_uuid(id);
	}
}