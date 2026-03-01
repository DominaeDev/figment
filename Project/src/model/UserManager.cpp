#include <pch.h>
#include "model/UserManager.h"

#include <random>
#include <chrono>
#include <filesystem>
#include <cassert>
#include <print>
#include <set>

#include "util/Common.h"
#include "util/Hash.h"
#include "util/Security.h"
#include "util/ProfileDatabase.h"

#include "fs/Xml.h"
#include "model/AssetManager.h"
#include "model/GlobalStrings.h"

using namespace fig::security;
using namespace fig::common_util;

namespace fig::fs
{
	static Bit128 kDefaultAuthKey { 
		fig::byte { 0xA1 }, fig::byte { 0xB2 }, fig::byte { 0xC3 }, fig::byte { 0xD4 }, fig::byte { 0xE5 }, fig::byte { 0xF6 }, fig::byte { 0xAB }, fig::byte { 0xCD },
		fig::byte { 0xEF }, fig::byte { 0x1A }, fig::byte { 0x2B }, fig::byte { 0x3C }, fig::byte { 0x4D }, fig::byte { 0x5E }, fig::byte { 0x6F }, fig::byte { 0xF0 },
	};
	static Bit128 kDefaultAuthSalt { 
		fig::byte { 0xEF }, fig::byte { 0x1A }, fig::byte { 0x2B }, fig::byte { 0x3C }, fig::byte { 0x4D }, fig::byte { 0x5E }, fig::byte { 0x6F }, fig::byte { 0xF0 },
		fig::byte { 0xA1 }, fig::byte { 0xB2 }, fig::byte { 0xC3 }, fig::byte { 0xD4 }, fig::byte { 0xE5 }, fig::byte { 0xF6 }, fig::byte { 0xAB }, fig::byte { 0xCD },
	};

