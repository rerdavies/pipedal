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


#include "WriteTemplateFile.hpp"
#include <fstream>
#include <stdexcept>

using namespace pipedal;

void pipedal::WriteTemplateFile(
    const std::map<std::string, std::string> &map,
    const std::filesystem::path &outputFile)
{
    std::string inputFile = "/etc/pipedal/config/templates/" + outputFile.filename().string() + ".template";
    WriteTemplateFile(map, inputFile,outputFile);
}

void pipedal::WriteTemplateFile(
    const std::map<std::string, std::string> &map,
    const std::filesystem::path &inputFile,
    const std::filesystem::path &outputFile)
{
    std::ofstream out(outputFile);
    std::ifstream in(inputFile);

    if (!in.is_open())
    {
        throw std::runtime_error("File not found: " + inputFile.string());
    }

    while (in.peek() != -1)
    {
        char c = in.get();
        if (c == '$')
        {
            if (in.peek() == '{')
            {
                in.get();

                std::stringstream sName;
                try
                {
                    while ((c = in.get()) != '}')
                    {
                        sName.put(c);
                    }
                }
                catch (const std::exception &)
                {
                    throw std::runtime_error("Unexpected end of file. Expecting '}'.");
                }
                std::string s = map.at(sName.str());
                out << s;
            }
            else if (in.peek() == '$')
            {
                out.put(in.get());
            }
            else
            {
                out.put(c);
            }
        }
        else
        {
            out.put(c);
        }
    }
}
