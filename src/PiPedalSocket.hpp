#pragma once

#include "BeastServer.hpp"
#include "PiPedalModel.hpp"


namespace pipedal {

    std::shared_ptr<ISocketFactory> MakePiPedalSocketFactory(PiPedalModel &model);
};

