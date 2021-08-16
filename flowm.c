#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#define CTRL ControlMask
#define MOD Mod4Mask
#define SHIFT ShiftMask
#define TERMINAL "xterm"
#define MENU "dmn"
#define MOVE_EAST "me"
#define MOVE_NORTH "mn"
#define MOVE_SOUTH "ms"
#define MOVE_WEST "mw"
#define RESIZE_EAST "re"
#define RESIZE_NORTH "rn"
#define RESIZE_SOUTH "rs"
#define RESIZE_WEST "rw"
#define MOVE_STEP 50
#define RESIZE_STEP 50
#define WINDOW_MIN_HEIGHT 100
#define WINDOW_MIN_WIDTH 100

/* Arguably common, but taken from TinyWM in this case */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef void (*EventHandler)(XEvent *event);
typedef struct KeyBinding KeyBinding;

struct KeyBinding {
	unsigned int modifier;
	KeySym key_sym;
	void (*function)(char *command);
	char *command;
};

static void center_window(char *command);
static int current_not_valid(void);
static int error_handler(Display *display, XErrorEvent *ev);
static void focus_current(void);
static void fullscreen_window(char *command);
static void grab_input(void);
static void handle_button_press(XEvent *event);
static void handle_destroy_notify(XEvent *event);
static void handle_key_press(XEvent *event);
static void handle_map_request(XEvent *event);
static void kill_window(char *command);
static void loop_events(void);
static void move_resize_window(char *command);
static void start_wm(void);
static void stop_wm(void);
static void spawn_process(char *command);
static void quit(char *command);

static Window current;
static Display *display;
static Window dummy;
static Window root;
static int running;
static int screen;
static int screen_width;
static int screen_height;

/* Very similar to the way DWM, SOWM and many others do it */
static KeyBinding key_bindings[] = {
	{ MOD, XK_Return, spawn_process, TERMINAL },
	{ MOD, XK_d, spawn_process, MENU },
	{ MOD, XK_h, move_resize_window, MOVE_WEST },
	{ MOD, XK_j, move_resize_window, MOVE_SOUTH },
	{ MOD, XK_k, move_resize_window, MOVE_NORTH },
	{ MOD, XK_l, move_resize_window, MOVE_EAST },
	{ MOD | SHIFT, XK_h, move_resize_window, RESIZE_WEST },
	{ MOD | SHIFT, XK_j, move_resize_window, RESIZE_SOUTH },
	{ MOD | SHIFT, XK_k, move_resize_window, RESIZE_NORTH },
	{ MOD | SHIFT, XK_l, move_resize_window, RESIZE_EAST },
	{ MOD, XK_f, fullscreen_window, NULL },
	{ MOD, XK_c, center_window, NULL },
	{ MOD | SHIFT, XK_q, kill_window, NULL },
	{ MOD | CTRL | SHIFT, XK_x, quit, NULL },
};

/* Same goes for these... */
static const EventHandler event_handler[LASTEvent] = {
	[ButtonPress] = handle_button_press,
	[DestroyNotify] = handle_destroy_notify,
	[KeyPress] = handle_key_press,
	[MapRequest] = handle_map_request,
};

void center_window(char *command)
{
	int x, y;
	XWindowAttributes attributes;

	(void)command;

	if (current_not_valid())
		return;

	if (!XGetWindowAttributes(display, current, &attributes))
		return;

	x = (screen_width - attributes.width) / 2;
	y = (screen_height - attributes.height) / 2;

	XMoveWindow(display, current, x, y);
}

int current_not_valid(void)
{
	if (!current || current == dummy || current == root)
		return 1;

	return 0;
}

/* This ugly bugger will ignore all errors once we're up
 * and running and have checked that no other WMs are running.
 * If the shit hits the fan, I'm afraid we'll be none the wiser. */
int error_handler(Display *display, XErrorEvent *event)
{
	(void)display;

	if (!running
		&& event->error_code == BadAccess
		&& event->resourceid == root) {
		fputs("flowm: Another WM is already running\n", stderr);
		exit(EXIT_FAILURE);
	}

	return 0;
}

void focus_current(void)
{
	/* Just to be paranoidly safe */
	if (!current || current == root) {
		current = dummy;
		focus_current();
		return;
	}

	XSetInputFocus(display, current, RevertToParent, CurrentTime);
	XRaiseWindow(display, current);
}

void fullscreen_window(char *command)
{
	(void)command;

	if (current_not_valid())
		return;

	XMoveResizeWindow(display, current, 0, 0, screen_width, screen_height);
}

