#pragma once
#include <exception>

namespace pipedal
{

    class PiPedalException : public std::exception
    {
        std::string what_;

    public:
        PiPedalException(const std::string &what)
            : std::exception(), what_(what)
        {
        }
        virtual ~PiPedalException() { 

        }
        virtual const char *what() const noexcept { return what_.c_str(); }
    };

    class PiPedalArgumentException : public PiPedalException
    {
    public:
        PiPedalArgumentException()
            : PiPedalException("Invalid argument")
        {
        }
        PiPedalArgumentException(std::string &&what)
            : PiPedalException(std::forward<std::string>(what))
        {
        }
    };
    class PiPedalStateException : public PiPedalException
    {
    public:
        PiPedalStateException(std::string &&what)
            : PiPedalException(std::forward<std::string>(what))
        {
        }
    };
    class PiPedalLogicException : public PiPedalException
    {
    public:
        PiPedalLogicException(const char *what)
            : PiPedalException(what)
        {
        }
    };

} // namespace