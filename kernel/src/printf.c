#include "limine.h"
#include <stddef.h>
#include <stdint.h>

#include "font8x8_basic.h"

#define NULL ((void *)0)
#define NBBY 8 /* number of bits in a byte */
char const hex2ascii_data[] = "0123456789abcdefghijklmnopqrstuvwxyz";
#define hex2ascii(hex) (hex2ascii_data[hex])
#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end
#define toupper(c) ((c) - 0x20 * (((c) >= 'a') && ((c) <= 'z')))

#define MAX_STRING 255
#define LINE_HEIGHT 8

struct limine_framebuffer *framebuffer;
int pitch;
static int next_x, next_y;

char log_buffer[4096];
int current_line = 0;

void putpixel(int x, int y, int rgb) {
  uint32_t *fb_ptr = framebuffer->address;
  fb_ptr[x + y * pitch] = rgb;
};

#define BIT(v, n) ((1 << n) & v)

void fb_putchar(int x, int y, char c, int rgb) {
  int i, j;
  /* rows  */
  for (i = 0; i < 8; i++) {
    /* cols */
    for (j = 0; j < 8; j++) {
      if (BIT(font8x8_basic[(unsigned int)c][i], j))
        putpixel(x + j, y + i, rgb);
    }
  }
}

void putstring(int x, int y, const char *str, size_t len, int rgb) {

  size_t i;
  for (i = 0; i < len; i++) {
    fb_putchar(x + i * 8, y, str[i], rgb);
  }
}

size_t strlen(const char *str) {
  int i;

  for (i = 0; i < MAX_STRING; i++)
    if (str[i] == '\0')
      return i;

  return i;
}
void println(const char *str) {
  int i;
  /* putstring(0, current_line * LINE_HEIGHT, str, strlen(str), 0x00FF00); */

  uint64_t x_offset = 0;
  for (i = 0; i < strlen(str); i++) {
    char c = str[i];

    if (c == '\n' || x_offset >= framebuffer->width) {
      current_line++;
      x_offset = 0;
      continue;
    }
    /* putchar(x_offset, current_line * 8, str[i], 0x00FF00); */
    x_offset += 8;
  }
  current_line++;
}

void clrscr(void *framebuffer);

static void putchar(int c, void *arg) { /* add your putchar here */
  if (c == '\n') {
    next_y += 8;
    next_x = 0;
    return;
  }
  fb_putchar(next_x, next_y, c, 0x00ff00);
  next_x += 8;
}

// Yet, another good itoa implementation
// returns: the length of the number string
int itoa(int value, char *sp, int radix) {
  char tmp[16]; // be careful with the length of the buffer
  char *tp = tmp;
  int i;
  unsigned v;

  int sign = (radix == 10 && value < 0);
  if (sign)
    v = -value;
  else
    v = (unsigned)value;

  while (v || tp == tmp) {
    i = v % radix;
    v /= radix;
    if (i < 10)
      *tp++ = i + '0';
    else
      *tp++ = i + 'a' - 10;
  }

  int len = tp - tmp;

  if (sign) {
    *sp++ = '-';
    len++;
  }

  while (tp > tmp)
    *sp++ = *--tp;

  return len;
}

int kvprintf(char const *fmt, void (*func)(int, void *), void *arg, int radix,
             va_list ap) {
  size_t len = 0, i = 0, j = 0;
  char num_str[255];
  uint64_t num;

  for (i = 0; i < strlen(fmt); i++) {
    /* Print until % is found */
    if (fmt[i] != '%') {
      func(fmt[i], NULL);
      continue;
    }

    /* Skip over % */
    i++;

    /* Handle format char */
    switch (fmt[i]) {
    case 'd':
    case 'i':
      num = va_arg(ap, int);
      len = itoa(num, num_str, 10);
      for (j = 0; j < len; j++)
        putchar(num_str[j], NULL);
      break;
    case 'x':
      num = va_arg(ap, int);
      len = itoa(num, num_str, 16);
      for (j = 0; j < len; j++)
        putchar(num_str[j], NULL);
      break;
    }
  }
  return 0;
}

void printf(const char *fmt, ...) {
  /* http://www.pagetable.com/?p=298 */
  va_list ap;

  va_start(ap, fmt);
  kvprintf(fmt, putchar, NULL, 10, ap);
  va_end(ap);
}
