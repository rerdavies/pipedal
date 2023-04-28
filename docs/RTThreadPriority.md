### Enabling Realtime Scheduling

The TooB Convolution Reverb and TooB Cab IR effects require realtime scheduling to be enabled for the current user 
in order process convolution data on background threads while audio is running in realtime.

The procedure for enabling realtime scheduling varies between various Linux distributions. There are two ways to enable realtime scheduling: 
using native Linux scheduler calls; or using the RTKIT DBus service.  

Most modern distributions have an RTKIT DBus service. But the restrictions imposed by the RTKIT DBus service may prevent the TooB plugins from
starting up realtime threads. RTKIT does have some configuration options, but the procedure for configuring RTKIT is unusually difficult. If 
the TooB plugins exceed the limits imposed by RTKIT, the easier solution is to configure realtime permissions for the Linux native scheduler.

The native Linux scheduler calls require the user to have appropriate permissions. To grant permissions for the TooB plugins, make the following
modifications to your system. 

...

