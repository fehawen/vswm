#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

static void loop_events(void);
static void open_display_connection(void);
static void close_display_connection(void);

static Display *display;

void loop_events(void)
{
	XEvent event;

	while (1) {
		XNextEvent(display, &event);

		/* Handle events */
	}
}

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

	loop_events();

	close_display_connection();

	return EXIT_SUCCESS;
}
