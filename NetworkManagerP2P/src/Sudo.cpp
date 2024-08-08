#include "Sudo.hpp"
#include "unistd.h"
#include "sys/types.h"
#include <vector>
#include <stdexcept>

bool IsSudo() 
{
    return (geteuid() == 0);
}
void  ExecWithSudo(int argc, char**argv)
{
    std::vector<const char*> newArgs;
    newArgs.reserve(argc+2);
    newArgs.push_back("sudo");
    for (int i = 0; i < argc; ++i)
    {
        newArgs.push_back(argv[i]);
    }
    newArgs.push_back(nullptr);

    execvp("sudo",(char*const*)&newArgs[0]);
    throw std::runtime_error("Failed to restart with sudo.");
}

void ForceSudo(int argc, char**argv)
{
    if (IsSudo()) return;
    ExecWithSudo(argc,argv);
}

