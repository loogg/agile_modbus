#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdint.h>
#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

int serial_init(const char *device,
                int baud, char parity, int data_bit,
                int stop_bit, struct termios *old_tios);
void serial_close(int s, struct termios *old_tios);
int serial_send(int s, const uint8_t *buf, int length);
int serial_receive(int s, uint8_t *buf, int bufsz, int timeout);
int serial_flush(int s);

#ifdef __cplusplus
}
#endif

#endif
