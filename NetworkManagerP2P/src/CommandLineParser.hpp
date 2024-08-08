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

#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

class CommandLineException : public std::runtime_error
{
public:
    using base = std::runtime_error;
    CommandLineException(const std::string &error)
        : base(error)
    {
    }
};
class CommandLineParser
{
private:
    class OptionBase
    {
        std::string option;

    public:
        OptionBase(const std::string &option_)
            : option(option_)
        {
        }
        virtual ~OptionBase()
        {
        }

    protected:
        const std::string &GetName() const { return option; }

    public:
        bool Matches(const std::string &text) const
        {
            return option == text;
        }
        virtual int Execute(int argcRemaining, const char **argvRemaining) = 0;
    };
    std::vector<OptionBase *> options;

    class BooleanOption : public OptionBase
    {

        bool *pOutput;
        bool value;

    public:
        BooleanOption(const std::string &name, bool *pOutput, bool value)
            : OptionBase(name),
              pOutput(pOutput),
              value(value)
        {
        }
        virtual int Execute(int argcRemaining, const char **argvRemaining)
        {
            *pOutput = value;
            return 0;
        }
    };

    template <typename T>
    class Option : public OptionBase
    {

        T *pOutput;

    public:
        Option(const std::string &name, T *pOutput)
            : OptionBase(name),
              pOutput(pOutput)
        {
        }
        virtual int Execute(int argcRemaining, const char **argvRemaining)
        {
            if (argcRemaining == 0)
            {
                throw CommandLineException("Expecting a parameter for option " + GetName());
            }
            std::stringstream s(argvRemaining[0]);
            try
            {
                s >> *pOutput;
                if (s.fail() || !s.eof())
                {
                    throw std::exception();
                }
            }
            catch (const std::exception &e)
            {
                throw CommandLineException("Argument for option " + GetName() + " is not in the correct format.");
            }
            return 1; // consumed one argument.
        }
    };

    class StringOption : public OptionBase
    {

        std::string name;
        std::string *pOutput;

    public:
        StringOption(const std::string &name, std::string *pOutput)
            : OptionBase(name),
              pOutput(pOutput)
        {
        }
        virtual int Execute(int argcRemaining, const char **argvRemaining)
        {
            if (argcRemaining == 0 || argvRemaining[0][0] == '-')
            {
                throw CommandLineException("Expecting a parameter for option " + GetName());
            }
            *pOutput = argvRemaining[0];
            return 1;
        }
    };

    int ProcessOption(const std::string &text, int argsRemaining, char *argv[])
    {
        for (auto option : options)
        {
            if (option->Matches(text))
            {
                return option->Execute(argsRemaining, (const char**)argv);
            }
        }
        std::stringstream s;
        s << "Invalid option: " << text;
        throw CommandLineException(s.str());
    }

    std::vector<std::string> args;

public:
    ~CommandLineParser()
    {
        for (auto item : options)
        {
            delete item;
        }
    }

    void AddOption(const std::string &shortOption, const std::string &longOption, bool *pResult)
    {
        if (shortOption.length() != 0)
        {
            auto option = "-" + longOption;
            options.push_back(new BooleanOption(option, pResult, true));
            options.push_back(new BooleanOption(option + "+", pResult, true));
            options.push_back(new BooleanOption(option + "-", pResult, false));
        }
        if (longOption.length() != 0)
        {
            auto option = "--" + longOption;
            options.push_back(new BooleanOption(option, pResult, true));
            options.push_back(new BooleanOption(option + "+", pResult, true));
            options.push_back(new BooleanOption(option + "-", pResult, false));
        }
    }

    void AddOption(const std::string &option, bool *pResult)
    {
        options.push_back(new BooleanOption(option, pResult, true));
        options.push_back(new BooleanOption(option + "+", pResult, true));
        options.push_back(new BooleanOption(option + "-", pResult, false));
    }

    void AddOption(const std::string &shortOption, const std::string &longOption, std::string *pResult)
    {
        options.push_back(new StringOption("-" + shortOption, pResult));
        options.push_back(new StringOption("--" + longOption, pResult));
    }

    void AddOption(const std::string &option, std::string *pResult)
    {
        options.push_back(new StringOption(option, pResult));
    }

    template <typename T>
    void AddOption(const std::string &shortOption, const std::string &longOption, T *pResult)
    {
        if (shortOption.length() != 0)
            options.push_back(new Option<T>("-" + shortOption, pResult));
        if (longOption.length() != 0)
            options.push_back(new Option<T>("--" + longOption, pResult));
    }

    template <typename T>
    void AddOption(const std::string &option, T *pResult)
    {
        options.push_back(new Option<T>(option, pResult));
    }
    bool Parse(int argc, char **argv)
    {
        for (int i = 1; i < argc; ++i)
        {
            const char *arg = argv[i];
            if (arg[0] == '-')
            {
                int argsConsumed = ProcessOption(arg, argc - i - 1, argv + i + 1);
                i += argsConsumed;
            }
            else
            {
                args.push_back(std::string(arg));
            }
        }
        return true;
    }

    const std::vector<std::string> &Arguments() { return args; }
};
