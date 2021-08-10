#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

static void open_display_connection(void);
static void close_display_connection(void);

static void handle_button_press(XEvent *event);
static void handle_button_release(XEvent *event);
static void handle_motion_notify(XEvent *event);

static void grab_input(void);
static void loop_events(void);

static Display *display;

static void (*event_handlers[LASTEvent])(XEvent *event) = {
	[ButtonPress] = handle_button_press, /* ButtonPressMask */
	[ButtonRelease] = handle_button_release, /* ButtonReleaseMask */
	[MotionNotify] = handle_motion_notify /* PointerMotionMask */
};

void handle_button_press(XEvent *event)
{
	(void)event;
	puts("flowm: ButtonPress");
}

void handle_button_release(XEvent *event)
{
	(void)event;
	puts("flowm: ButtonRelease");
}

void handle_motion_notify(XEvent *event)
{
	(void)event;
	puts("flowm: MotionNotify");
}

void grab_input(void)
{
	XGrabButton(
		display, /* display */
		1, /* button */
		Mod4Mask, /* modifiers */
		DefaultRootWindow(display), /* grab window */
		True, /* owner events */
		ButtonPressMask|ButtonReleaseMask|PointerMotionMask, /* event mask */
		GrabModeAsync, /* pointer mode */
		GrabModeAsync, /* keyboard mode */
		None, /* confine to */
		None /* cursor */
	);
}

void loop_events(void)
{
	XEvent event;

	while (1) {
		XNextEvent(display, &event);

		if (event_handlers[event.type])
			event_handlers[event.type](&event);
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

	grab_input();

	/* Handle input */

	loop_events();

	close_display_connection();

	return EXIT_SUCCESS;
}
