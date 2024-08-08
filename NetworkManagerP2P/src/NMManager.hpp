#pragma once
#include <memory>

class NMManager {
protected:
    NMManager();
public:
    using ptr = std::unique_ptr<NMManager>;
    static ptr Create();
    virtual ~NMManager();
    virtual void Init() =0;
    virtual void Run() = 0;
    virtual void Stop() = 0;
};