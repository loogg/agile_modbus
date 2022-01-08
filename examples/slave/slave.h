#ifndef __SLAVE_H
#define __SLAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "agile_modbus.h"

int slave_callback(agile_modbus_t *ctx, struct agile_modbus_slave_info *slave_info);

#ifdef __cplusplus
}
#endif

#endif /* __SLAVE_H */
