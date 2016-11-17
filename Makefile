# arm-linux-gnueabihf  toolchain for Linux based compiling
# arm-none-eabi  toolchain for Windows based compiling
.SUFFIXES:
.SUFFIXES: .c .s .h .o
DRV=drivers/
DRVTI=drivers_ti/
OBJ=obj/
# CROSSCOMPILE = arm-linux-gnueabihf-
CROSSCOMPILE = arm-none-eabi-

CFLAGS = -mcpu=cortex-a8 -marm -Wall -O2 -nostdlib -nostartfiles -ffreestanding \
  -fstack-usage -Wstack-usage=16384 -I $(DRV) \
  -I $(DRVTI) \
  -I $(DRVTI)/include \
  -I lwip \
  -I lwip/apps/echoserver_raw \
  -I lwip/ports/cpsw \
  -I lwip/ports/cpsw/include \
  -I lwip/ports/cpsw/netif \
  -I lwip/src/include \
  -I lwip/src/include/ipv4 \
  -I lwip/src/include/lwip

VPATH = $(DRV) lwip/ports/cpsw lwip/ports/cpsw/netif lwip/src/core

all : rts.elf

$(OBJ)enetecho.o : enetecho.c
	$(CROSSCOMPILE)gcc $(CFLAGS) -c enetecho.c -o $(OBJ)enetecho.o

$(OBJ)%.o : $(DRV)%.s
	$(CROSSCOMPILE)gcc $(CFLAGS) -c $< -o $@

$(OBJ)lwiplib.o : lwip/ports/cpsw/lwiplib.c lwip/ports/cpsw/netif/cpswif.c
	$(CROSSCOMPILE)gcc $(CFLAGS) -c lwip/ports/cpsw/lwiplib.c -o $(OBJ)lwiplib.o

$(OBJ)%.o : $(DRVTI)%.c
	$(CROSSCOMPILE)gcc $(CFLAGS) -c $< -o $@

$(OBJ)%.o : $(DRVTI)%.s
	$(CROSSCOMPILE)gcc $(CFLAGS) -c $< -o $@

$(OBJ)echod.o : echod.c
	$(CROSSCOMPILE)gcc $(CFLAGS) -c echod.c -o $(OBJ)echod.o

rts.elf : memmap.lds $(OBJ)*.o
	$(CROSSCOMPILE)ld -o rts.elf -T memmap.lds $(OBJ)startup.o $(OBJ)irq.o \
  $(OBJ)enetecho.o $(OBJ)echod.o $(OBJ)lwiplib.o $(OBJ)mmu.o $(OBJ)cache.o $(OBJ)cpsw.o \
  $(OBJ)mdio.o $(OBJ)phy.o $(OBJ)cp15.o $(OBJ)libc.o $(OBJ)gpio.o $(OBJ)uart.o \
  $(OBJ)eth.o $(OBJ)tim.o

	$(CROSSCOMPILE)objcopy rts.elf rts.bin -O srec
# srec format above for jtag loading (ie binary format with a short header)
# binary format below for MMC booting
#	$(CROSSCOMPILE)objcopy rts.elf app -O binary
	$(CROSSCOMPILE)objdump -M reg-names-raw -D rts.elf > rts.lst
	$(CROSSCOMPILE)objdump -d -S -h -t rts.elf > rts.dmp

clean :
	-@del *.dmp *.lst *.elf *.bin
# NB never ever delete any object files from obj/ 
#    it will mess up the rule for making rts.elf
