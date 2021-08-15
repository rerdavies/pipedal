#pragma once

#include "PedalBoard.hpp"
#include "json.hpp"

namespace pipedal {

class PedalPreset {
private:
    int instanceId_ = -1;
    std::vector<ControlValue> values_; 
public:
    DECLARE_JSON_MAP(PedalPreset);
};

class PedalBoardPreset {
    long instanceId_;
    std::string displayName_;
    std::vector<std::unique_ptr<PedalPreset> > values_;
public:
    DECLARE_JSON_MAP(PedalBoardPreset);
};

class PedalBoardPresets {
    long nextInstanceId_ = 0;

    long currentPreset_ = 0;
    std::vector<std::unique_ptr<PedalBoardPreset> > presets_;
public:
    DECLARE_JSON_MAP(PedalBoardPresets);

};

} // namespace.