        
#include <stdio.h>
#include "terminal.h"


int main() {
	enableRawMode();
        printf("A helper program which displays the ascii value of keyboard inputs\n(Press 'q' to exit)\n");
	char c = editorReadKey();
	while (c != 'q') {
		printf("%d (%c)\r\n", c, c);
		c = editorReadKey();
	}
	return 0;
}

