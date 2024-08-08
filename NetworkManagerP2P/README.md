The pipedal_nm_p2pd service manages PiPedal P2P connections on Debian bookworm or later distros.

Debian Bookworm switchs the entire network stack from the deprecated dhcpcd/NetworkManager 
stack to the current NetworkManager stack. Unfortunately, NetworkManager does not 
provide support for P2P Wi-fi direct. 

NetworkManager P2P support is abysmal, providing no support whatsoever 
for P2P GOs. Furthermore, a bug in NetworkManager prevents anyone else from providing a P2P GO 
solution through wpas_supplicant. Specifically, NetworkManager call RemoveInterface on any P2P 
group that it didn't start itself, even if p2p-dev-wlan-0 is marked as unmanaged.

The solution: Configure NetworkManager to mark wlan0 as an unmanaged interface. This allows
the pipedal_nm_p2pd service to manage p2p sessions via wpa_supplicant without interference from
NetworkManager. Unfortunately, that means that PiPedal can't run wi-fi connections anymore when 
running Wifi p2p connections. If the NetworkManager project provides sensible support 
for p2p in future, we will look at it again.

This project uses DBus interfaces, which is a significant improvement on the use of 
wpa_cli sockets in the old pipedal_p2pd module.

Dependent packages:


    sudo apt install libsdbus-c++-dev
    sudo apt install libsdbus-c++-bin