void grab_input(void)
{
	size_t i;

	for (i = 0; i < sizeof(key_bindings) / sizeof(struct KeyBinding); i++)
		XGrabKey(
			display,
			XKeysymToKeycode(display, key_bindings[i].key_sym),
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
	Window window;

	window = event->xbutton.subwindow;

	if (window && window != dummy)
		current = window;
	else if (event->xbutton.window == root)
		current = dummy;

	focus_current();
}

void handle_destroy_notify(XEvent *event)
{
	(void)event;

	if (!current)
		current = dummy;
}

void handle_key_press(XEvent *event)
{
	size_t i;
	KeySym key_sym;
	XKeyEvent key_event;

	key_event = event->xkey;
	key_sym = XkbKeycodeToKeysym(display, key_event.keycode, 0, 0);

	if (!key_sym)
		return;

	/* The commonly used clean mask "mask & ~(0|LockMask) & (ShiftMask|Con...)"
	 * approach does not _seem_ to be necessary if we just compare modifier to
	 * state directly? But I could be wrong (and probably am), although I
	 * haven't ran into any issues by doig it this way so far... */
	for (i = 0; i < sizeof(key_bindings) / sizeof(struct KeyBinding); i++)
		if (key_sym == key_bindings[i].key_sym
			&& key_bindings[i].modifier == key_event.state)
			key_bindings[i].function(key_bindings[i].command);
}

void handle_map_request(XEvent *event)
{
	XWindowAttributes attributes;
	Window window;

	window = event->xmaprequest.window;

	if (!XGetWindowAttributes(display, window, &attributes))
		return;

	if (attributes.override_redirect)
		return;

	XSelectInput(display, window, StructureNotifyMask);
	XMapWindow(display, window);

	current = window;

	center_window(NULL);
	focus_current();
}

void kill_window(char *command)
{
	(void)command;

	if (current_not_valid())
		return;

	/* XSetCloseDownMode may not be necessary... */
	XSetCloseDownMode(display, DestroyAll);
	XKillClient(display, current);

	current = dummy;

	focus_current();
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

void move_resize_window(char *command)
{
	char direction;
	int move, height, width, x, y;
	XWindowAttributes attributes;

	if (current_not_valid())
		return;

	if (!XGetWindowAttributes(display, current, &attributes))
		return;

	/* This "direction" and "move" approach is gaspingly ugly,
	 * but it does get the job done. */
	direction = command[1];
	move = command[0] == 'm';

	height = attributes.height;
	width = attributes.width;

	x = attributes.x;
	y = attributes.y;

	switch (direction) {
		case 'n':
			height = MAX(WINDOW_MIN_HEIGHT, height - RESIZE_STEP);
			y = y - MOVE_STEP;
			break;
		case 'e':
			width = width + RESIZE_STEP;
			x = x + MOVE_STEP;
			break;
		case 's':
			height = height + RESIZE_STEP;
			y = y + MOVE_STEP;
			break;
		case 'w':
			width = MAX(WINDOW_MIN_WIDTH, width - RESIZE_STEP);
			x = x - MOVE_STEP;
			break;
	}

	if (move)
		XMoveWindow(display, current, x, y);
	else
		XResizeWindow(display, current, width, height);
}

void start_wm(void)
{
	XSetWindowAttributes attributes;

	XSetErrorHandler(error_handler);

	display = XOpenDisplay(NULL);

	if (!display) {
		fprintf(stderr, "flowm: Failed to open display\n");
		exit(EXIT_FAILURE);
	}

	root = DefaultRootWindow(display);
	screen = DefaultScreen(display);

	screen_width = XDisplayWidth(display, screen);
	screen_height = XDisplayHeight(display, screen);

	XSelectInput(display, root, SubstructureRedirectMask);

	/* Cursor the easy way, as SOWM does it */
	XDefineCursor(display, root, XCreateFontCursor(display, 68));

	/* Hide the bastard as we don't need to see it, we just use it
	 * to prevent focusing the root window (which is bad, I've been told). */
	dummy = XCreateSimpleWindow(display, root, -2, -2, 1, 1, 0, 0, 0);
	attributes.override_redirect = True;

	XChangeWindowAttributes(display, dummy, CWOverrideRedirect, &attributes);
	XMapWindow(display, dummy);

	current = dummy;
	running = 1;
}

void stop_wm(void)
{
	/* Should kill all windows here later... */

	XUngrabPointer(display, CurrentTime);
	XUngrabKey(display, AnyKey, AnyModifier, root);

	XUnmapWindow(display, dummy);
	XDestroyWindow(display, dummy);

	XCloseDisplay(display);
}

/* Double fork to avoid zombie processes the way i3WM does it */
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

void quit(char *command)
{
	(void)command;

	running = 0;
}

int main(void)
{
	start_wm();
	grab_input();
	loop_events();
	stop_wm();

	return EXIT_SUCCESS;
}
