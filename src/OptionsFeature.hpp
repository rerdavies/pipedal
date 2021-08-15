#pragma once


#include "MapFeature.hpp"
#include "lv2/lv2plug.in/ns/ext/options/options.h"

namespace pipedal {


    class OptionsFeature {

	private:
        float sampleRate;
        int32_t blockLength;
        int32_t atomBufferBlockLength;

		LV2_Feature feature;
		LV2_Options_Option options[6];

	public:
        OptionsFeature();
        void Prepare(MapFeature&map,
                double sampleRate,
                int32_t blockLength, 
                int32_t atomBufferBlockLength);
        ~OptionsFeature();
	public:
		const LV2_Feature* GetFeature()
		{
			return &feature;
		}
    };

}