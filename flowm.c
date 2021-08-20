#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

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
#define SNAP_HALF_EAST "he"
#define SNAP_HALF_NORTH "hn"
#define SNAP_HALF_SOUTH "hs"
#define SNAP_HALF_WEST "hw"
#define SNAP_QUARTER_EAST "qe"
#define SNAP_QUARTER_NORTH "qn"
#define SNAP_QUARTER_SOUTH "qs"
#define SNAP_QUARTER_WEST "qw"
#define MOVE_STEP 50
#define RESIZE_STEP 50
#define WINDOW_MIN_HEIGHT 100
#define WINDOW_MIN_WIDTH 100

/* Arguably common, but taken from TinyWM in this case */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef void (*EventHandler)(XEvent *event);
typedef struct Key Key;

/* Very similar to the way DWM, SOWM and many others do it */
struct Key {
	unsigned int modifier;
	KeySym keysym;
	void (*function)(char *command);
	char *command;
};

static void center_window(char *command);
static void create_dummy(void);
static int current_not_valid(void);
static int error_handler(Display *display, XErrorEvent *event);
static void focus_current(void);
static void fullscreen_window(char *command);
static void grab_input(void);
static void handle_button_press(XEvent *event);
static void handle_configure_request(XEvent *event);
static void handle_key_press(XEvent *event);
static void handle_mapping_notify(XEvent *event);
static void handle_map_request(XEvent *event);
static void handle_unmap_notify(XEvent *event);
static void init_wm(void);
static void kill_window(char *command);
static void loop_events(void);
static void move_resize_window(char *command);
static void reset_window(void);
static void snap_window(char *command);
static void spawn_process(char *command);

static Display *display;
static Window root, current, dummy;
static int screen, screen_width, screen_height;

static Key keys[] = {
	{ MOD, XK_Return, spawn_process, TERMINAL },
	{ MOD, XK_m, spawn_process, MENU },
	{ MOD, XK_l, move_resize_window, MOVE_EAST },
	{ MOD, XK_k, move_resize_window, MOVE_NORTH },
	{ MOD, XK_j, move_resize_window, MOVE_SOUTH },
	{ MOD, XK_h, move_resize_window, MOVE_WEST },
	{ MOD | SHIFT, XK_l, move_resize_window, RESIZE_EAST },
	{ MOD | SHIFT, XK_k, move_resize_window, RESIZE_NORTH },
	{ MOD | SHIFT, XK_j, move_resize_window, RESIZE_SOUTH },
	{ MOD | SHIFT, XK_h, move_resize_window, RESIZE_WEST },
	{ MOD, XK_d, snap_window, SNAP_QUARTER_EAST },
	{ MOD, XK_w, snap_window, SNAP_QUARTER_NORTH },
	{ MOD, XK_s, snap_window, SNAP_QUARTER_SOUTH },
	{ MOD, XK_a, snap_window, SNAP_QUARTER_WEST },
	{ MOD | SHIFT, XK_d, snap_window, SNAP_HALF_EAST },
	{ MOD | SHIFT, XK_w, snap_window, SNAP_HALF_NORTH },
	{ MOD | SHIFT, XK_s, snap_window, SNAP_HALF_SOUTH },
	{ MOD | SHIFT, XK_a, snap_window, SNAP_HALF_WEST },
	{ MOD, XK_f, fullscreen_window, 0 },
	{ MOD, XK_c, center_window, 0 },
	{ MOD | SHIFT, XK_q, kill_window, 0 },
};

