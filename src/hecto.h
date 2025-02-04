#ifndef _HECTO_H_
#define _HECTO_H_

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define HECTO_VERSION "0.1.0"
#define HECTO_TAB_STOP 8
#define HECTO_NUMLINE 7
#define HECTO_QUIT_CONFIRM 3 // how many times should the quit button be pressed to confirm unsaved exit

typedef struct erow {
	int idx; // row's position in file
	int size; // size of row
	int rsize; // size of rendered row (including characters taking up more space like Tab)
	char *chars; // content of row
	char *render; // content of row that will be rendered
	unsigned char *hl; // array acting like a mask for highlights rendering
	int hl_open_comment; // whether the row is inside a multiline comment
} erow;

struct editorConfig {
	int cx, cy; // cursor x & y position in file (starting from the upperleft corner)
	int rx; // rendered position of cursor in row -- this position gets displayed on screen
	int rowoff; // row offset -- for vertical scrolling
	int coloff; // collumn offset -- for horizontal scrolling
	int screenrows; // height of terminal window
	int screencols; // width of terminal window
	int numrows; // number of rows currently in editor memory
	int show_numline; // boolean to show line numbers on the left side of the screen
	erow *row; // array of rows in editor memory
	char *filename; // name of opened file
	int dirty; // flag if file was edited since opening
	char statusmsg[80]; // status message displayed on the bottom of the screen
	time_t statusmsg_time; // status message timestamp
	struct editorSyntax *syntax;
};

#endif