	constexpr std::array<fig::string_view, 8> RECOVERY_CODE_LUT {
		"B2S3IRWLVAFTW7GFU1QG9JIH45GBVBP6TJ8GXNX2525TS5V6U9MQFSHHAI85PPJ96HKJLRVNWIPW14CTE759R7JXIGGM55J8KFX4BDB9QVSRAPXKT1TDQM2LRK6LS1MLVIJKRAH7PEQLPGD4BG75CXJJQC37Q28HTX43SLDCPIP2DL5XGWRH61KRDX5BR9FUUX56U529D6BPN1NUH85SL872R2LVPHX8M2QSF71HR3UGGR5KE9NPEB1BRX1TGD2IW8IUPDTG6EQPVXNQ65QRU3CV5F9T18EQEHMD1N67EWD1X9UA68MUNJ6NE1PNRUHE8KX3MRF2WBPJSU5N6VJFM71KJG8LBI33574L41HD47XFQQNDBTD3FA7NMS44JPHBLFA67HDTD23R1JUWHF9V7QTRQKI6SJGUFWU23IRBP3Q7EPK9EFLBA5XASPFHHXBNNIS8DNMP2A1EE8B55D63IA4M3J6KMK1RGKXHLKDGDU7EF3NVVCT6SNRD2REJCC1A",
		"HBBUPBU4ILK94EWT5U4PNKGQJ56GCHNHMAQ3L2MKLFSW8I3RR963GR5T14UFS5A9DCVMSKSRIEHW7WC7R7R8DUURH3SQ6CKDB8P59IKAQSN66HRN6E49HHAK261ET5VU41QKNIN19FEELNB7WWD7QRH6F4RUQ6FLRI4H5I1DXC1PTM3XAG7JJIS2R52DE574W7IHU3GWS6VS1HH1XB7R3AUK2XGV62U8QLF7HVMT8TXT3HVX13FP1CQNBCLS6LC5784LNX8RDFVEK8INN3FBEQ8J2EHTAQ1VDR18T8UEITF86AWMV2UA86NUP93JKSA1H2X156KCBW9NWHIIABG7I1P7DGG3E62R46NTNSX3H7GXEH2J65IA22A8W8M727J3WQKQFHE7F5A3UM3P2IJNIWM4TLDX4FTSBFQFKMEPD9CJ2WMWBXXP2UVH1KJ6SSPHSAQAHN9L1BGEMV5SSNMGDBV65RP3FCHCHA7UAXPR3I9WU9BSX2MXJPV37PAWLG17",
		"I84SF9PJBW8UM3RMA5CNSC9X8MT863CMC9GCA95P3J2IE18LKHQG7EG1NACPT7P3FAUSX2QUFCRFQ4JG68XCL2TD7MPN2KQHVL3EIFG3QPN4DCA2JX3XN3D38B3A7KHGKPP8WWQ3URNCM1PUC548K1BI5FUM9G7GW8QMRTNX4BMXICEMA1G2NE2272VUU3CGXQRHUHIGKLACBEVR5BIU21RAI2NBJ7EENTF5J38HFX7CGGUN4FVQDI9IB9PHMEVV9LBCW5868PNJEBSD1LHVWAF3FWFDF4IXAWR2X5WRP5FNLF5RAAHETSV5SHGKTJW1GMCFULH7SW3NEKH5D5BXNFBN5DUA7H6BS5EPCS37WC9ER9U6WXEJE428AVKT16HNVXXFIWNUT6NS4KPMEQQX7JKV3BLKAKU1KMDU2MN24D5ILQXMMKKDA44RV26F8XECBAPRM942MBHXQ1CERRJK2XN59PL6JBGXKJCURU8FNG5MIH76DXCJKCST4Q246N5A",
		"LEU9GDM7LDCXE3PIHXDV1H93KP43UUPSEXDL2QNQ2XR8TQJT1RMDSE2AJNFFHTGN63J6481K52TM2U6P1UTGDN4M9R7WWL2N8LSQTE4K4RWKIIMPWH1EEHGLKJ8EA551QW5V1I3VATBDG9S8RWWIFLNR1MBIGA9MIT2B726UPFXAMR3XXMGC2K9PSBIB19HWW94V69PTSAAGEB46XFAS3TJ7T2MQJBN7JM73J2AJDTCQUK6QI8B5P3N6I6I2WTKRWSNTX41PF89W3F4GA1IX8QL1BEWEDIIRX2XUCCNKW52G2D4F54SN71UDAL6E5P6GIHF684GB6A98P5RRUVBG6FV5N1XJQ1VJ6MGS9F6D5U3B9A36AM6KCJ78B31BV2P8M5CPX9PPGWFC372CILRDINI1CHFX66IFIP8FP4MCRJPNGUFUV3DSHDDCKGAASJ39GML7H7LI4NXSUSH3KQJU88UC3H1CFKBB6I4BW3GJUQ9TK7TKNIT6R9HUBHRE74VQ",
		"J7PS8RAN3SNAJBUUJRCAQAJFETJ1XSJK4BF8WGM9IVD1EPAAACMJE2WWRI1E7GLE9FMBEE6I88D2E6V96QHFVI44CXKMKNLK1LJEPXT77B2T3QA1BAW81996M27DU47WLD6TG1M1UHB6V3TU5BWNMFPA9EX58377UJDALBVLL9VK536FARDFT52FA28KP29N4CAVB21UDD69NEFQMRFKS58XX3HCKPB9AJRVKVM39P7HFBTQ1CVHTHBEFHAU2VWV8ITXGS239IHGHRD4IQQVAD6MTS72A9TB6HDRE58FL2RWJ2WTP96AI6RU861IJS6LDTLHLQ1787K9CG7RS6AL13F3S8VJSRCWIEEFDH71JTGIL3RA5TMCXUX1JG8J8E3F51KLMS5XVQR66K8S1WP1XTVCR1F245BBUQX8HKJVFSL8GG3BPKXKKJ3AKWBSU1R8FPHS3HA5BLKR5KKSSTKFHIRTQP5PEDNFBV249V4PEG4UQX4FPVS29446RN54RKXX",
		"ENC6WHF1T94TNT534WBDCH5AF85E6X8RFIEBQP18T1TXGJWQTTNRSXRFG8DLBKPT4C38BIPBIQFDCL8F5SX41S6F6I7H377KNWLK2H6JFKF67PB5ESTRIIRWR6BQEX4R5D6P95PSIH8MLA7AG3SMGFDTSHRCDU1C4DU3T7XRITJW5LD6LNSAC3RDHLIUXVFA7WDRJ3IPNSNBTSGTMCJEIFFGL2HBDIC77QP4GKWMPF1ESSVRPNH38A43X22U77QF6HCFK2CJNLM6E9IWS26A6NHQCRMKA12M3VWSGGGL36XDAX88JI6WH8MM133E3RR3NALC8K5FMAA9ND4V2257P6VF3Q6LX57G5XJH8VK5VDB6IK1G7MLJPLVCM5DXPQ8IVIMXBCNUUCB71VN3XFE8HKT299WVAMVB9VUTDAJDDWH4P1HCKK61D3QMU7UL8W764KERT4V5D2I8QJG52873AF1K2W9CEDUEBJIG4IU9BALMD7TBV26KQQXX3PQ3VWQ2",
		"23RJH3SP249RDDAIIR1RKUW5QUGQ7FN9EV82TERNIDQV6Q9H1FTHV6TS89RRV1VQ6K9J9TGGQQ3PM7M6L8SH7EDLQGTM8WXUPSSK3466D3CG447RUVN2U3F5UNBGFTTG76FKMC1K865H6RXI2UUW7LR7VGK6AHQ9WITUNN4LD26TSDEIF6DSM5CIS6DCX4G9N536CSNHJP5D1A1XMNIWJ43AHATJH7IPJ8VKSQK7N6R1EW1C8H5WW7RHUM932QW8KS71KVGAVBLGILRIEMBWS7VXW1RF7XIXLITKL6B9AU8EQ2EP8KU69XCKNVNK4CTL973MGSIJQLLTKD4VFEIGKJFFXCL5MDF8ITI9GTSFQFXXTWL3DVD93BWPT3JS5M65T2G37P88TF2PN4RS9E6E5T1JVE3TCP9L3UFCXM7AE1NC4UFPE8TCBMVTMT47HP8161GNCEHFHCA6GKC18XR6E49IS44QCCBRLRLW1D7M5GT72GM3TI574RDU2742ADKI",
		"3HVXMGI5V6CM5RMFMVQ1KFUGVVLVQ3G47FD92ARUMXQSU8FIDJP13M24AQ8KNRTU4DDSST3BDHW9X23DNWK8DTFG1LN1BAB47T5QUTPMEDQXRDE7QUGAXGLKQ5MKCEIPMPJMM3GBPWU162FHIAMJRX7EHWE288K2HCT5WWCBX5SBC4KJTTT6AAINWLP85XT7BFCUPXW71R971PFC9VNBKWL4JWKA4329XHW1D8EEXPHNG2F5MRUF7AUA3NS87535GFQHIEPSBEHGQKL6PDEGR8V8RGJUS6IUP54EDC8QTPPP7QXLEKH9FKU9L5JP7H7JGI5WI1SH716C7DW2NPER64RJAVLFPKA6IKLULHM989G66LN4WDKVUEPVWS26GD3WBCBLUU5UD4NIWV67538XVM5KF81WAGEINV6N4J9F2XVL572PX8PT79U7HHHLQ7B37RTLGN1D8CNSFDEQF9G7UV9N3UCNL7P3APVT2V3PSJFFJV968VEJND1BX147JI6J",
	};

