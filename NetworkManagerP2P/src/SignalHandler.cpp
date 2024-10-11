#include "SignalHandler.hpp"

#include <csignal>
#include <atomic>
#include <vector>
#include <stdexcept>


static int ToOsSignalType(SignalHandler::SignalType signalType)
{
    int result;
    switch (signalType)
    {
        case SignalHandler::Terminate: result = SIGTERM; break;
        case SignalHandler::SegmentationFault: result = SIGSEGV; break;
        case SignalHandler::Interrupt: result =  SIGINT; break;
        case SignalHandler::IllegalInstruction: result =  SIGILL; break;
        case SignalHandler::Abort: result =  SIGABRT; break;
        case SignalHandler::FloatingPointError: return  SIGFPE; break;
        case SignalHandler::Hangup: result = SIGHUP; break;

        default:
            throw std::runtime_error("Invalid signal type");
    }
    return result;
}


void SignalHandler::HandleSignal(SignalHandler::SignalType signal, SignalCallback&&callback)
{
    int osSignal = ToOsSignalType(signal);
    if (osSignal > MAX_SIGNAL)
    {
        throw std::runtime_error("MAX_SIGNAL isn't large enough.");
    }

    handlers[osSignal] = std::move(callback);
    std::signal(osSignal,OnSignal);
}

void SignalHandler::UnhandleSignal(SignalHandler::SignalType signal)
{
    int osSignal = ToOsSignalType(signal);
    std::atomic_signal_fence(std::memory_order::release);
    std::signal(osSignal,SIG_DFL);
    handlers[osSignal] = std::function<void(void)>();
}

void SignalHandler::IgnoreSignal(SignalHandler::SignalType signal)
{
    int osSignal = ToOsSignalType(signal);
    std::signal(osSignal,SIG_IGN);
    handlers[osSignal] = std::function<void(void)>();
}

void SignalHandler::OnSignal(int osSignal)
{
    std::atomic_signal_fence(std::memory_order::acquire);
    handlers[osSignal]();
}

SignalHandler::SignalCallback SignalHandler::handlers[MAX_SIGNAL];  
