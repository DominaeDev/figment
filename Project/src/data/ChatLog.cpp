#include <pch.h>
#include "data/ChatLog.h"
#include "io/XmlSerializable.h"

using namespace fig::io;
using namespace fig::chat;

namespace fig::data
{

	auto ChatLog::GetXmlFields()
	{
		return XmlFields(
			XmlElement { "ID",			&ChatLog::assetId }.MustExist(),
			XmlElement { "Entries",		&ChatLog::entries }.MustExist()
		);

		static_assert(XmlSerializable<ChatLog>);
	}

	auto ChatLog::Entry::GetXmlFields()
	{
		return XmlFields(
			XmlElement { "ID",			&Entry::entryId }
				.MustExist(),
			XmlElement { "Speaker",		&Entry::speakerId }
				.MustExist(),
			XmlElement { "Turn",		&Entry::turn }
				.MustExist(),
			XmlElement { "Role",		&Entry::role,
				[](auto& value) { return enum_serialize(value, RoleMapping); },
				[](auto& value) { return enum_deserialize(value, RoleMapping); }
			}	.MustExist(),
			XmlElement { "Type",		&Entry::msgType,
				[](auto& value) { return enum_serialize(value, MessageTypeMapping); },
				[](auto& value) { return enum_deserialize(value, MessageTypeMapping); }
			}	.MustExist(),
			XmlElement { "Timestamp",	&Entry::timestamp },
			XmlElement { "Content",		&Entry::content }
				.MustExist()
		);

		static_assert(XmlSerializable<Entry>);
	}

}