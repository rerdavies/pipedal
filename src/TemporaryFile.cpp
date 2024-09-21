// Copyright (c) 2024 Robin Davies
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

#include "TemporaryFile.hpp"
#include <fstream>

using namespace pipedal;


TemporaryFile::TemporaryFile(const std::filesystem::path&directory)
{
    namespace fs = std::filesystem;
    fs::create_directories(directory);

    // Generate a unique filename
    std::string filename;
    do {
        std::string random_string(8, '\0');
        const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        
        srand(static_cast<unsigned int>(time(nullptr)));
        for (int i = 0; i < 8; ++i) {
            random_string[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        }
        
        filename = directory / ("temp_" + random_string + ".tmp");
    } while (fs::exists(filename));

    // Create the file
    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("Failed to create temporary file");
    }
    file.close();

    this->path = filename;

}
TemporaryFile::~TemporaryFile()
{
    if (!path.empty())
    {
        std::filesystem::remove(path);
    }
}

