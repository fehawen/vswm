#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#define BORDER_WIDTH 3
#define BORDER_COLOR 0x24292e
#define GAP_TOP 30
#define GAP_RIGHT 30
#define GAP_BOTTOM 80
#define GAP_LEFT 30

typedef struct Key Key;
typedef void (*Events)(XEvent *event);

struct Key {
	unsigned int modifier;
	KeySym keysym;
	void (*function)(XEvent *event, char *command);
	char *command;
};

static void enter(XEvent *event);
static void configure(XEvent *event);
static void key(XEvent *event);
static void map(XEvent *event);

static void focus(XEvent *event, char *command);
static void launch(XEvent *event, char *command);
static void destroy(XEvent *event, char *command);

static int ignore(Display *display, XErrorEvent *event);

static Display *display;
static Window root;
static int screen, width, height;

static Key keys[] = {
	{ Mod4Mask, XK_Return, launch, "xterm" },
	{ Mod4Mask, XK_d, launch, "dmn" },
	{ Mod4Mask, XK_b, launch, "chromium" },
	{ Mod4Mask, XK_p, launch, "scr" },
	{ Mod4Mask | ShiftMask, XK_q, destroy, 0 },
	{ Mod4Mask, XK_Tab, focus, "next" },
	{ Mod4Mask | ShiftMask, XK_Tab, focus, "prev" },
	{ 0, XF86XK_AudioMute, launch, "pamixer -t" },
	{ 0, XF86XK_AudioLowerVolume, launch, "pamixer -d 5" },
	{ 0, XF86XK_AudioRaiseVolume, launch, "pamixer -i 5" },
	{ 0, XF86XK_MonBrightnessDown, launch, "xbacklight -dec 5" },
	{ 0, XF86XK_MonBrightnessUp, launch, "xbacklight -inc 5" },
};

static const Events events[LASTEvent] = {
	[ConfigureRequest] = configure,
	[EnterNotify] = enter,
	[KeyPress] = key,
	[MapRequest] = map,
};

int ignore(Display *display, XErrorEvent *event)
{
	(void)display;
	(void)event;

	return 0;
}

void enter(XEvent *event) {
	Window window = event->xcrossing.window;

	XSetInputFocus(display, window, RevertToParent, CurrentTime);
	XRaiseWindow(display, window);
}

void configure(XEvent *event)
{
	XConfigureRequestEvent *request = &event->xconfigurerequest;
	XWindowChanges changes = {
		.x = request->x,
		.y = request->y,
		.width = request->width,
		.height = request->height,
		.border_width = request->border_width,
		.sibling = request->above,
		.stack_mode = request->detail,
	};

	XConfigureWindow(display, request->window, request->value_mask, &changes);
}

void key(XEvent *event)
{
	size_t i;
	KeySym keysym = XkbKeycodeToKeysym(display, event->xkey.keycode, 0, 0);

	for (i = 0; i < sizeof(keys) / sizeof(struct Key); i++)
		if (keysym == keys[i].keysym && keys[i].modifier == event->xkey.state)
			keys[i].function(event, keys[i].command);
}

void map(XEvent *event)
{
	Window window = event->xmaprequest.window;
	XWindowChanges changes = { .border_width = BORDER_WIDTH };

	XSelectInput(display, window, StructureNotifyMask | EnterWindowMask);
	XSetWindowBorder(display, window, BORDER_COLOR);
	XConfigureWindow(display, window, CWBorderWidth, &changes);
	XMoveResizeWindow(display, window, GAP_TOP, GAP_LEFT, width, height);
	XMapWindow(display, window);
}

void focus(XEvent *event, char *command)
{
	(void)event;
	int next = command[0] == 'n';

	XCirculateSubwindows(display, root, next ? RaiseLowest : LowerHighest);
}

void launch(XEvent *event, char *command)
{
	(void)event;

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

void destroy(XEvent *event, char *command)
{
	(void)command;

	XSetCloseDownMode(display, DestroyAll);
	XKillClient(display, event->xkey.subwindow);
}

int main(void)
{
	size_t i;
	XEvent event;

	if (!(display = XOpenDisplay(0)))
		exit(1);

	XSetErrorHandler(ignore);

	signal(SIGCHLD, SIG_IGN);

	root = DefaultRootWindow(display);
	screen = DefaultScreen(display);

	width = XDisplayWidth(display, screen) - \
		(GAP_RIGHT + GAP_LEFT + (BORDER_WIDTH * 2));

	height = XDisplayHeight(display, screen) - \
		(GAP_TOP + GAP_BOTTOM + (BORDER_WIDTH * 2));

	XSelectInput(display, root, SubstructureRedirectMask);
	XDefineCursor(display, root, XCreateFontCursor(display, 68));

	for (i = 0; i < sizeof(keys) / sizeof(struct Key); i++)
		XGrabKey(display, XKeysymToKeycode(display, keys[i].keysym),
			keys[i].modifier, root, True, GrabModeAsync, GrabModeAsync);

	while (1 && !XNextEvent(display, &event)) {
		if (events[event.type])
			events[event.type](&event);
	}
}
