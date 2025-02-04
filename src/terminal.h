
#ifndef _HECTO_TERMINAL_H_
#define _HECTO_TERMINAL_H_

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>


#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

enum editorKey { // abstract special keys as constants 
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

void die(const char *er_message);
void disableRawMode();
void enableRawMode();
int editorReadKey();
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);
void clearScreen();

#endif