	static AuthChallenge CreateAuthChallenge(Bit128 key, Bit128 salt, AuthKey encKey)
	{
		AuthChallenge challenge;
		std::memcpy(challenge.data() + ptrdiff_t(0), key.data(), key.size());
		for (size_t i = 0; i < 16uz; ++i)
			challenge[i + 16uz] = challenge[i] ^ salt[i];
		auto encrypted = Encrypt(challenge, encKey);
		std::memcpy(challenge.data(), encrypted.data.data(), challenge.size());
		return challenge; // rvo
	}
	
	static bool Verify(const fig::string& text)
	{
		std::set<fig::string> used {};
		for (size_t i = 0; i < text.size() / 2; ++i)
		{
			fig::string bit = text.substr(i * 2, 2);
			used.insert(bit);
		}
		return used.size() == 256uz;
	}

	static void GenerateLUT()
	{
		static constexpr fig::const_string Symbols { "123456789ABCDEFGHIJKLMNPQRSTUVXW" };
		static std::mt19937_64 rng { std::default_random_engine{}() };
		static std::uniform_int_distribution<size_t> dist(0, Symbols.size() - 1);

		fig::string lut;
		lut.reserve(512);

		std::set<fig::string> used {};
		char tmp[3] { 'x', 'x', '\0' };
		for (size_t i = 0; i < 256; ++i)
		{
			while (true)
			{
				tmp[0] = Symbols[dist(rng)];
				tmp[1] = Symbols[dist(rng)];
				if (used.contains(tmp))
					continue;
				break;
			}
			used.insert(tmp);
			lut.append(tmp);
		}

		if (Verify(lut))
			std::println("{}", lut);
	}

