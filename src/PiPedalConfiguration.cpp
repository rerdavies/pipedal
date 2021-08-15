#include "pch.h"
#include "PiPedalConfiguration.hpp"
#include "json.hpp"


using namespace pipedal;

JSON_MAP_BEGIN(PiPedalConfiguration)
JSON_MAP_REFERENCE(PiPedalConfiguration, lv2_path)
JSON_MAP_REFERENCE(PiPedalConfiguration, local_storage_path)
JSON_MAP_REFERENCE(PiPedalConfiguration, mlock)
JSON_MAP_REFERENCE(PiPedalConfiguration, reactServerAddresses)
JSON_MAP_REFERENCE(PiPedalConfiguration, socketServerAddress)
JSON_MAP_REFERENCE(PiPedalConfiguration, threads)
JSON_MAP_REFERENCE(PiPedalConfiguration, logLevel)
JSON_MAP_REFERENCE(PiPedalConfiguration, logHttpRequests)
JSON_MAP_REFERENCE(PiPedalConfiguration, maxUploadSize)
JSON_MAP_REFERENCE(PiPedalConfiguration, accessPointGateway)
JSON_MAP_REFERENCE(PiPedalConfiguration, accessPointServerAddress)
JSON_MAP_END()
