#include "pch.h"
#include "WriteTemplateFile.hpp"
#include <fstream>
#include "PiPedalException.hpp"

using namespace pipedal;


void pipedal::WriteTemplateFile(
    const std::map<std::string,std::string> &map,
    std::filesystem::path inputFile,
    std::filesystem::path outputFile)
{
    std::ofstream out(outputFile);
    std::ifstream in(inputFile);

    while (in.peek() != -1)
    {
        char c = in.get();
        if (c == '$')
        {
            if (in.peek() == '{') {
                in.get();

                std::stringstream sName;
                try {
                    while ((c = in.get()) != '}')
                    {
                        sName.put(c);
                    }
                } catch (const std::exception&)
                {
                    throw PiPedalException("Unexpected end of file. Expecting '}'.");
                }
                std::string s = map.at(sName.str());
                out << s;
            } else if (in.peek() == '$')
            {
                out.put(in.get());
            } else {
                out.put(c);
            }
        } else {
            out.put(c);
        }
    }
}
