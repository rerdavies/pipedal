#pragma once

#include <functional>
#include <atomic>

class SignalHandler {
public:
    // Signal type. Limited to signal types supported by <csignal>
    enum SignalType 
    {
        Terminate, //SIGTERM
        SegmentationFault,   //SIGSEGV
        Interrupt, // SIGINT External interrupt executed by user.
        IllegalInstruction, // SIGILL
        Abort, // SIGABORT
        FloatingPointError, // SIGFPE.
        Hangup, // SIGHUP
    };
    using SignalCallback = std::function<void ()>;

    static void HandleSignal(SignalType signal, SignalCallback&&fn);
    static void UnhandleSignal(SignalType signal);
    static void IgnoreSignal(SignalType signal);
private:
    static void OnSignal(int osSignal);
    static const int MAX_SIGNAL = 64;
    static SignalCallback handlers[MAX_SIGNAL];  
};