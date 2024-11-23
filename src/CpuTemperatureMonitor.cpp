// Copyright (c) 2022 Robin Davies
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include "CpuTemperatureMonitor.hpp"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <optional>
#include <system_error>

namespace fs = std::filesystem;
using namespace pipedal;



static constexpr std::string_view THERMAL_PATH = "/sys/class/thermal/";


static std::optional<std::string> readFile(const fs::path& path) {
    std::ifstream file(path);
    if (!file) {
        return std::nullopt;
    }
    
    std::string content;
    std::getline(file, content);
    return content;
}


static bool isCpuThermal(const fs::path &zone_path) {
    auto type = readFile(zone_path / "type");
    return type && (*type == "cpu-thermal") || (*type == "x86_pkg_temp");
}




class CpuTemperatureMonitorImpl: public CpuTemperatureMonitor {
private:
    std::vector<fs::path> cpuZones;
public:
    explicit CpuTemperatureMonitorImpl(std::vector<fs::path>&&cpuZones) 
    : cpuZones(std::move(cpuZones)) {
    }


    virtual float GetTemperatureC() override {
        float result = INVALID_TEMPERATURE*1000;

        for (const auto&cpuZone: cpuZones)
        {

            std::ifstream file(cpuZone / "temp");
            if (file) {
                float v;
                file >> v;
                if (file)
                {
                    if (v > result)
                    {
                        result = v;
                    }
                }
            }
        }
        return result/1000;
    }
};

static std::vector<fs::path> findCpuThermalZones() {
    std::vector<fs::path> cpu_zones;
    std::error_code ec;
    
    for (const auto& entry : fs::directory_iterator(THERMAL_PATH, ec)) {
        if (ec) {
            std::cerr << "Error reading directory: " << ec.message() << '\n';
            continue;
        }
        
        const auto& path = entry.path();
        if (path.filename().string().find("thermal_zone") == 0) {
            if (isCpuThermal(path)) {
                cpu_zones.push_back(path);
            }
        }
    }
    return cpu_zones;
}


CpuTemperatureMonitor::ptr CpuTemperatureMonitor::Get()
{

    std::vector<fs::path> cpuZones = findCpuThermalZones();

    return std::make_unique<CpuTemperatureMonitorImpl>(std::move(cpuZones));
}
