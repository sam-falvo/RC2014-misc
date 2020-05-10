/* Build with:
 * gcc -o send send.c
 *
 * Usage:
 * $ ./send <filename>
 *
 * See README.md for the protocol that it speaks.
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <unistd.h>
#include <termios.h>


static int bytes_sent;


static void
error_happened(char *msg) {
	fprintf(stderr, "Error: %s\n", msg);
}


static int
out(int ch) {
	int echo;

	printf("%c", ch);
	fflush(stdout);

	echo = fgetc(stdin);
	return echo == ch;
}


static void
send(FILE *fp) {
	uint8_t buffer[9];
	int checksum, i, h, l, done, size, prompt;

	bytes_sent = 0;
	while(!feof(fp)) {
		memset(buffer, 0, 9);
		size = fread(buffer, 1, 8, fp);
		if(size == 0) {
			if(feof(fp)) break;
			error_happened("file read");
			return;
		} else {
			checksum = 0;

			/* Ask receiver to record 8 bytes */

			out('Z');
			for(i = 0; i < 8; i++) {
				h = (buffer[i] & 0xF0) >> 4;
				l = (buffer[i] & 0x0F);
				checksum += h+l;
				out('@'+h);
				out('@'+l);

				bytes_sent++;
				if((bytes_sent & 0xFF) == 0) {
					fprintf(stderr, "\r%d bytes sent.   ", bytes_sent);
				}
			}
			out('@'+(checksum & 15));
			out('\r');
			out('\n');

			/* Wait for the ? response. */

			prompt = fgetc(stdin);
			if(prompt != '?') {
				error_happened("'? ' expected");
				return;
			}

			prompt = fgetc(stdin);
			if(prompt != ' ') {
				error_happened("'? ' expected");
				return;
			}
		}
	}

	/* Ask receiver to close currently open file. */

	printf("C\r\n");
	fflush(stdout);
}


int
main(int argc, char *argv[]) {
	FILE *fp = NULL;
	struct termios ttysave, ttynonblock;

	tcgetattr(STDIN_FILENO, &ttysave);
	ttynonblock = ttysave;
	ttynonblock.c_lflag &= ~(ICANON | ECHO);
	ttynonblock.c_cc[VMIN] = 1;
	tcsetattr(STDIN_FILENO, TCSANOW, &ttynonblock);

	fp = fopen(argv[1], "rb");
	if(fp) {
		send(fp);
		fclose(fp);
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &ttysave);
}
