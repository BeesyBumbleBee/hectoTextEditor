#include "terminal.h"

// macro to disable -Wunused-result warning on functions where its intentionaly ignored
#define ign(X) if(1==((long)X)){;} 

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

struct termios orig_termios;

// Display error message and kill the program
void die(const char *er_message) {
	clearScreen();
	perror(er_message);
	exit(1);
}

// Disable raw mode and return console's original configuration
void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
		die("tcsetattr");
}

// Enable raw mode of console for better control (won't work on Windows)
void enableRawMode() {
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
		die("tcsetattr");
	atexit(disableRawMode); // enable raw mode on program exit
	
	struct termios raw = orig_termios;
	
	// disable terminal flags
	raw.c_iflag &= ~(BRKINT | INPCK | ICRNL | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	raw.c_cflag |= (CS8);
	// enable read() timeout
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1; // timeout set to x/10 of a second after which read() returns 0
	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		die("tcsetattr");
}

// Read keys and escape sequensces from stdin
int editorReadKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) // in some terminals read may return -1, but no error has occured in which situation EAGAIN is broadcasted into errno and needs to be checked for error handling 
			die("read");
	}
	
	if (c == '\x1b') {
		char seq[3]; // sequence buffer
		
		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
		
		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			} else {
				switch (seq[1]) {
					case 'A': return ARROW_UP;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'H': return HOME_KEY;
					case 'F': return END_KEY;
				}
			}
		} else if (seq[0] == 'O') {
			switch (seq[1]) {
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
			}
		}
		
		return '\x1b';
	} else {
		return c;
	}
}

// Get get cursor position -- used as fallback to determine window size
int getCursorPosition(int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;
	
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1; // call the Devive Status Report to get cursor position
	
	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0'; 
	
	if (buf[0] != '\x1b' || buf[1] != '[') return -1; // make sure reply is an esc sequence
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1; // read row and column info
	
	return 0;
}

// Get window size
int getWindowSize(int *rows, int *cols) {
	struct winsize ws;
	
	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) { // Terminal Input/Output Control Get Window Size
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1; // if terminal doesnt support ioctl we use fallback method
		return getCursorPosition(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows= ws.ws_row;
		return 0;
	}
}

void clearScreen() {
	ign(write(STDOUT_FILENO, "\x1b[2J", 4));
	ign(write(STDOUT_FILENO, "\x1b[H", 3));
}
