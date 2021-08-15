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

typedef struct Client Client;
typedef struct KeyBinding KeyBinding;

struct Client {
	int fullscreen;
	int height, width;
	int x, y;
	Window window;
};

struct KeyBinding {
	unsigned int modifier;
	KeySym key_sym;
	void (*function)(char *command);
	char *command;
};

typedef void (*EventHandler)(XEvent *e);

static void add_client(Window window, XWindowAttributes *attributes);
static int error_handler(Display *display, XErrorEvent *ev);
static void fullscreen_client(char *command);
static void grab_input(void);
static void handle_button_press(XEvent *event);
static void handle_key_press(XEvent *event);
static void handle_map_request(XEvent *event);
static void loop_events(void);
static void move_resize_client(char *command);
static void start_wm(void);
static void stop_wm(void);
static void spawn_process(char *command);

static Client *focused_client;
static Display *display;
static Window root;
static int running;
static int screen;
static int screen_width;
static int screen_height;

static KeyBinding key_bindings[] = {
	{ MOD, XK_Return, spawn_process, TERMINAL },
	{ MOD, XK_d, spawn_process, MENU },
	{ MOD, XK_h, move_resize_client, MOVE_WEST },
	{ MOD, XK_j, move_resize_client, MOVE_SOUTH },
	{ MOD, XK_k, move_resize_client, MOVE_NORTH },
	{ MOD, XK_l, move_resize_client, MOVE_EAST },
	{ MOD | SHIFT, XK_h, move_resize_client, RESIZE_WEST },
	{ MOD | SHIFT, XK_j, move_resize_client, RESIZE_SOUTH },
	{ MOD | SHIFT, XK_k, move_resize_client, RESIZE_NORTH },
	{ MOD | SHIFT, XK_l, move_resize_client, RESIZE_EAST },
	{ MOD, XK_f, fullscreen_client, NULL },
};

static const EventHandler event_handler[LASTEvent] = {
	[ButtonPress] = handle_button_press,
	[KeyPress] = handle_key_press,
	[MapRequest] = handle_map_request,
};

void add_client(Window window, XWindowAttributes *attributes)
{
	Client *client;

	client = (Client *) calloc(1, sizeof(Client));

	if (!client) {
		fputs("flowm: Failed to calloc Client\n", stderr);
		exit(EXIT_FAILURE);
	}

	client->window = window;
	client->x = attributes->x;
	client->y = attributes->y;
	client->height = attributes->height;
	client->width = attributes->width;
	client->fullscreen = 0;

	/* Probably need to set default initial window size,
	 * and possibly center window or set a default initial position */

	XMapWindow(display, client->window);

	/* Might need XSelectInput here as well */

	XSetInputFocus(display, client->window, RevertToParent, CurrentTime);

	/* Might need to do more here */
	focused_client = client;
}

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

void fullscreen_client(char *command)
{
	int height, width, x, y;

	(void)command;

	if (!focused_client)
		return;

	/* Check override_redirect attributes to know if should be ignored */

	if (focused_client->fullscreen) {
		height = focused_client->height;
		width = focused_client->width;

		x = focused_client->x;
		y = focused_client->y;

		focused_client->fullscreen = 0;
	} else {
		height = screen_height;
		width = screen_width;

		x = 0;
		y = 0;

		focused_client->fullscreen = 1;
	}

	XMoveResizeWindow(
		display,
		focused_client->window,
		x,
		y,
		width,
		height
	);
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
	KeySym key_sym;
	XKeyEvent key_event;

	/* Check override_redirect attributes to know if should be ignored */

	key_event = event->xkey;
	key_sym = XkbKeycodeToKeysym(display, key_event.keycode, 0, 0);

	if (!key_sym)
		return;

	for (i = 0; i < sizeof(key_bindings) / sizeof(struct KeyBinding); i++)
		if (key_sym == key_bindings[i].key_sym
			&& key_bindings[i].modifier == key_event.state)
			key_bindings[i].function(key_bindings[i].command);
}

void handle_map_request(XEvent *event)
{
	XWindowAttributes attributes;
	XMapRequestEvent *request;

	request = &event->xmaprequest;

	if (!XGetWindowAttributes(display, request->window, &attributes))
        return;

	if (attributes.override_redirect)
        return;

	/* Might need XSelectInput here */

	add_client(request->window, &attributes);
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

void move_resize_client(char *command)
{
	char direction;
	int move, height, width, x, y;

	if (!focused_client)
		return;

	/* Check override_redirect attributes to know if should be ignored */

	if (focused_client->fullscreen)
		return;

	direction = command[1];
	move = command[0] == 'm';
	height = focused_client->height;
	width = focused_client->width;
	x = focused_client->x;
	y = focused_client->y;

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

	if (move) {
		XMoveWindow(display, focused_client->window, x, y);

		focused_client->x = x;
		focused_client->y = y;
	} else {
		XResizeWindow(display, focused_client->window, width, height);

		focused_client->height = height;
		focused_client->width = width;
	}
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

	/* Figure out which of these we actually need... */
	attributes.event_mask = SubstructureRedirectMask /* most likely */
		| SubstructureNotifyMask /* most likely */
		| ButtonPressMask /* most likely */
		| PointerMotionMask /* probably not, unless move windows with mouse */
		| EnterWindowMask /* probably not, no sloppy focus */
		| LeaveWindowMask /* probably not, no sloppy focus */
		| StructureNotifyMask /* most likely */
		| PropertyChangeMask; /* most likely */

	XChangeWindowAttributes(display, root, CWEventMask | CWCursor, &attributes);
	XSelectInput(display, root, attributes.event_mask);

	focused_client = NULL;

	running = 1;
}

void stop_wm(void)
{
	XCloseDisplay(display);

	/* Free clients, etc */
}

/* Double fork to avoid zombie processes the way i3wm does it */
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
