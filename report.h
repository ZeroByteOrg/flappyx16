#ifndef __REPORT_H__
#define __REPORT_H__

#include <stdint.h>

#define REPORT_PALETTE	2

typedef struct report_t {
  uint16_t	addr;
  int16_t	x, y, target;
  int8_t	speed;
} report_t;

extern void init_report(report_t *results, int16_t x, int16_t y);
extern uint8_t *update_report(report_t *results, uint8_t *spregs);

#endif