	UserManager::UserManager()
	{
	}

	std::optional<UserProfileCRef> UserManager::CreateDefaultProfile()
	{
		return CreateProfile(fig::string(fig::strings::UserProfile::DefaultUser), "");
	}

	std::optional<UserProfileCRef> UserManager::CreateProfile(const fig::string& name, const fig::string& password)
	{
		auto authKey = Random128Bits();
		auto authSalt = not password.empty() ? Random128Bits() : kDefaultAuthSalt;
		auto encKey = not password.empty() ? DeriveKeyFromPassword(password, authSalt) : kDefaultAuthKey;

		auto id = CreateUUID();
		auto& db = GetDatabase();

		// Create auth challenge
		AuthChallenge authChallenge = CreateAuthChallenge(authKey, authSalt, encKey);

		UserProfile profile {
			.id = id,
			.name = name,
			.auth = UserAuth {
				.challenge = authChallenge,
				.salt = authSalt,
			},
		};

		// Generate recovery key
		AuthKey recoveryKey;
		AuthChallenge recoveryChallenge;
		if (CreateRecoveryFile(profile, password, recoveryChallenge, recoveryKey))
		{
			profile.recovery = UserAuth {
				.challenge = recoveryChallenge,
				.salt = authSalt,
			};

			auto code = RecoveryKeyToCode(recoveryKey);
			LogLn(std::format("Recovery code for user {}: {}", profile.id.str(), code));
		}

		if (db.CreateProfile(profile) == DatabaseError::NoError)
		{
			_profiles.emplace_back(profile);
			return std::make_optional(std::cref(_profiles.back()));
		}

		return std::nullopt;
	}

	static bool __Authenticate(const AuthChallenge& challenge, const AuthSalt& salt, const AuthKey& key, AuthKey& outKey)
	{
		auto decrypted = Decrypt(challenge, key);

		// Validate
		if (decrypted.size() != sizeof(AuthChallenge))
			return false;

		for (size_t i = 0; i < 16; ++i)
		{
			if (decrypted[i + 16uz] != (decrypted[i] ^ salt[i]))
				return false; // Invalid password
		}

		std::memcpy(outKey.data(), decrypted.data(), outKey.size());
		return true;
	}

	bool UserManager::Authenticate(const UserProfile& profile, const fig::string& password, AuthKey& outKey)
	{
		auto encKey = not password.empty() ? DeriveKeyFromPassword(password, profile.auth.salt) : kDefaultAuthKey;
		return __Authenticate(profile.auth.challenge, profile.auth.salt, encKey, outKey);
	}

	bool UserManager::Authenticate(const AuthChallenge& challenge, const AuthSalt& salt, const AuthKey& key, AuthKey& outKey)
	{
		auto encKey = DeriveKeyFromBytes(key, salt);
		return __Authenticate(challenge, salt, encKey, outKey);
	}

