#ifndef ENV_BOARD_H
#define ENV_BOARD_H

#include "io.h"

extern const struct env_io board_io;
extern const char board_name[];
void board_init(void);

#endif
