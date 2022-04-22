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

#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>


namespace pipedal
{

    using namespace std;

    class PrettyPrinter
    {
    private:
        std::ostream &s;
        size_t lineWidth = 80;
        std::vector<char> lineBuffer;
        size_t column = 0;
        size_t indent = 0;
        size_t tabSize = 4;
        bool hangingIndent = false;
        bool hungIndent = false;

    public:
        PrettyPrinter()
            : s(std::cout)
        {
            lineBuffer.reserve(120);
        }
        std::ostream&Stream() const { return s;}

        PrettyPrinter& Indent(size_t indent)
        {
            this->indent = indent;
            return *this;
        }

        void WriteLine() {
            lineBuffer.push_back('\n');
            s.write(&lineBuffer[0],lineBuffer.size());
            lineBuffer.clear();
            column = 0;
        }

        PrettyPrinter& HangingIndent()
        {
            hangingIndent = true;
            return *this;
        }
        void Write(const std::string &text)
        {
            for (char c : text)
            {
                if (c == '\n')
                {
                    WriteLine();
                    hungIndent = false;
                    hangingIndent = false;
                }
                else
                {
                    if (column <= indent)
                    {
                        if (hangingIndent)
                        {
                            hangingIndent = false;
                            hungIndent = true;

                        } else if (!hungIndent) {
                            if (c == ' ')
                            {
                                continue;
                            }
                            while (column < indent)
                            {
                                lineBuffer.push_back(' ');
                                ++column;
                            }
                        }
                    }
                    if (c == '\t')
                    {
                        if (hungIndent)
                        {
                            if (column < indent-1)
                            {
                                while (column < indent)
                                {
                                    lineBuffer.push_back(' ');
                                    ++column;
                                }
                            } else {
                                WriteLine();
                                column = 0;
                                while (column < indent)
                                {
                                    lineBuffer.push_back(' ');
                                    ++column;
                                }
                            }
                            hungIndent = false;
                        } else {
                            lineBuffer.push_back(' ');
                            ++column;
                            while ((column % tabSize) != 0)
                            {
                                lineBuffer.push_back(' ');
                                ++column;
                            }
                        }
                    }
                    else if ((c & 0x80) == 0)
                    {
                        lineBuffer.push_back(c);
                        ++column;
                    }
                    else
                    {
                        //utf-8 sequence.
                        lineBuffer.push_back(c);
                        if ((c & 0xC0) == 0x80)
                        {
                            ++column;
                        }
                    }

                    if (column >= lineWidth - 1)
                    {

                        size_t breakPos = FindBreak();

                        size_t startOfOverflow = breakPos;
                        while (startOfOverflow < lineBuffer.size() && lineBuffer[startOfOverflow] == ' ')
                        {
                            ++startOfOverflow;
                        }
                        std::string overflow = string(&lineBuffer[startOfOverflow], lineBuffer.size() - startOfOverflow);
                        lineBuffer.resize(breakPos);

                        WriteLine();

                        this->Write(overflow);
                    }
                }
            }
        }

    private:
        size_t FindBreak()
        {
            size_t i = lineBuffer.size();
            while (i > indent && lineBuffer[i-1] != ' ')
            {
                --i;
            }
            if (i > indent)
                return i;
            return lineBuffer.size();
        }
    };


    template <typename T> 
    PrettyPrinter& operator<< (PrettyPrinter &p, T &v)
    {
        std::stringstream s;
        s << v;
        p.Write(s.str());
        return p;
    }

    inline PrettyPrinter& operator << (PrettyPrinter& p,const std::string&s)
    {
        p.Write(s);
        return p;
    }

    // iomanip

    // template<typename _CharT, typename _Traits>
    // inline basic_ostream<_CharT, _Traits>&
    // endl(basic_ostream<_CharT, _Traits>& __os)
    // { return flush(__os.put(__os.widen('\n'))); }

    inline PrettyPrinter& operator << (PrettyPrinter& p,PrettyPrinter& (*iomanip)(PrettyPrinter&os))
    {
        return (*iomanip)(p);
    }

    using pp_manip = std::function<PrettyPrinter& (PrettyPrinter&)>;
    
    inline
    PrettyPrinter& operator << (PrettyPrinter&pp,pp_manip manip)
    {
        return manip(pp);
    }

    pp_manip Indent(int n)
    {
        return [n] (PrettyPrinter&pp) ->PrettyPrinter& {
            pp.Indent(n);
            return pp;
        };
    }
    pp_manip HangingIndent()
    {
        return [] (PrettyPrinter&pp)  ->PrettyPrinter&{
            pp.HangingIndent();
            return pp;
        };
    }


}