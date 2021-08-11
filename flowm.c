#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#define CTRL ControlMask
#define MOD Mod4Mask
#define SHIFT ShiftMask

typedef struct {
	const char *argv;
} Arg;

typedef struct {
	unsigned int modifiers;
	KeySym symbol;
	void (*function)(const Arg arg);
	const Arg arg;
} KeyBinding;

typedef void (*EventHandler)(XEvent *e);

static void close_display_connection(void);
static void dummy_command(const Arg arg);
static void grab_input(void);
static void handle_button_press(XEvent *event);
static void handle_key_press(XEvent *event);
static void loop_events(void);
static void open_display_connection(void);

static Display *display;

static KeyBinding key_bindings[] = {
	{ MOD | SHIFT, XK_1, dummy_command, { "foo" } },
	{ MOD | SHIFT, XK_2, dummy_command, { "bar" } },
	{ MOD | SHIFT, XK_3, dummy_command, { "baz" } },
};

static const EventHandler event_handler[LASTEvent] = {
	[ButtonPress] = handle_button_press,
	[KeyPress] = handle_key_press,
};

void close_display_connection(void)
{
	XCloseDisplay(display);

	puts("flowm: Closed display connection.");
}

void dummy_command(const Arg arg)
{
	printf("flowm: %s\n", &arg.argv[0]);
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

void grab_input(void)
{
	unsigned int i;

	for (i = 0; i < sizeof(key_bindings)/sizeof(*key_bindings); i++)
		XGrabKey(
			display,
			XKeysymToKeycode(display, key_bindings[i].symbol),
			key_bindings[i].modifiers,
			DefaultRootWindow(display),
			True,
			GrabModeAsync,
			GrabModeAsync
		);

	XGrabButton(
		display,
		1,
		MOD,
		DefaultRootWindow(display),
		True,
		ButtonPressMask,
		GrabModeAsync,
		GrabModeAsync,
		None,
		None
	);
}

void handle_button_press(XEvent *event)
{
	if (!event->xbutton.subwindow)
		return;

	XRaiseWindow(display, event->xbutton.subwindow);

	XSetInputFocus(
		display,
		event->xbutton.subwindow,
		RevertToParent,
		CurrentTime
	);
}

void handle_key_press(XEvent *event)
{
	KeySym symbol;
	unsigned int i;

	symbol = XkbKeycodeToKeysym(display, event->xkey.keycode, 0, 0);

	if (!symbol)
		return;

	for (i = 0; i < sizeof(key_bindings)/sizeof(*key_bindings); i++)
		if (symbol == key_bindings[i].symbol)
			key_bindings[i].function(key_bindings[i].arg);
}

void loop_events(void)
{
	XEvent event;

	while (1) {
		XNextEvent(display, &event);

		if (event_handler[event.type])
			event_handler[event.type](&event);
	}
}

int main(void)
{
	open_display_connection();
	grab_input();
	loop_events();
	close_display_connection();

	return EXIT_SUCCESS;
}
