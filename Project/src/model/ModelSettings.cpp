#include <pch.h>
#include "model/ModelSettings.h"
#include "Constants.h"

namespace fig::llm
{
	ModelSettings::ModelSettings() :
		contextSize { Constants::DefaultModelSettings::ContextSize },
		contextWindowKeepRatio { Constants::DefaultModelSettings::ContextWindowKeepRatio },
		gpuLayers { Constants::DefaultModelSettings::GPULayers },
		bUseMlock { Constants::DefaultModelSettings::UseMlock },
		bUseMmap { Constants::DefaultModelSettings::UseMmap },
		microBatchSize { Constants::DefaultModelSettings::MicroBatchSize },
		maxSequences { Constants::DefaultModelSettings::MaxSequences }
	{}
}