//
// HECTO
// A minimalistic text editor for UNIX OS
//
// Made by Szymon Oleśkiewicz
// Based on the editor in tutorial from:
// https://viewsourcecode.org/snaptoken/kilo/
//

#include "hecto.h"
#include "terminal.h"
#include "syntax.h"

#define CTRL_KEY(k) ((k) & 0x1f)	

// only one editorConfig struct is ever initiated and it's called 'E' for convienece
struct editorConfig E; 

void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char* editorPrompt(char *prompt, void (*callback)(char *, int));


/*** syntax highlighting ***/
int is_separator(int c) {
	return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void editorUpdateSyntax(erow *row) {
	row->hl = realloc(row->hl, row->rsize);
	memset(row->hl, HL_NORMAL, row->rsize);
	
	if (E.syntax == NULL) return;
	
	char **keywords = E.syntax->keywords;
	
	char *scs = E.syntax->singleline_comment_start;
	char *mcs = E.syntax->multiline_comment_start;
	char *mce = E.syntax->multiline_comment_end;
	char *cls = E.syntax->custom_line_start;
	
	int scs_len = scs ? strlen(scs) : 0;
	int mcs_len = mcs ? strlen(mcs) : 0;
	int mce_len = mce ? strlen(mce) : 0;
	int cls_len = cls ? strlen(cls) : 0;
	
	int prev_sep = 1;
	int in_string = 0;
	int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment); // for multiline comments
	
	int i = 0;
	while (i < row->rsize) {
		char c = row->render[i];
		unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;
		
		// Custom line
		if (cls_len && !in_string && !in_comment) {
			if (!strncmp(&row->render[i], cls, cls_len)) {
				memset(&row->hl[i], HL_CUSTOM, row->rsize - i);
				break;
			}
		}

		// Singleline comments
		if (scs_len && !in_string && !in_comment) {
			if (!strncmp(&row->render[i], scs, scs_len)) {
				memset(&row->hl[i], HL_COMMENT, row->rsize - i);
				break;
			}
		}
		
		// Multiline comments
		if (mcs_len && mce_len && !in_string) {
			if (in_comment) {
				row->hl[i] = HL_MLCOMMENT;
				if(!strncmp(&row->render[i], mce, mce_len)) {
					memset(&row->hl[i], HL_MLCOMMENT, mce_len);
					i += mce_len;
					in_comment = 0;
					prev_sep = 1;
					continue;
				} else {
					i++;
					continue;
				}
			} else if (!strncmp(&row->render[i], mcs, mcs_len)) {
				memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
				i += mcs_len;
				in_comment = 1;
				continue;
			}
		}
		
		// Strings
		if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
			if (in_string) {
				row->hl[i] = HL_STRING;
				
				if (c == '\\' && i + 1 < row->size) {
					row->hl[i + 1] = HL_STRING;
					i += 2;
					continue;
				}
				
				if (c == in_string) in_string = 0;
				i++;
				prev_sep = 1;
				continue;
				
			} else {
				if (c == '"' || c == '\'') {
					in_string = c;
					row->hl[i] = HL_STRING;
					i++;
					continue;
				}
			}
		}
		
		// Numbers 
		if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
			if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) || 
				(c == '.' && prev_hl == HL_NUMBER) ||
				(c == 'x' && prev_hl == HL_NUMBER)) {
					
				row->hl[i] = HL_NUMBER;
				i++;
				prev_sep = 0;
				continue;
			}
		}
		
		// Keywords
		if (prev_sep) {
			int j;
			for (j = 0; keywords[j]; j++) {
				int klen = strlen(keywords[j]);
				int kw2 = keywords[j][klen - 1] == '|';
				if (kw2) klen--;
				
				if (!strncmp(&row->render[i], keywords[j], klen) && 
					is_separator(row->render[i + klen])) {
					
					memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
					i += klen;
					break;
				}
			}
			if (keywords[j] != NULL) {
				prev_sep = 0;
				continue;
			}
		}
		
		prev_sep = is_separator(c);
		i++;
	}
	
	int changed = (row->hl_open_comment != in_comment);
	row->hl_open_comment = in_comment;
	if (changed && row->idx + 1 < E.numrows)
		editorUpdateSyntax(&E.row[row->idx + 1]);
}

