#include "InheritPriorityMutex.hpp"


using namespace pipedal;
using namespace std;



inherit_priority_mutex::inherit_priority_mutex()
{
#ifdef __linux__ // (windows mutexes do NOT have priority inheritance alread)

    // Recreate the raw mutex with priory inheritance.
    ::pthread_mutex_destroy(native_handle());
    ::pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_setprotocol(&attr,PTHREAD_PRIO_INHERIT);
    ::pthread_mutex_init(native_handle(),&attr);
    ::pthread_mutexattr_destroy(&attr);



#endif
}