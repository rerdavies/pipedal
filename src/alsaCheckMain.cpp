#include <alsa/asoundlib.h>
#include <iostream>
#include <vector>
#include <string>
#include "CommandLineParser.hpp"
#include "alsaCheck.hpp"

using namespace std;
using namespace pipedal;


int main(int argc, char* argv[]) {

    bool listDevices = false;

    try {
        CommandLineParser commandLineParser;

        std::string device = "";

        commandLineParser.AddOption("--list",&listDevices);

        commandLineParser.Parse(argc,(const char**)argv);

        if (commandLineParser.Arguments().size() >= 1) {
            device = commandLineParser.Arguments()[0];
        }
        if (listDevices || device.empty()) {
            list_alsa_devices();
        } else {
            check_alsa_channel_configs(device.c_str());
        }
        return EXIT_SUCCESS;
    } catch (const std::exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}