int editorSyntaxToColor(int hl) {
	switch (hl) {
		case HL_NUMBER: return 31;
		case HL_KEYWORD2: return 32;
		case HL_KEYWORD1: return 33;
		case HL_CUSTOM: 
		case HL_MATCH: return 34;
		case HL_STRING: return 35;
		case HL_MLCOMMENT:
		case HL_COMMENT: return 36;
		
		default: return 37;
	}
}

void editorSelectSyntaxHighlight() {
	E.syntax = NULL;
	if (E.filename == NULL) return;
	
	char *ext = strrchr(E.filename, '.');
	
	for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
		struct editorSyntax *s = &HLDB[j];
		unsigned int i = 0;
		while (s->filematch[i]) {
			int is_ext = (s->filematch[i][0] == '.');
			if ((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
				(!is_ext && strstr(E.filename, s->filematch[i]))) {
				E.syntax = s;

				int filerow;
				for (filerow = 0; filerow < E.numrows; filerow++) {
					editorUpdateSyntax(&E.row[filerow]);
				}
				
				return;
			}
			i++;
		}
	}
}


/*** row operations ***/

// Convert cursor's position in file to it's rendered position which includes Tabs	
int editorRowCxToRx(erow *row, int cx) {
	int rx = 0;
	int j;
	for (j = 0; j < cx; j++) {
		if (row->chars[j] == '\t')
			rx += (HECTO_TAB_STOP - 1) - (rx % HECTO_TAB_STOP);
		rx++;
	}
	return rx;
}

// Convert cursor's rendered postion to it's position inside file
int editorRowRxToCx(erow *row, int rx) {
	int cur_rx = 0;
	int cx;
	for (cx = 0; cx < row->size; cx++) {
		if (row->chars[cx] == '\t')
			cur_rx += (HECTO_TAB_STOP - 1) - (cur_rx % HECTO_TAB_STOP);
		cur_rx++;
		
		if (cur_rx > rx) return cx;
	}
	return cx;
}

// Render row including Tabs -- influences rendered position of cursor
void editorUpdateRow(erow *row) {
	int tabs = 0;
	int j;
	for (j = 0; j < row->size; j++)
		if (row->chars[j] == '\t') tabs++;
	
	free(row->render);
	row->render = malloc(row->size + tabs*(HECTO_TAB_STOP - 1) + 1);
	
	int idx = 0;
	for (j = 0; j < row->size; j++) {
		if (row->chars[j] == '\t') {
			row->render[idx++] = ' ';
			while (idx % HECTO_TAB_STOP != 0) row->render[idx++] = ' ';
		} else {
			row->render[idx++] = row->chars[j];
		}
	}
	row->render[idx] = '\0';
	row->rsize = idx;
	editorUpdateSyntax(row);
}

// Parse row into editor memory
void editorInsertRow(int at, char *s, size_t len) {
	if (at < 0 || at > E.numrows) return;
	
	E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
	memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
	for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;
	
	E.row[at].idx = at;
	
	E.row[at].size = len;
	E.row[at].chars = malloc(len + 1);
	memcpy(E.row[at].chars, s, len);
	E.row[at].chars[len] = '\0';
	
	E.row[at].rsize = 0;
	E.row[at].render = NULL;
	E.row[at].hl = NULL;
	E.row[at].hl_open_comment = 0;
	editorUpdateRow(&E.row[at]);
	
	E.numrows++;
	E.dirty++;
}

void editorFreeRow(erow *row) {
	free(row->render);
	free(row->chars);
	free(row->hl);
}

void editorDelRow(int at) {
	if (at < 0 || at >= E.numrows) return;
	editorFreeRow(&E.row[at]);
	memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
	for (int j = at; j < E.numrows - 1; j++) E.row[j].idx--;
	E.numrows--;
	E.dirty++;
}

// Insert char at current cursor position
void editorRowInsertChar(erow *row, int at, int c) {
	if (at < 0 || at > row->size) at = row->size;
	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
	row->size++;
	row->chars[at] = c;
	editorUpdateRow(row);
	E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
	row->chars = realloc(row->chars, row->size + len + 1);
	memcpy(&row->chars[row->size], s, len);
	row->size += len;
	row->chars[row->size] = '\0';
	editorUpdateRow(row);
	E.dirty++;
}

