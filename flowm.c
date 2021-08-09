#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

static void open_display_connection(void);
static void close_display_connection(void);

static Display *display;

void open_display_connection(void)
{
	display = XOpenDisplay(NULL);

	if (!display) {
		puts("flowm: Failed to open display connection.");
		exit(EXIT_FAILURE);
	}

	puts("flowm: Opened display connection.");
}

void close_display_connection(void)
{
	XCloseDisplay(display);

	puts("flowm: Closed display connection.");
}

int main(void)
{
	open_display_connection();

	/* Grab input */

	/* Handle input */

	/* Loop events */

	/* Handle events */

	close_display_connection();

	return EXIT_SUCCESS;
}
