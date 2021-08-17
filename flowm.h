#include <X11/keysym.h>
#include <X11/XKBlib.h>

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
typedef struct KeyBinding KeyBinding;

/* Very similar to the way DWM, SOWM and many others do it */
struct KeyBinding {
	unsigned int modifier;
	KeySym key_sym;
	void (*function)(char *command);
	char *command;
};

void center_window(char *command);
int current_not_valid(void);
int error_handler(Display *display, XErrorEvent *ev);
void focus_current(void);
void fullscreen_window(char *command);
void grab_input(void);
void handle_button_press(XEvent *event);
void handle_destroy_notify(XEvent *event);
void handle_key_press(XEvent *event);
void handle_map_request(XEvent *event);
void kill_window(char *command);
void loop_events(void);
void move_resize_window(char *command);
void snap_window(char *command);
void start_wm(void);
void stop_wm(void);
void spawn_process(char *command);
void quit(char *command);