// Overwrites character at given position with characters to its right
void editorRowDelChar(erow *row, int at) {
	if (at < 0 || at >= row->size) return;
	memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
	row->size--;
	editorUpdateRow(row);
	E.dirty++;
}	


/*** editor operations ***/

void editorInsertChar(int c) {
	if (E.cy == E.numrows) {
		editorInsertRow(E.numrows, "", 0);
	}
	editorRowInsertChar(&E.row[E.cy], E.cx, c);
	E.cx++;
}

void editorInsertNewLine() {
	if (E.cx == 0) {
		editorInsertRow(E.cy, "", 0);
	} else {
		erow *row = &E.row[E.cy];
		editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
		row = &E.row[E.cy];
		row->size = E.cx;
		row->chars[row->size] = '\0';
		editorUpdateRow(row);
	}
	E.cy++;
	E.cx = 0;
}

void editorDelChars() {
	if (E.cy == E.numrows && E.numrows >= 2) return;
	if (E.cx == 0 && E.cy == 0) return;
	
	erow *row = &E.row[E.cy];
	if (E.cx > 0) {
		editorRowDelChar(row, E.cx - 1);
		E.cx--;
	} else {
		E.cx = E.row[E.cy - 1].size;
		editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
		editorDelRow(E.cy);
		E.cy--;
	}
}


/*** file i/o ***/

char *editorRowsToString(int *buflen) {
	int totlen = 0;
	int j;
	for (j = 0; j < E.numrows; j++)
		totlen += E.row[j].size + 1;
	*buflen = totlen;
	
	char *buf = malloc(totlen);
	char *p = buf;
	for (j = 0; j < E.numrows; j++) {
		memcpy(p, E.row[j].chars, E.row[j].size);
		p += E.row[j].size;
		*p = '\n';
		p++;
	}
	
	return buf;
}

// Open and load file given its name
void editorOpen(char *filename) {
	free(E.filename);
	E.filename = strdup(filename);
	
	editorSelectSyntaxHighlight();
	
	FILE *fp = fopen(filename, "r");
	if (!fp) { // try to create if doesn't exist
		fp = fopen(filename, "ab+");
		fclose(fp);
		if ((fp = fopen(filename, "r")) == NULL)
			die("fopen");
	}

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' || 
							   line[linelen - 1] == '\r'))
			linelen--;
		editorInsertRow(E.numrows, line, linelen);
	}
	
	free(line);
	fclose(fp);
	
	E.dirty = 0;
}

void editorSave() {
	if (E.filename == NULL) {
		E.filename = editorPrompt("Save as: %s", NULL);
		if (E.filename == NULL) {
			editorSetStatusMessage("Save aborted");
			return;
		}
		editorSelectSyntaxHighlight();
	}
	
	int len;
	char *buf = editorRowsToString(&len);
	int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
	if (fd != -1) {
		if (ftruncate(fd, len) != -1) {
			if (write(fd, buf, len) == len) {
				close(fd);
				free(buf);
				editorSetStatusMessage("%d bytes written to disk", len);
				E.dirty = 0;
				return;
			}
		}
		close(fd);
	}
	free(buf);
	editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}


/*** find ***/

void editorFindCallback(char *query, int key) {
	static int last_match = -1;
	static int direction = 1;
	
	static int saved_hl_line;
	static char *saved_hl = NULL;
	
	if (saved_hl) {
		memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
		free(saved_hl);
		saved_hl = NULL;
	}
	
	if (key == '\r' || key == '\x1b') {
		last_match = -1;
		direction = 1;
		if (key == '\x1b') editorSetStatusMessage("Search Cancelled");
		return;
	} else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
		direction = 1;
	} else if (key == ARROW_LEFT || key == ARROW_UP) {
		direction = -1;
	} else {
		last_match = -1;
		direction = 1;
	}
	
	if (last_match == -1) direction = 1;
	int current = last_match;
	int i;
	for (i = 0; i < E.numrows; i++) {
		current += direction;
		if (current == -1) current = E.numrows - 1;
		else if (current == E.numrows) current = 0;
		
		/* TO DO */
		/* Right now search only finds first occurence on each line, that means it can't locate two substring on signle line */
		
		erow *row = &E.row[current];
		char *match = strstr(row->render, query);
		if (match) {
			last_match = current;
			E.cy = current;
			E.cx = editorRowRxToCx(row, match - row->render);
			E.rowoff = E.numrows;
			
			saved_hl_line = current;
			saved_hl = malloc(row->rsize);
			memcpy(saved_hl, row->hl, row->rsize);
			memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
			break;
		}
	}
}