static const EventHandler event_handler[LASTEvent] = {
	[ButtonPress] = handle_button_press,
	[ConfigureRequest] = handle_configure_request,
	[KeyPress] = handle_key_press,
	[MappingNotify] = handle_mapping_notify,
	[MapRequest] = handle_map_request,
	[UnmapNotify] = handle_unmap_notify,
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

/*
 * Create a focus fallback dummy and hide the bastard,
 * we just use it to prevent focusing the root window,
 * which I've been told is bad.
 */
void create_dummy(void)
{
	XSetWindowAttributes attributes;

	dummy = XCreateSimpleWindow(display, root, -2, -2, 1, 1, 0, 0, 0);
	attributes.override_redirect = True;

	XChangeWindowAttributes(display, dummy, CWOverrideRedirect, &attributes);
	XMapWindow(display, dummy);

	current = dummy;
}

int current_not_valid(void)
{
	if (!current || current == dummy || current == root)
		return 1;

	return 0;
}

/* Just ignore all errors. */
int error_handler(Display *display, XErrorEvent *event)
{
	(void)display;
	(void)event;

	return 0;
}

void focus_current(void)
{
	/* Just to be paranoidly safe. */
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

	XWindowAttributes attributes;

	if (current_not_valid())
		return;

	if (!XGetWindowAttributes(display, current, &attributes))
		return;

	/*
	 * Since we don't compose any sort of Clients, and just
	 * deal with Windows directly, our only way to detect if
	 * a window's already fullscreen is to check if it's the
	 * same size as our root window, and naively assume that
	 * if this is the case, the window's fullscreen.
	 *
	 * And if it is, we just center it and give it a size of
	 * half the screen width and height.
	 *
	 * Annoying to some; a sane solution to me.
	 */
	if (attributes.height == screen_height && attributes.width == screen_width)
		reset_window();
	else
		XMoveResizeWindow(display, current, 0, 0, screen_width, screen_height);

}

void grab_input(void)
{
	size_t i;

	XUngrabKey(display, AnyKey, AnyModifier, root);
	XUngrabButton(display, AnyButton, AnyModifier, root);

	for (i = 0; i < sizeof(keys) / sizeof(struct Key); i++)
		XGrabKey(display, XKeysymToKeycode(display, keys[i].keysym),
			keys[i].modifier, root, True, GrabModeAsync, GrabModeAsync);

	XGrabButton(display, 1, MOD, root, True, ButtonPressMask, GrabModeAsync,
		GrabModeAsync, None, None);
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

void handle_configure_request(XEvent *event)
{
	XWindowChanges changes;
	XConfigureRequestEvent *request;

	request = &event->xconfigurerequest;

	changes.x = request->x;
	changes.y = request->y;
	changes.width = request->width;
	changes.height = request->height;
	changes.sibling = request->above;
	changes.stack_mode = request->detail;

	XConfigureWindow(display, request->window, request->value_mask, &changes);
}

void handle_key_press(XEvent *event)
{
	size_t i;
	KeySym keysym;
	XKeyEvent *xkey;

	xkey = &event->xkey;
	keysym = XkbKeycodeToKeysym(display, xkey->keycode, 0, 0);

	if (!keysym)
		return;

	/*
	 * The common clean mask approach, along the lines of
	 * `mask & ~(0|LockMask) & (ShiftMask|ControlMask|Mod1Mask...)`
	 * does not _seem_ to be necessary if we just compare modifier
	 * to state directly?
	 *
	 * But I could be wrong (and probably am), although I haven't
	 * ran into any issues by doig it this way so far...
	 *
	 * Of course, capslock and numlock etc will break our key
	 * bindings, but at the price of less code I can easily live
	 * with that.
	 */
	for (i = 0; i < sizeof(keys) / sizeof(struct Key); i++)
		if (keysym == keys[i].keysym && keys[i].modifier == xkey->state)
			keys[i].function(keys[i].command);
}

void handle_mapping_notify(XEvent *event)
{
	XMappingEvent *mapping;

	mapping = &event->xmapping;

	XRefreshKeyboardMapping(mapping);
	grab_input();
}

void handle_map_request(XEvent *event)
{
	Window window;
	XWindowAttributes attributes;

	window = event->xmaprequest.window;

	if (!XGetWindowAttributes(display, window, &attributes))
		return;

	if (attributes.override_redirect)
		return;

	XSelectInput(display, window, StructureNotifyMask);
	XMapWindow(display, window);

	current = window;

	if (attributes.height == 0 || attributes.width == 0)
		reset_window();
	else if (attributes.x + attributes.y == 0)
		center_window(0);

	focus_current();
}

void handle_unmap_notify(XEvent *event)
{
	(void)event;

	if (!current || current == root) {
		current = dummy;
		focus_current();
	}
}

void kill_window(char *command)
{
	(void)command;

	if (current_not_valid())
		return;

	XKillClient(display, current);

	current = dummy;
	focus_current();
}

void loop_events(void)
{
	XEvent event;

	while (1 && !XNextEvent(display, &event)) {
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

	/*
	 * This "direction" and "move" approach is very ugly,
	 * but it does get the job done.
	 * */
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

void reset_window()
{
	if (current_not_valid())
		return;

	XMoveResizeWindow(display, current,
		screen_width / 4, screen_height / 4,
		screen_width / 2, screen_height / 2);
}

void snap_window(char *command)
{
	char direction;
	int half, height, width, x, y;
	XWindowAttributes attributes;

	if (current_not_valid())
		return;

	if (!XGetWindowAttributes(display, current, &attributes))
		return;

	/*
	 * Ugly approach, same as for move_resize_window,
	 * but still gets the job done.
	 * */
	direction = command[1];
	half = command[0] == 'h';
	height = attributes.height;
	width = attributes.width;
	x = attributes.x;
	y = attributes.y;

	switch (direction) {
		case 'n':
			height = screen_height / 2;
			width = half ? screen_width : (screen_width / 2);
			y = 0;
			x = 0;
			break;
		case 'e':
			height = half ? screen_height : (screen_height / 2);
			width = screen_width / 2;
			y = 0;
			x = screen_width / 2;
			break;
		case 's':
			height = screen_height / 2;
			width = half ? screen_width : (screen_width / 2);
			y = screen_height / 2;
			x = half ? 0 : (screen_width / 2);
			break;
		case 'w':
			height = half ? screen_height : (screen_height / 2);
			width = screen_width / 2;
			y = half ? 0 : (screen_height / 2);
			x = 0;
			break;
	}

	XMoveResizeWindow(display, current, x, y, width, height);
}

void init_wm(void)
{
	display = XOpenDisplay(0);

	if (!display)
		exit(1);

	/* Ignore SIGCHLD, hopefully prevent zombie processes. */
	signal(SIGCHLD, SIG_IGN);

	root = DefaultRootWindow(display);
	screen = DefaultScreen(display);
	screen_width = XDisplayWidth(display, screen);
	screen_height = XDisplayHeight(display, screen);

	XSetErrorHandler(error_handler);
	XSelectInput(display, root, SubstructureRedirectMask);

	create_dummy();
}

/* Double fork to avoid zombie processes, the way i3WM does it. */
void spawn_process(char *command)
{
	if (fork() == 0) {
		if (fork() == 0) {
			if (display)
				close(ConnectionNumber(display));

			setsid();
			execl("/bin/sh", "sh", "-c", command, 0);

			exit(1);
		}
		else {
			exit(0);
		}
	}
}

/*
 * We exit the WM by killing the process, leaving it up to
 * Xorg to do all the necessary cleaning up for us.
 *
 * Let's keep things nice and tidy by separating concerns,
 * leaving main deliberately close to empty.
 */
int main(void)
{
	init_wm();
	grab_input();
	loop_events();
}
