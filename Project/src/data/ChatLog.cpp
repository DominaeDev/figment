#include <pch.h>
#include "data/ChatLog.h"
#include "io/XmlSerializable.h"

using namespace fig::io;
using namespace fig::chat;

namespace fig::data
{
	auto ChatLog::SerializeInfo()
	{
		return XmlFields(
			AsElement { "ID", &ChatLog::assetId }
				.MustExist(),
			AsElement { "Entries", &ChatLog::entries }
				.MustExist()
		);

		static_assert(XmlSerializable<ChatLog>);
	}

	auto ChatLog::Entry::SerializeInfo()
	{
		return XmlFields(
			AsElement { "ID", &Entry::entryId }
				.MustExist(),
			AsElement { "Speaker", &Entry::speakerId }
				.MustExist(),
			AsElement { "Turn", &Entry::turn }
				.MustExist(),
			AsElement { "Role", &Entry::role,
				[](auto& value) { return enum_serialize(value, RoleMapping); },
				[](auto& value) { return enum_deserialize(value, RoleMapping); }
			}	.MustExist(),
			AsElement { "Type", &Entry::msgType,
				[](auto& value) { return enum_serialize(value, MessageTypeMapping); },
				[](auto& value) { return enum_deserialize(value, MessageTypeMapping); }
			}	.MustExist(),
			AsElement { "Timestamp", &Entry::timestamp },
			AsElement { "Content", &Entry::content }
				.MustExist()
		);

		static_assert(XmlSerializable<Entry>);
	}

}