void editorFind() {
	int saved_cx = E.cx;
	int saved_cy = E.cy;
	int saved_coloff = E.coloff;
	int saved_rowoff = E.rowoff;
	
	char *query = editorPrompt("Search: %s (Use ECS/Enter/Arrows)", 
								editorFindCallback);
	
	if (query) {
		free(query);
	} else {
		E.cx = saved_cx;
		E.cy = saved_cy;
		E.coloff = saved_coloff;
		E.rowoff = saved_rowoff;
	}
}


/*** append buffer ***/

// append buffer is used to display whole editor interface at once
struct abuf {
	char *b;
	int len;
};

#define ABUF_INIT {NULL, 0}

// everything to be displayed has to be parsed into the buffer first
void abAppend(struct abuf *ab, const char *s, int len) {
	char *new = realloc(ab->b, ab->len + len);
	
	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab) {
	free(ab->b);
}	


/*** -highlighting- ***/

void editorHighlightChar(struct abuf *ab, const char *s, const char *format) {
	
	// int format_len = 0;
	// while (format[format_len]) format_len++;
	
	abAppend(ab, format, 5);
	abAppend(ab, s, 1);
	if (format[2] == '4') abAppend(ab, "\x1b[49m", 5);
	else abAppend(ab, "\x1b[39m", 5);
	
}

void editorHighlightString(struct abuf *ab, const char *s, const char *format) {
	
	// int format_len = 0;
	int string_len = 0;
	// while (format[format_len]) format_len++;
	while (s[string_len]) string_len++; 
	
	if (string_len <= 1) editorHighlightChar(ab, s, format);
	
	abAppend(ab, format, 5);
	abAppend(ab, s, string_len);
	if (format[2] == '4') abAppend(ab, "\x1b[49m", 5);
	else abAppend(ab, "\x1b[39m", 5);
	
}


/*** output ***/

// Controls scrolling of text based on cursor position
void editorScroll() {
	E.rx = 0;
	if (E.cy < E.numrows) {
		E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
	}
	
	if (E.cy < E.rowoff) {
		E.rowoff = E.cy;
	}
	if (E.cy >= E.rowoff + E.screenrows) {
		E.rowoff = E.cy - E.screenrows + 1;
	}
	if (E.rx < E.coloff) {
		E.coloff = E.rx;
	}
	if (E.rx >= E.coloff + E.screencols) {
		E.coloff = E.rx - E.screencols + 1;
	}
}

// Resposible for drawing every row in a file
void editorDrawRows(struct abuf *ab) {
	int y;
	for (y = 0; y < E.screenrows; y++) {
		int filerow = y + E.rowoff;
		if (filerow >= E.numrows) {
			// Drawing empty lines
			if (E.numrows == 0 && y == E.screenrows / 3) {
				
				// Welcome string
				char welcome[80];
				int welcomelen = snprintf(welcome, sizeof(welcome),
					"Hecto editor -- version %s", HECTO_VERSION);
				if (welcomelen > E.screencols) welcomelen = E.screencols;
				int padding = (E.screencols - welcomelen) / 2;
				if (padding) {
					abAppend(ab, "~", 1);
					padding--;
				}
				while (padding--) abAppend(ab, " ", 1);
				abAppend(ab, welcome, welcomelen);
				
			} else {
				// Empty line character 
				abAppend(ab, "~", 1);
			}
			
		} else {
			/* WIP
			if (E.show_numline) {
				char buf[8];
				snprintf(buf, sizeof(buf), "%4d | ", filerow);
				abAppend(ab, "\x1b[34m", 5);
				abAppend(ab, buf, 7);
				abAppend(ab, "\x1b[m", 3);
				
			}
			*/
			
			// Drawing file lines
			int len = E.row[filerow].rsize - E.coloff;
			if (len < 0) len = 0;
			if (len > E.screencols) len = E.screencols;
			char *c = &E.row[filerow].render[E.coloff];
			unsigned char *hl = &E.row[filerow].hl[E.coloff];
			int current_color = -1;
			int j;
			
			// Drawing individual characters of the row
			for (j = 0; j < len; j++) {
				
				// Non-printanle characters
				if (iscntrl(c[j])) {
					char sym = (c[j] <= 26) ? '@' + c[j] : '?';
					abAppend(ab, "\x1b[7m", 4);
					abAppend(ab, &sym, 1);
					abAppend(ab, "\x1b[m", 3);
					if (current_color != -1) {
						char buf[16];
						int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
						abAppend(ab, buf, clen);
					}
				
				// Normal characters
				} else if (hl[j] == HL_NORMAL) {
					if (current_color != -1) {
						abAppend(ab, "\x1b[39m", 5);
						current_color = -1;
					}
					abAppend(ab, &c[j], 1);
					
				// Syntax highlighing
				} else {
					
					int color = editorSyntaxToColor(hl[j]);
					if (color != current_color) {
						current_color = color;
						char buf[16];
						int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
						abAppend(ab, buf, clen);
					}
					abAppend(ab, &c[j], 1);
				}
			}
			abAppend(ab, "\x1b[39m", 5);
		}
		
		// Move cursor to the upper-left corner and append new line
		abAppend(ab, "\x1b[K", 3);
		abAppend(ab, "\r\n", 2);
	}
}

