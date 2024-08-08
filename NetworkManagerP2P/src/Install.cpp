#include "Install.hpp"
#include "Sudo.hpp"
#include "cassert"
#include "DBusLog.hpp"

int  InstallService()
{
    assert(IsSudo());
    LogInfo("","InstallService","");
    return EXIT_SUCCESS;
}
int UninstallService()
{
    assert(IsSudo());
    LogInfo("","Uninstall","");
    return EXIT_SUCCESS;

}