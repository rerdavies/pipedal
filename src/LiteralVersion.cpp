/*
 * MIT License
 *
 * Copyright (c) 2022 Robin E. R. Davies
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "LiteralVersion.hpp"
#include <sstream>
#include <cctype>

using namespace pipedal;

LiteralVersion::LiteralVersion()
{
}
LiteralVersion::LiteralVersion(const std::string &version)
{
    Parse(version);
}

bool LiteralVersion::Parse(const std::string &version)
{
    int ix = 0;

    while (ix < version.length())
    {
        char c = version[ix];
        int n = 0;
        if (std::isdigit(c))
        {
            while (std::isdigit(c))
            {
                c = version[ix++];
                n = c -'0' + n;
            }
            this->versionNumbers.push_back(n);

            if (version[ix] == '.')
            {
                ++ix;
            }
        } else {
            std::stringstream t;

            while (ix < version.length())
            {
                c = version[ix++];
                t << c;
            }
            suffix = t.str();            
        }
    }

    return true;
}