// Draws status bar at the bottom of screen with file information
void editorDrawStatusBar(struct abuf *ab) {
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	
	abAppend(ab, "\x1b[7m", 4);
	
	char status[80], rstatus[80];
	
	int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
		E.filename ? E.filename : "[No Name]", E.numrows,
		E.dirty ? "(modified)" : "");
		
	int rlen = snprintf(rstatus, sizeof(rstatus), "%s  |  Ln %d/%d, Col %d/%d",
		E.syntax ? E.syntax->filetype : "-",
		E.cy + 1, E.numrows, E.cx, row ? row->size : 0);
		
	if (len > E.screencols) len = E.screencols;
	abAppend(ab, status, len);
	while (len < E.screencols) {
		if (E.screencols - len == rlen) {
			abAppend(ab, rstatus, rlen);
			break;
		} else {
			abAppend(ab, " ", 1);
			len++;
		}
	}
	
	abAppend(ab, "\x1b[m", 3);
	abAppend(ab, "\r\n", 2);
}

// Draws a message bar underneath the status bar
void editorDrawMessageBar(struct abuf *ab) {
	abAppend(ab, "\x1b[K", 3);
	int msglen = strlen(E.statusmsg);
	if (msglen > E.screencols) msglen = E.screencols;
	if (msglen && time(NULL) - E.statusmsg_time < 5)
		abAppend(ab, E.statusmsg, msglen);
}

// Set message to be displayed in a message bar
void editorSetStatusMessage(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
	va_end(ap);
	E.statusmsg_time = time(NULL);
}

// Main drawing function -- controls the order of other draw functions 
void editorRefreshScreen() {
	editorScroll();
	
	struct abuf ab = ABUF_INIT;
	
	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[H", 3);
	
	editorDrawRows(&ab);
	editorDrawStatusBar(&ab);
	editorDrawMessageBar(&ab);
	
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,
											  (E.rx - E.coloff) + 1);
	abAppend(&ab, buf, strlen(buf));
	
	abAppend(&ab, "\x1b[?25h", 6);
	
	if (write(STDOUT_FILENO, ab.b, ab.len) != ab.len) editorSetStatusMessage("An error occured while refreshing the screen. Some values were not displayed.");
	abFree(&ab);
}


/*** input ***/

char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
	size_t bufsize = 128;
	char *buf = malloc(bufsize);
	
	size_t buflen = 0;
	buf[0] = '\0';
	
	while (1) {
		editorSetStatusMessage(prompt, buf);
		editorRefreshScreen();
		
		int c = editorReadKey();
		if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
			if (buflen != 0) buf[--buflen] = '\0';
		} else if (c == '\x1b') {
			editorSetStatusMessage("");
			if (callback) callback(buf, c);
			free(buf);
			return NULL;
		} else if  (c == '\r') {
			if (buflen != 0) {
				editorSetStatusMessage("");
				if (callback) callback(buf, c);
				return buf;
			}
		} else if (!iscntrl(c) && c < 128) {
			if (buflen == bufsize - 1) {
				bufsize *= 2;
				buf = realloc(buf, bufsize);
			}
			buf[buflen++] = c;
			buf[buflen] = '\0';
		}
		if (callback) callback(buf, c);
	}
}

