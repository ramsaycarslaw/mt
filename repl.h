#ifndef MT_REPL_H
#define MT_REPL_H

#include "vm.h"
#include <poll.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void repl_loop(void);
char *mt_readline(void);

#endif
