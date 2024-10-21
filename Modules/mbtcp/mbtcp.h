#ifndef __MBTCP_H
#define __MBTCP_H

#include <stdint.h>

int mbtcp_cycle_init(void);
void mbtcp_copy_input_registers(uint16_t *src, int count);
void mbtcp_set_output_register(uint16_t *val, int count);

#endif /* __MBTCP_H */
