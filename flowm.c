#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

typedef struct Key Key;
typedef void (*Events)(XEvent *event);

struct Key {
	unsigned int modifier;
	KeySym keysym;
	void (*function)(XEvent *event, char *command);
	char *command;
};

static void init(void);
static void grab(void);
static void loop(void);

static int ignore(Display *display, XErrorEvent *event);

static void enter(XEvent *event);
static void configure(XEvent *event);
static void key(XEvent *event);
static void map(XEvent *event);
static void mapping(XEvent *event);
static void unmap(XEvent *event);

static void next(XEvent *event, char *command);
static void prev(XEvent *event, char *command);
static void launch(XEvent *event, char *command);
static void destroy(XEvent *event, char *command);

static Display *display;
static Window root;
static int screen, width, height;

static Key keys[] = {
	{ Mod4Mask, XK_Return, launch, "xterm" },
	{ Mod4Mask, XK_d, launch, "dmn" },
	{ Mod4Mask, XK_b, launch, "chromium" },
	{ Mod4Mask, XK_p, launch, "scr" },
	{ Mod4Mask | ShiftMask, XK_q, destroy, 0 },
	{ Mod4Mask, XK_Tab, next, 0 },
	{ Mod4Mask | ShiftMask, XK_Tab, prev, 0 },
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
	[MappingNotify] = mapping,
	[UnmapNotify] = unmap,
};

void init(void)
{
	if (!(display = XOpenDisplay(0)))
		exit(1);

	XSetErrorHandler(ignore);

	signal(SIGCHLD, SIG_IGN);

	root = DefaultRootWindow(display);
	screen = DefaultScreen(display);
	width = XDisplayWidth(display, screen);
	height = XDisplayHeight(display, screen);

	XSelectInput(display, root, SubstructureRedirectMask);
}

void grab(void)
{
	size_t i;

	XUngrabKey(display, AnyKey, AnyModifier, root);

	for (i = 0; i < sizeof(keys) / sizeof(struct Key); i++)
		XGrabKey(display, XKeysymToKeycode(display, keys[i].keysym),
			keys[i].modifier, root, True, GrabModeAsync, GrabModeAsync);
}

void loop(void)
{
	XEvent event;

	while (1 && !XNextEvent(display, &event)) {
		if (events[event.type])
			events[event.type](&event);
	}
}

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
	XWindowChanges changes;
	XConfigureRequestEvent *request = &event->xconfigurerequest;

	changes.x = request->x;
	changes.y = request->y;
	changes.width = request->width;
	changes.height = request->height;
	changes.border_width = request->border_width;
	changes.sibling = request->above;
	changes.stack_mode = request->detail;

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
	XWindowAttributes attributes;
	Window window = event->xmaprequest.window;
	XWindowChanges changes = { .border_width = 0 };

	if (!XGetWindowAttributes(display, window, &attributes))
		return;

	if (attributes.override_redirect)
		return;

	XSelectInput(display, window, StructureNotifyMask | EnterWindowMask);
    XConfigureWindow(display, window, CWBorderWidth, &changes);
	XMoveResizeWindow(display, window, 0, 0, width, height);
	XMapWindow(display, window);
}

void mapping(XEvent *event)
{
	XRefreshKeyboardMapping(&event->xmapping);

	if (event->xmapping.request == MappingKeyboard)
		grab();
}

void unmap(XEvent *event)
{
	XUnmapWindow(display, event->xunmap.window);
}

void next(XEvent *event, char *command)
{
	(void)event;
	(void)command;

	XCirculateSubwindowsUp(display, root);
}

void prev(XEvent *event, char *command)
{
	(void)event;
	(void)command;

	XCirculateSubwindowsDown(display, root);
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
	init();
	grab();
	loop();
}
