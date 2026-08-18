#include <pch.h>
#include "tts/AudioServerConfiguration.h"
#include <json.hpp>

namespace fig::tts
{
	fig::string AudioServerConfiguration::ToJson() const noexcept
	{
		try
		{
			nlohmann::json jConfig;
			jConfig["host"] = "127.0.0.1";
			jConfig["port"] = Constants::TTS::ServerPort;
			switch (backend)
			{
			default:
			case Backend::CPU: jConfig["backend"] = "cpu"; break;
			case Backend::CUDA: jConfig["backend"] = "cuda"; break;
			}
			jConfig["device"] = 0;
			jConfig["threads"] = 2;
			jConfig["lazy_load"] = true;
			jConfig["log_request_body"] = false;
			jConfig["max_request_body_bytes"] = std::numeric_limits<int32_t>::max();

			nlohmann::json jModels = nlohmann::json::array();
			for (auto& model : models.models)
			{
				for (auto& variant : model.variants)
				{
					nlohmann::json jModel = nlohmann::json::object();
					jModel["id"] = (fig::string)variant.id;
					jModel["family"] = model.family;
					jModel["path"] = std::format("models/{0}", variant.filename);
					jModel["task"] = model.task.id;
					jModel["mode"] = "offline";
					jModel["busy_timeout_ms"] = 60000;

					nlohmann::json load_options = nlohmann::json::object();
					load_options["language"] = model.supportedLanguages[0].id;
					jModel["load_options"] = load_options;

					nlohmann::json session_options = nlohmann::json::object();
					session_options["language"] = model.supportedLanguages[0].id;
					jModel["session_options"] = session_options;

					nlohmann::json default_request_options = nlohmann::json::object();
					if (model.parameters.temperature != 0_fp)
						default_request_options["temperature"] = static_cast<double>(model.parameters.temperature);
					if (model.parameters.guidance != 0_fp)
						default_request_options["guidance"] = static_cast<double>(model.parameters.guidance);
					if (model.parameters.topK != 0)
						default_request_options["top-k"] = model.parameters.topK;
					if (model.parameters.topP != 0_fp)
						default_request_options["top-p"] = static_cast<double>(model.parameters.topP);
					if (model.parameters.repetitionPenalty != 0_fp)
						default_request_options["repetition-penalty"] = static_cast<double>(model.parameters.repetitionPenalty);
					if (model.parameters.chunkSize != 0)
						default_request_options["text-chunk-size"] = model.parameters.chunkSize;
					jModel["default_request_options"] = default_request_options;
					jModels.push_back(jModel);
				}
			}
			jConfig["models"] = jModels;

			return jConfig.dump();
		}
		catch (const nlohmann::json::exception&)
		{
			return "{}";
		}
	}
}