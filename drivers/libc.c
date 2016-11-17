//
// libc.c - spoofed C Library routines
//
#include "string.h"
#include <stdint.h>
#define CONSOLE_UART            SOC_UART_0_REGS
extern void uart_tx(uint32_t uart_base_addr, uint32_t byte);
//
// memcpy from Daniel Vik
//
void *memcpy(void *dest, const void *src, size_t count) {
  char* dst8 = (char*)dest;
  char* src8 = (char*)src;
  while (count--) {
    *dst8++ = *src8++;
  }
  return dest;
}
//
// memset
//
void *memset(void *ptr, int value, size_t num) {
  char* dst8 = (char*)ptr;
  unsigned char val = (unsigned char)value;
  while (num--) {
    *dst8++ = val;
  }
  return ptr;
}
//
// strlen
//
//size_t strlen(str)
//	const char *str;
size_t strlen(const char *str) {
	register const char *s;
	for (s = str; *s; ++s);
	return(s - str);
}
//
// idiv from github.com/auselen
//
int __aeabi_idiv(int num, int denom) {
    int q = 0;
    while (num >= denom) {
        num -= denom;
        q++;
    }
   return q;
}
unsigned int __aeabi_uidiv(unsigned int num, unsigned int denom) {
    unsigned int q = 0;
    while (num >= denom) {
        num -= denom;
        q++;
    }
   return q;
}
//
// ConsolePrintf - ascii formatted print to consoleUART
//
// hacked from github.com/auselen
//
// TODO upgrade from polled mode to interrupt
// 
void ConsolePrintf(const char *fmt, ...) {
  int *stack_head = __builtin_frame_address(0);
  stack_head += 2; // skip fmt, skip stack_head
  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt++) {
        case 'c': {
          uart_tx(CONSOLE_UART, *stack_head++);
          break;
        }
        case 's': {
          const char *s = (char *) *stack_head++;
          while (*s) {
            uart_tx(CONSOLE_UART, *s++);
          }
          break;
        }
        case 'x': {
          int num = *stack_head++;
          int shift = 28;
          while (shift >= 0) {
            int hd = (num >> shift) & 0xf;
            if (hd > 9)
              hd += 'A' - 10;
            else
              hd += '0';
            uart_tx(CONSOLE_UART, hd);
            shift -= 4;
          }
          break;
        }
        case 'd': {
          int num = *stack_head++;
          char buf[16];
          char *s = buf + (sizeof(buf) / sizeof(buf[0])) - 1;
          char *e = s;
          do {
            *--s = '0' + num % 10;
          } while (num /= 10);
          while (s < e)
            uart_tx(CONSOLE_UART, *s++);
            break;
        }
        default:
          uart_tx(CONSOLE_UART, '?');
      }
    } else {
      uart_tx(CONSOLE_UART, *fmt++);
    }
  }
}