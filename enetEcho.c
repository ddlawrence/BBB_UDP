//
// baremetal enetecho demo
//
// self contained - gnu version
//
// This is a modded port of the TI Starterware ethernet echo application.  
//
// The BBB will tell you it's MAC address and IP# then it will spew 
// all the packets out UART0 as the lwIP echoserver returns them.
// Ping packets are 14+60 bytes long, you can check their contents.  
//
// Plug your BBB into your router and ping from a PC:
// ping 10.0.0.2    [or whatever IP the bbb is assigned]
//
// Hit the boot button to flash a USR LED (interrupt sanity check)
// 
// Input comes by way of ethernet_input() in etharp.c and the main loop below.  
// Output goes by way of tcp_write() in tcp_out.c and echod.c .  
// See rawapi.txt in /doc for interface info.  Most of the TI ethernet middleware 
// is in cpswif.c
//
// lwIP is a Picasso!  It is quite intricate so I did not touch it.  
// The TI starterware drivers are pretty good also and required very little 
// tweaking to build with GNU.  Changes are marked with "// n!"
//
// The full lwIP suite is next.  It needs the full FAT32 file system so it 
// will take months.  In the mean time you have UDP functionality.  
//
// Use a jtag cable to load/boot the BBB, but you can boot from MMC.
//
// built with GNU tools :) on platform Win32 :(
//
// if you want to get involved, contact at www.baremetal.tech
//
#include <stdint.h>
#include "bbb_main.h"

#include "locator.h"
#include "echod.h"
#include "lwiplib.h"
#include "lwipopts.h"
#include "soc_AM335x.h"
#include "beaglebone.h"
#include "cache.h"
#include "mmu.h"
#include "cache.h"

#define LEN_IP_ADDR                        (4u)

#define START_ADDR_DDR                     (0x80000000)
#define START_ADDR_DEV                     (0x44000000)
#define START_ADDR_OCMC                    (0x40300000)
#define START_ADDR_ROM                     (0x00020000) // n!
#define NUM_SECTIONS_DDR                   (512)
#define NUM_SECTIONS_DEV                   (960)
#define NUM_SECTIONS_OCMC                  (1)
#define NUM_SECTIONS_ROM                   (1)          // n!

static volatile uint32_t pageTable[4*1024] __attribute__((aligned(0x4000)));

//
// Interrupt Handler for Core 0 Receive interrupt
//
void CPSWCore0RxIsr(void){
  lwIPRxIntHandler(0);
}

//
// Interrupt Handler for Core 0 Transmit interrupt
//
void CPSWCore0TxIsr(void){
  lwIPTxIntHandler(0);
}

//
// Display the IP addrss on the Console
//
static void IpAddrDisplay(unsigned int ipAddr) {
  ConsolePrintf("%d.%d.%d.%d\n", (ipAddr & 0xFF), ((ipAddr >> 8) & 0xFF),
                ((ipAddr >> 16) & 0xFF), ((ipAddr >> 24) & 0xFF));
}

//
// Set up the MMU
//
static void MMUConfigAndEnable(void){
  REGION regionDdr = {MMU_PGTYPE_SECTION, START_ADDR_DDR, NUM_SECTIONS_DDR,
                      MMU_MEMTYPE_NORMAL_NON_SHAREABLE(MMU_CACHE_WT_NOWA,
                                                       MMU_CACHE_WB_WA),
                      MMU_REGION_NON_SECURE, MMU_AP_PRV_RW_USR_RW,
                      (unsigned int*)pageTable
                     };
  REGION regionOcmc = {MMU_PGTYPE_SECTION, START_ADDR_OCMC, NUM_SECTIONS_OCMC,
                       MMU_MEMTYPE_NORMAL_NON_SHAREABLE(MMU_CACHE_WT_NOWA,
                                                        MMU_CACHE_WB_WA),
                       MMU_REGION_NON_SECURE, MMU_AP_PRV_RW_USR_RW,
                       (unsigned int*)pageTable
                      };
  REGION regionDev = {MMU_PGTYPE_SECTION, START_ADDR_DEV, NUM_SECTIONS_DEV,
                      MMU_MEMTYPE_DEVICE_SHAREABLE,
                      MMU_REGION_NON_SECURE,
                      MMU_AP_PRV_RW_USR_RW  | MMU_SECTION_EXEC_NEVER,
                      (unsigned int*)pageTable
                     };
  REGION regionROM = {MMU_PGTYPE_SECTION, START_ADDR_ROM, NUM_SECTIONS_ROM,
                      MMU_MEMTYPE_NORMAL_NON_SHAREABLE(MMU_CACHE_WT_NOWA,
                                                       MMU_CACHE_WB_WA),
                      MMU_REGION_NON_SECURE, MMU_AP_PRV_RW_USR_RW,
                      (unsigned int*)pageTable
                     };  // n!

  MMUInit((unsigned int*)pageTable);

  MMUMemRegionMap(&regionDdr);
  MMUMemRegionMap(&regionOcmc);
  MMUMemRegionMap(&regionDev);
  MMUMemRegionMap(&regionROM);  // n!

  MMUEnable((unsigned int*)pageTable);
}

//
// main
//
void main(void){
  uint32_t i, j, ipAddr, usr_leds, packet_count;
  LWIP_IF lwipIfPort1;

  consoleUART = SOC_UART_0_REGS;
  usr_leds = 0xf << 21;  // enab USR LEDs, pin # 21-24
  gpio_init(SOC_GPIO_1_REGS, usr_leds);

  uart_tx(consoleUART, 0x0A);   // print n!
  uart_tx(consoleUART, 0x6E);
  uart_tx(consoleUART, 0x21);
  uart_tx(consoleUART, 0x0A);

  MMUConfigAndEnable();

  CacheEnable(CACHE_ALL);

  eth_init();

  tim_init();

  irq_init();

  ipAddr = 0;
  MACAddrGet(0, lwipIfPort1.macArray);
  ConsolePrintf("Port 1 MAC:  ");
  for(i = 0; i <= 5; i++) {
    hexprintbyte(lwipIfPort1.macArray[5-i]);
    uart_tx(consoleUART, 0x20);
  }

  lwipIfPort1.instNum = 0;
  lwipIfPort1.slvPortNum = 1; 
  lwipIfPort1.ipAddr = 0; 
  lwipIfPort1.netMask = 0; 
  lwipIfPort1.gwAddr = 0; 
  lwipIfPort1.ipMode = IPADDR_USE_DHCP; 
  ipAddr = lwIPInit(&lwipIfPort1);
  if(ipAddr) {
    ConsolePrintf("\nPort 1 IP Address Assigned: ");
    IpAddrDisplay(ipAddr);
  } else {
    ConsolePrintf("\nPort 1 IP Address Acquisition Failed.\n");
  }

  echo_init(); // Initialize echo server

  asm volatile(".word 0xe7f000f0");  // undefined instruction   test UND isr

  ConsolePrintf("looping...\n");

  packet_count = ip_rx_count;
  while(1) {
    if(packet_count != ip_rx_count) {  // incoming data   trigger on a new Rx IRQ
      packet_count = ip_rx_count;
      for(j = 0; j < packet_len; j++) {
        if(!(j % 0x10)) uart_tx(consoleUART, 0x0a);
        hexprintbyte(*packet_ptr++);
        uart_tx(consoleUART, 0x20); // print a space fast
      }
      ConsolePrintf("\nPKT#%d LEN=%d\n", packet_count, packet_len);
      packet_len = 0;
    }
    tim_delay(1);
  }
}
