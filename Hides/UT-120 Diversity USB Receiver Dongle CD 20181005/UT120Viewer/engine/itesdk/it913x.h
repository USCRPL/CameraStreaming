/* Windows user-mode stand-in for the Linux driver header the ITE API includes. */
#ifndef UT120_IT913X_SHIM_H
#define UT120_IT913X_SHIM_H
#include <string.h>
#include "type.h"
void ut120_sleep_ms(unsigned long ms);
void ut120_log(const char* fmt, ...);
#define msleep(ms) ut120_sleep_ms(ms)
#define mdelay(ms) ut120_sleep_ms(ms)
#define deb_data(...) ((void)0)
#define printk(...) ((void)0)
#endif
