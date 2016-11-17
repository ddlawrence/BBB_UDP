# BBB_UDP
BeagleBone Black Ethernet/UDP drivers ported from TI/Starterware in Assembly &amp; C demo, GNU/baremetal 
baremetal enetecho demo

self contained - gnu version

This is a modded port of the TI Starterware ethernet echo application.  

The BBB will tell you it's MAC address and IP# then it will spew 
all the packets out UART0 as the lwIP echoserver returns them.
Ping packets are 14+60 bytes long, you can check their contents.  

Plug your BBB into your router and ping from a PC:
ping 10.0.0.2    [or whatever IP the bbb is assigned]

Hit the boot button to flash a USR LED (interrupt sanity check)
 
Input comes by way of ethernet_input() in etharp.c and the main() loop  
Output goes by way of tcp_write() in tcp_out.c and echod.c .  
See rawapi.txt in /doc for interface info.  Most of the TI ethernet middleware 
is in cpswif.c

lwIP is a Picasso!  It is quite intricate so I did not touch it.  
The TI starterware drivers are pretty good also and required very little 
tweaking to build with GNU.  Changes are marked with "// n!"

Use a jtag cable to load/boot the BBB, but you can boot from MMC.

built with GNU tools :) on platform Win32 :(

if you want to get involved, contact at www.baremetal.tech
