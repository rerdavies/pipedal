### Enabling Realtime Scheduling

_Note that running the plugins with PiPedal does not require this change. The Pipedal package installer grants real-time scheduling permission to the pipedald daemon at install time._

The TooB Convolution Reverb and TooB Cab IR effects require realtime scheduling to be enabled for the current user 
in order process convolution data on background threads while audio is running in realtime.

The procedure for enabling realtime scheduling varies between Linux distributions. 

Most modern distributions have the RTKIT DBus service, which allow programs to request realtime priority. But the restrictions imposed by the RTKIT 
DBus service prevent the TooB plugins from running. RTKIT does have configuration options, but the procedure for configuring RTKIT is unusually difficult. 
The easiest solution is to grant realtime scheduling permissions to 
the current user using the RLIMIT subsystem. In truth, you want to make these changes anyway, since they will increase amount of CPU time available to all audio plugins by 500%! (Just one of the reasons you should not be running pro-audio applications with RTKIT).

The native Linux scheduler calls require the user to have appropriate permissions. To grant permissions to the TooB plugins, make the following
modifications to your system.

On many Linux distros, any user of the audio group is granted realtime scheduling permissions. Check for files in the `/etc/limits.d/` directory named nn-audio.conf.
If such a file exists, open it to see which group has been granted permissions. There should be a line simular to the following:

```
   @audio    - rtprio   95
```

If the file exists, add your user-id to the audio group, and reboot to make the change take effect. Some distros use a group named `realtime` instead.

If not, grant realtime scheduling permissions to your user-id by adding the following line to `/etc/security/limits.conf`:

```
    youruserid     -    rtprio   95
```
or add the following line to grant realtime scheduling permissions for all users:
```
    *              -    rtprio   95
```

Once you have made the change, reboot to make the change take effect.

A more detailed discussion of why these changes need to be made can be found [here](https://github.com/rerdavies/pipedal/discussions/99), if you're interested.