	bool UserManager::SignIn(const fig::string& profileName, const fig::string& password)
	{
		auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&profileName](const UserProfile& profile) {
			return profile.name == profileName;
		});

		if (itProfile == _profiles.cend())
			return false; // Not found

		return SignIn(*itProfile, password);
	}

	bool UserManager::SignIn(const fig::uuid& profileID, const fig::string& password)
	{
		auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&profileID](const UserProfile& profile) {
			return profile.id == profileID;
		});

		if (itProfile == _profiles.cend())
			return false; // Not found

		return SignIn(*itProfile, password);
	}

	bool UserManager::SignIn(UserProfile& profile, const fig::string& password)
	{
		if (not Authenticate(profile, password, _signedInAuthKey))
			return false; // Incorrect password

		_signedInProfile = &profile;
		_pAssetMngr = std::make_unique<AssetManager>(*this);
		_pContentDatabase = std::make_unique<ContentDatabase>(*_pAssetMngr.get());

		return true;
	}

	bool UserManager::SignInDefaultProfile()
	{
		return SignIn(fig::string(fig::strings::UserProfile::DefaultUser), "");
	}

	bool UserManager::SignOut()
	{
		if (not IsSignedIn())
			return false;

		_pContentDatabase.reset();

		_pAssetMngr->SaveModified();
		_pAssetMngr.reset();

		_signedInProfile = nullptr;
		_signedInAuthKey = AuthKey {};
		return true;
	}

	bool UserManager::LoadProfiles()
	{
		auto& db = GetDatabase();
		if (auto profiles = db.FetchProfiles(); profiles.has_value())
		{
			_profiles = std::move(profiles.value());
			return not _profiles.empty();
		}
		else
			return false;
	}

	bool UserManager::ChangePassword(const fig::uuid& profileID, const fig::string& oldPassword, const fig::string& newPassword)
	{
		auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&profileID](const UserProfile& profile) {
			return profile.id == profileID;
		});

		if (itProfile == _profiles.cend())
			return false; // Not found

		auto& profile = *itProfile;

		AuthKey authKey;
		if (not Authenticate(profile, oldPassword, authKey))
			return false;

		AuthKey newPasswordKey = not newPassword.empty() ? DeriveKeyFromPassword(newPassword, profile.auth.salt) : kDefaultAuthKey;

		auto newChallenge = CreateAuthChallenge(authKey, profile.auth.salt, newPasswordKey);

		auto& db = GetDatabase();
		if (db.UpdateProfile(UserProfile {
			.id = profile.id,
			.name = profile.name,
			.auth = UserAuth {
				.challenge = newChallenge,
				.salt = profile.auth.salt,
			},
			.recovery = {},
		}) == DatabaseError::NoError)
		{
			// Update local profile
			profile.auth.challenge = newChallenge;
			return true;
		}
		return false;
	}

	const UserProfile& UserManager::GetActiveProfile() const
	{
		if (_signedInProfile == nullptr)
			throw std::runtime_error("Not signed in");

		return std::cref(*_signedInProfile);
	}

	AssetManager& UserManager::GetProfileAssets()
	{
		if (_signedInProfile == nullptr || !_pAssetMngr)
			throw std::runtime_error("Not signed in");

		return static_cast<AssetManager&>(*_pAssetMngr);
	}

	ContentDatabase& UserManager::GetContent()
	{
		if (_signedInProfile == nullptr || !_pContentDatabase)
			throw std::runtime_error("Not signed in");

		return static_cast<ContentDatabase&>(*_pContentDatabase);
	}

	fig::user::ProfileDatabase& UserManager::GetDatabase() noexcept
	{
		if (!_pProfileDB)
			_pProfileDB = std::make_unique<fig::user::ProfileDatabase>(fig::path(std::format("{}/{}.{}", Constants::Paths::ProfilesFolder, Constants::Paths::ProfilesFileName, Constants::Paths::ProfilesFileExt)));
		return *_pProfileDB.get();
	}

	bool UserManager::CreateRecoveryFile(const UserProfile& profile, const fig::string& password, AuthChallenge& recoveryChallenge, AuthKey& recoveryKey)
	{
		AuthKey authKey;
		if (Authenticate(profile, password, authKey) == false)
			return false;

		auto& salt = profile.auth.salt;
		recoveryKey = Random128Bits();
	
		AuthKey encKey = DeriveKeyFromBytes(recoveryKey, salt);
		recoveryChallenge = CreateAuthChallenge(authKey, salt, encKey);
		
		return BinaryWriter::WriteRecoveryFile(profile, recoveryChallenge) == FileError::NoError;
	}

	bool UserManager::RecoverProfile(const fig::uuid& profileID, const fig::string& recoveryCode)
	{
		AuthKey recoveryKey;
		if (RecoveryCodeToKey(recoveryCode, recoveryKey))
			return RecoverProfile(profileID, recoveryKey);
		return false;
	}

	bool UserManager::RecoverProfile(const fig::uuid& profileID, const AuthKey& recoveryKey)
	{
		auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&profileID](const UserProfile& profile) {
			return profile.id == profileID;
		});

		if (itProfile == _profiles.cend())
			return false; // Not found

		auto& profile = *itProfile;
		if (is_empty(std::span { profile.recovery.challenge.data(), profile.recovery.challenge.size() }))
			return false;

		// Recover key
		AuthKey authKey;
		if (Authenticate(profile.recovery.challenge, profile.recovery.salt, recoveryKey, authKey))
		{
			// Reset password
			auto newChallenge = CreateAuthChallenge(authKey, kDefaultAuthSalt, kDefaultAuthKey);

			auto& db = GetDatabase();
			if (db.UpdateProfile(UserProfile {
				.id = profile.id,
				.name = profile.name,
				.auth = UserAuth {
					.challenge = newChallenge,
					.salt = kDefaultAuthSalt,
				},
				.recovery = {},
				}) == DatabaseError::NoError)
			{
				// Update local profile
				profile.auth.challenge = newChallenge;
				profile.auth.salt = kDefaultAuthSalt;
				return true;
			}
			return false;
		}
		return false;
	}

	fig::string UserManager::RecoveryKeyToCode(const AuthKey& key) noexcept
	{
		fig::string code;
		code.reserve(35);

		char tmp[2] { 'x', 'x' };
		const uint8_t* pKey = reinterpret_cast<const uint8_t*>(key.data());
		for (size_t i = 0; i < key.size(); ++i)
		{
			if (i > 0 and i % 4 == 0)
				code.append(" ");

			auto& lut = RECOVERY_CODE_LUT[i % size(RECOVERY_CODE_LUT)];
			assert(lut.size() == 512);
			std::memcpy(tmp, lut.data() + ptrdiff_t((uint16_t)key[i] * 2), 2uz);
			code.append(tmp, 2uz);
		}
		return code;
	}

	bool UserManager::RecoveryCodeToKey(const fig::string& code, AuthKey& outKey) noexcept
	{
		fig::string formatted = code
			| std::views::filter([](auto& c) { return std::isalnum((int)c); })
			| std::views::transform([](auto& c) { return (char)std::toupper((int)c); })
			| std::ranges::to<fig::string>();

		if (formatted.size() != sizeof(AuthKey) * 2)
			return false;

		for (size_t i = 0; i < 16uz; ++i)
		{
			char ch[2] { formatted[i * 2 + 0], formatted[i * 2 + 1] };
			auto& lut = RECOVERY_CODE_LUT[i % size(RECOVERY_CODE_LUT)];
			assert(lut.size() == 512);
			uint16_t n = 0;
			while (n < 512 and not (lut[n + 0] == ch[0] and lut[n + 1] == ch[1]))
				n += 2;
			if (n == 512)
				return false; // Error
			outKey[i] = static_cast<std::byte>(n / 2);
		}
		return true;
	}
}