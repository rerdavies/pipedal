## Changing the Web Server Port.
     
If your Pi already has a web server on port 80, you can reconfigure PiPedal to use a different port. After installing PiPedal, run the following command on your Raspberry Pi:
     
     sudo pipedalconfig --install --port 81
     
You can optionally restrict the addresses on which PiPedal will respond by providing an explicit IP address. For example, to configure PiPedal to only accept connections from the local host:
     
     sudo pipedalconfig --install --port 127.0.0.1:80
     
To configure PiPedal to only accept connections on the Wi-Fi host access point:
     
     sudo pipedalconfig --install --port 172.22.1.1:80

--------
[<< Command-Line Configuration of PiPedal](CommandLine.md)  | [Up](Documentation.md) | [Using LV2 Audio Plugins >>](UsingLv2Plugins.md)