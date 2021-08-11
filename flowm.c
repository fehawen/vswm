#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#define CTRL ControlMask
#define MOD Mod4Mask
#define SHIFT ShiftMask

typedef struct KeyBinding KeyBinding;

struct KeyBinding {
	unsigned int modifier;
	KeySym keysym;
	void (*function)(char *command);
	char *command;
};

typedef void (*EventHandler)(XEvent *e);

static int error_handler(Display *display, XErrorEvent *ev);
static void grab_input(void);
static void handle_button_press(XEvent *event);
static void handle_key_press(XEvent *event);
static void loop_events(void);
static void start_wm(void);
static void stop_wm(void);
static void spawn_process(char *command);

static Display *display;
static Window root;
static int running = 0;

static KeyBinding key_bindings[] = {
	{ MOD, XK_Return, spawn_process, "xterm" },
};

static const EventHandler event_handler[LASTEvent] = {
	[ButtonPress] = handle_button_press,
	[KeyPress] = handle_key_press,
};

int error_handler(Display *display, XErrorEvent *event)
{
	(void)display;

	if (!running
	&& event->error_code == BadAccess
	&& event->resourceid == root) {
		fprintf(stderr, "flowm: Another WM is already running\n");
		exit(EXIT_FAILURE);
	}

	return 0;
}

void grab_input(void)
{
	size_t i;

	for (i = 0; i < sizeof(key_bindings) / sizeof(struct KeyBinding); i++)
		XGrabKey(
			display,
			XKeysymToKeycode(display, key_bindings[i].keysym),
			key_bindings[i].modifier,
			root,
			True,
			GrabModeAsync,
			GrabModeAsync
		);

	XGrabButton(
		display,
		1,
		MOD,
		root,
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
	size_t i;
	KeySym keysym;

	keysym = XkbKeycodeToKeysym(display, event->xkey.keycode, 0, 0);

	if (!keysym)
		return;

	for (i = 0; i < sizeof(key_bindings) / sizeof(struct KeyBinding); i++)
		if (keysym == key_bindings[i].keysym)
			key_bindings[i].function(key_bindings[i].command);
}

void loop_events(void)
{
	XEvent event;

	while (running) {
		XNextEvent(display, &event);

		if (event_handler[event.type])
			event_handler[event.type](&event);
	}
}

void start_wm(void)
{
	XSetErrorHandler(error_handler);

	display = XOpenDisplay(NULL);

	if (!display) {
		fprintf(stderr, "flowm: Failed to open display\n");
		exit(EXIT_FAILURE);
	}

	root = DefaultRootWindow(display);

	running = 1;
}

void stop_wm(void)
{
	XCloseDisplay(display);
}

void spawn_process(char *command)
{
	if (fork() == 0) {
		if (fork() == 0) {
			if (display)
				close(ConnectionNumber(display));

			setsid();
			execl("/bin/sh", "sh", "-c", command, NULL);

			exit(EXIT_FAILURE);
		}
		else {
			exit(EXIT_SUCCESS);
		}
	}
}

int main(void)
{
	start_wm();
	grab_input();
	loop_events();
	stop_wm();

	return EXIT_SUCCESS;
}
