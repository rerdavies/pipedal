#!/bin/bash
# Sign the built .deb package with the private key used in the Auto-update process.
#
# Won't work for you, because you don't have MY private key.
#
# Auto-update requires that .deb files be distributed from the GitHub rerdavies/pipedal project.
# And auto-updates WILL update installs of your forked binary unless you take steps to prevent that.
#
# If you are distributing a fork, you should see the file UpdaterSecurity.hpp for details on how to 
# modify pipedal to perform updates from an alternate github site. Or you could modify your fork 
# to NOT do auto-updating (probably best). The build doesn't currently have a procedure for 
# knocking out Auto-Update from UpdaterSecurity.hpp. But I would happily accept a pull request
# to bring such a feature back into mainline if you do it.

# Build the release package and create a detached GPG signature. This
# command assumes you have already run makePackage.sh so the .deb file
# exists in the build directory.
./build/src/makeRelease