// Moves cursor based on given key
void editorMoveCursor(int key) {
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	
	switch (key) {
		case ARROW_LEFT:
			if (E.cx != 0) {
				E.cx--;
			} else if (E.cy > 0) {
				E.cy--;
				E.cx = E.row[E.cy].size;
			}
			break;
		case ARROW_DOWN:
			if (E.cy < E.numrows)
				E.cy++;
			break;
		case ARROW_RIGHT:
			if (row && E.cx < row->size) {
				E.cx++;
			} else if (row && (E.cx >= row->size) && (E.cy < E.numrows - 1)) {
				E.cx = 0;
				E.cy++;
			}
			break;
		case ARROW_UP:
			if (E.cy != 0)
				E.cy--;
			break;
	}
	
	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	int rowlen = row ? row->size : 0;
	if (E.cx > rowlen) {
		E.cx = rowlen;
	}
	
	if (E.show_numline) E.cx += 7;
}

// Processes pressed keys and special keys
void editorProcessKeypress() {
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	
	static int quit_times = HECTO_QUIT_CONFIRM;
	
	int c = editorReadKey();
	
	switch (c) {
		case '\r':
			editorInsertNewLine();
			break;
			
		case CTRL_KEY('q'):
			if (E.dirty && quit_times > 0) {
				editorSetStatusMessage("WARNING! File has unsaved changes. Press Ctrl-Q %d more times to confirm quit.", quit_times);
				quit_times--;
				return;
			}
			clearScreen();
			exit(0);
			break;
		
		case CTRL_KEY('s'):
			editorSave();
			break;
		
		case CTRL_KEY('f'):
			editorFind();
			break;
			
		case CTRL_KEY('r'):
			/* TO DO */
			E.show_numline = E.show_numline ? 0 : 1;
			break;
			
		case HOME_KEY:
			E.cx = 0;
			break;
		
		case END_KEY:
			if (E.cy < E.numrows)
				E.cx = E.row[E.cy].size;
			break;
			
		case BACKSPACE:
		case CTRL_KEY('h'):
		case DEL_KEY:
			if (c == DEL_KEY) {
				if ((!row) || ((E.cx >= row->size) && (E.cy >= E.numrows - 1))) break;
				else editorMoveCursor(ARROW_RIGHT);
			}
			editorDelChars();
			break;
		
		case PAGE_DOWN:
		case PAGE_UP:
			{
				if (c == PAGE_UP) {
					E.cy = E.rowoff;
				} else if (c == PAGE_DOWN) {
					E.cy = E.rowoff + E.screenrows - 1;
					if (E.cy > E.numrows) E.cy = E.numrows;
				}
				
				int times = E.screenrows;
				while (times--)
					editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
			}
			break;
		
		case ARROW_UP:
		case ARROW_DOWN:
		case ARROW_LEFT:
		case ARROW_RIGHT:
			editorMoveCursor(c);
			break;
		
		case CTRL_KEY('l'):
		case '\x1b':
			break;
		
		default:
			editorInsertChar(c);
			break;
	}
	
	quit_times = HECTO_QUIT_CONFIRM;
}


/*** init ***/

// Initiate main editor struct, which contains most of the used data
void initEditor() {
	E.cx = 0;
	E.cy = 0;
	E.rx = 0;
	E.rowoff = 0;
	E.coloff = 0;
	E.numrows = 0;
	E.row = NULL;
	E.show_numline = 0; // TO DO
	E.dirty = 0;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;
	E.syntax = NULL;
	
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) 
		die("getWindowSize");
	E.screenrows -= 2;
}

int main(int argc, char *argv[]) 
{
	enableRawMode();
	initEditor();
	if (argc >= 2) {
		editorOpen(argv[1]);
	}
	
	editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-F = find | Ctrl-R = toggle line numbers | Ctrl-Q = quit");
	
	while (1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}
	return 0;
}
