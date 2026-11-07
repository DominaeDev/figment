#pragma once

#include "Figment.h"
#include "user/EncryptedString.h"

namespace fig::io
{
	enum class AssetFolderType
	{
		Undefined = 0,
		Character,
		Story,
		World,
	};

	class AssetFolder
	{
	public:
		fig::uuid id;
		fig::uuid parent_id;
		fig::auth::EncryptedString encrypted_name {};
		fig::string settings {};
		fig::string decrypted_name {};
		AssetSyncState sync_state {};
		fig::timestamp createdAt {};
		fig::timestamp updatedAt {};
		AssetFolderType folderType {};
	};
}