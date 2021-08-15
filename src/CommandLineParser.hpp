#pragma once
#include <string>
#include <vector>
#include "PiPedalException.hpp"
#include <sstream>

namespace pipedal
{

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
                    throw PiPedalException("Expecting a parameter for option " + GetName());
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
                    throw PiPedalException("Argument for option " + GetName() + " is not in the correct format.");
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
                :   OptionBase(name),
                    pOutput(pOutput)
            {
            }
            virtual int Execute(int argcRemaining, const char **argvRemaining)
            {
                if (argcRemaining == 0 || argvRemaining[0][0] == '-')
                {
                    throw PiPedalException("Expecting a parameter for option " + GetName());
                }
                *pOutput = argvRemaining[0];
                return 1;
            }
        };

        int ProcessOption(const std::string &text, int argsRemaining, const char *argv[])
        {
            for (auto option : options)
            {
                if (option->Matches(text))
                {
                    return option->Execute(argsRemaining, argv);
                }
            }
            std::stringstream s;
            s << "Invalid option: " << text;
            throw PiPedalException(s.str());
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

        void AddOption(const std::string &option, bool *pResult)
        {
            options.push_back(new BooleanOption(option, pResult, true));
            options.push_back(new BooleanOption(option + "+", pResult, true));
            options.push_back(new BooleanOption(option + "-", pResult, false));
        }
        void AddOption(const std::string &option, std::string *pResult)
        {
            options.push_back(new StringOption(option, pResult));
        }
        template <typename T>
        void AddOption(const std::string &option, T *pResult)
        {
            options.push_back(new Option<T>(option, pResult));
        }
        bool Parse(int argc, const char *argv[])
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

        const std::vector<std::string> Arguments() { return args; }
    };

} // namespace