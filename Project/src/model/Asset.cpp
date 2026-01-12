#include <pch.h>
#include <sstream>
#include "model/Asset.h"

namespace fig::fs
{
	constexpr fig::string Asset::as_string() const
	{
		fig::string str;
		str.assign(reinterpret_cast<const char*>(data.data()), data.size());
		return str;
	}

}