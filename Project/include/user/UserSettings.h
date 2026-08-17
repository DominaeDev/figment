#pragma once

#include "io/SettingsCollection.h"

namespace fig::io
{
	namespace UserSetting
	{
		namespace Interface
		{
			constexpr fig::io::SettingKey SidePanelCollapsed	{ "Interface", "SidePanel.Collapsed" };

			namespace CharacterList
			{
				constexpr fig::io::SettingKey SmallCards		{ "Interface", "Characters.SmallCards" };
				constexpr fig::io::SettingKey ShowTags			{ "Interface", "Characters.ShowTags" };
				constexpr fig::io::SettingKey Sorting			{ "Interface", "Characters.Sorting" };
				constexpr fig::io::SettingKey Ordering			{ "Interface", "Characters.Ordering" };
				constexpr fig::io::SettingKey Filtering			{ "Interface", "Characters.Filter" };
			}

			namespace ChatList
			{
				constexpr fig::io::SettingKey Sorting			{ "Interface", "Chats.Sorting" };
				constexpr fig::io::SettingKey Ordering			{ "Interface", "Chats.Ordering" };
				constexpr fig::io::SettingKey Filtering			{ "Interface", "Chats.Filter" };
			}

			namespace Chat
			{
				constexpr fig::io::SettingKey InfoPanelWidth		{ "Interface", "Chat.InfoPanel.Width" };
				constexpr fig::io::SettingKey InfoPanelCollapsed	{ "Interface", "Chat.InfoPanel.Collapsed" };
				constexpr fig::io::SettingKey ImageSize				{ "Interface", "Chat.InfoPanel.ImageSize" };
			}
		}

		namespace TTS
		{
			constexpr fig::io::SettingKey Enabled			{ "TTS", "Enabled" };
			constexpr fig::io::SettingKey Volume			{ "TTS", "Volume" };
			constexpr fig::io::SettingKey TTSModel			{ "TTS", "Model.Speech" };
			constexpr fig::io::SettingKey DesignModel		{ "TTS", "Model.Design" };
		}
		
		namespace Settings
		{
			constexpr fig::io::SettingKey Clock			{ "Settings", "Clock" };
			constexpr fig::io::SettingKey ModelPreset	{ "Settings", "ModelPreset" };
		}
	}

	enum class SortBy
	{
		Name = 0,
		CreatedAt,
		UpdatedAt,
		LastUsedAt,
		ChatCount,

		Count,
		Default = UpdatedAt,
	};

	enum class OrderBy
	{
		Ascending = 0,
		Descending,
		
		Count,
		Default = Descending,
	};

	enum class FilterFlag : int32_t
	{
		GenderMale		= 1 << 0,
		GenderFemale	= 1 << 1,
		GenderOther		= 1 << 2,

		New				= 1 << 4,
		Starred			= 1 << 5,
		Chats			= 1 << 6,
		Hidden			= 1 << 7,
		SourceCreated	= 1 << 8,
		SourceImported	= 1 << 9,
	};
	using FilterFlags = EnumFlags<FilterFlag>;

	static auto FilterFlagMapping = std::array<std::pair<FilterFlag, std::string_view>, 9> {
		std::pair { FilterFlag::GenderMale,		"male" },
		std::pair { FilterFlag::GenderFemale,	"female" },
		std::pair { FilterFlag::GenderOther,	"nonbinary" },
		std::pair { FilterFlag::New,			"new" },
		std::pair { FilterFlag::Starred,		"starred" },
		std::pair { FilterFlag::Chats,			"chats" },
		std::pair { FilterFlag::Hidden,			"hidden" },
		std::pair { FilterFlag::SourceCreated,	"created" },
		std::pair { FilterFlag::SourceImported,	"imported" },
	};

	constexpr FilterFlags DefaultFilterFlags { FilterFlag::GenderMale, FilterFlag::GenderFemale, FilterFlag::GenderOther, FilterFlag::SourceCreated, FilterFlag::SourceImported  };
	
	enum class ChatFilterFlag : int32_t
	{
		Starred			= 1 << 1,
		Hidden			= 1 << 2,
	};
	using ChatFilterFlags = EnumFlags<ChatFilterFlag>;

	static auto ChatFilterFlagMapping = std::array<std::pair<ChatFilterFlag, std::string_view>, 3> {
		std::pair { ChatFilterFlag::Starred,	"starred" },
		std::pair { ChatFilterFlag::Hidden,		"hidden" },
	};

	constexpr ChatFilterFlags DefaultChatFilterFlags {};

	class UserSettings : public SettingsCollection
	{
	public:
		explicit UserSettings(const fig::path& path) noexcept : SettingsCollection(path)
		{
			Init();
		}

		void Init() noexcept override;
		FileError Load() noexcept override;
		FileError Save() const noexcept override;
	};
}
