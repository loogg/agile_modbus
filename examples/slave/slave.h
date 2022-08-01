#ifndef __SLAVE_H
#define __SLAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>
#include "agile_modbus.h"
#include "agile_modbus_slave_util.h"

extern pthread_mutex_t slave_mtx;
extern const agile_modbus_slave_util_t slave_util;

#ifdef __cplusplus
}
#endif

#endif /* __SLAVE_H */
