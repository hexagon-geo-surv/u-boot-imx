#ifndef BOARD_SERIAL_H_
#define BOARD_SERIAL_H_

char board_serial_getc(void);
void board_serial_putc(const char c);
void board_serial_puts(const char *s);
int board_serial_tstc(void);
int board_serial_init(void *base, unsigned long baudrate, bool use_dte);


#endif /* BOARD_SERIAL_H_ */
