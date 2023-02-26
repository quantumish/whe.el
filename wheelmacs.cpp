#include <cstdint>
#include <thread>
#include <memory>
#include <vector>
#include <cstdio>
#include <cerrno>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

#include "linux/joystick.h"
#include "emacs-module.h"

extern "C" {
	#include "xdo.h"
}

#define VERSION 0.1
int plugin_is_GPL_compatible;

#define S(s) (env->intern(env, s)) 

static void bind_function (emacs_env *env, const char *name, emacs_value Sfun) {
	/* Set the function cell of the symbol named NAME to SFUN using
	   the 'fset' function.  */

	/* Convert the strings to symbols by interning them */
	emacs_value Qfset = env->intern (env, "fset");
	emacs_value Qsym = env->intern (env, name);

	/* Prepare the arguments array */
	emacs_value args[] = { Qsym, Sfun };

	/* Make the call (2 == nb of arguments) */
	env->funcall (env, Qfset, 2, args);
}

#define DEFUN(lsym, csym, amin, amax, doc, data)                        \
	bind_function (env, lsym, env->make_function (env, amin, amax, csym, doc, data))

static void provide (emacs_env *env, const char *feature) {
	emacs_value Qfeat = env->intern (env, feature);
	emacs_value Qprovide = env->intern (env, "provide");
	emacs_value args[] = { Qfeat };

	env->funcall (env, Qprovide, 1, args);
}


std::shared_ptr<std::vector<bool>> buttons;
std::shared_ptr<std::vector<int16_t>> axes;
std::thread worker;
std::atomic_bool stop_worker;

void grab_values() {
	xdo_t* x = xdo_new(NULL);	
	int fd = open("/dev/input/js0", O_RDONLY | O_NONBLOCK);
	while (true) {
		struct js_event e;
		int r = read(fd, &e, sizeof(e));
		if (e.type & JS_EVENT_BUTTON) {
			(*buttons)[e.number] = e.value ? true : false;
			char key[4] = {0};
			sprintf(key, "F%d", e.number+12);
			if (e.value) xdo_send_keysequence_window(x, CURRENTWINDOW, key, 0);
		} else if (e.type & JS_EVENT_AXIS) {
			(*axes)[e.number] = e.value;
		}
		if (stop_worker) return;
	}
}

static emacs_value wheel_init (emacs_env *env, ptrdiff_t nargs, emacs_value* args, void* data) noexcept {
	std::vector<bool> tmp_buttons =
		{{false,false,false,false,false,false,false,false,false,false,false,false}};
	std::vector<int16_t> tmp_axes = {{0,0,0,0,0}};

	buttons = std::make_unique<std::vector<bool>>(tmp_buttons);
	axes = std::make_unique<std::vector<int16_t>>(tmp_axes);
	
	stop_worker = false;
	worker = std::thread(grab_values);
	return S("nil");
}

static emacs_value wheel_stop (emacs_env *env, ptrdiff_t nargs, emacs_value* args, void* data) noexcept {	
	stop_worker = true;
	worker.join();
	return S("nil");
}

static emacs_value wheel_button (emacs_env *env, ptrdiff_t nargs, emacs_value* args, void* data) noexcept {
	auto index = env->extract_integer(env, args[0]);
	if (index >= (*buttons).size() || stop_worker) return S("nil");
	bool down = (*buttons)[index];
	return down ? S("t") : S("nil");
}

static emacs_value wheel_axis (emacs_env *env, ptrdiff_t nargs, emacs_value* args, void* data) noexcept {
	auto index = env->extract_integer(env, args[0]);
	if (index >= (*axes).size() || stop_worker) return S("nil");
	int16_t val = (*axes)[index];
	return env->make_integer(env, val);
}

extern int emacs_module_init (struct emacs_runtime *ert) noexcept {
	emacs_env *env = ert->get_environment (ert);
	DEFUN("wheel-init", wheel_init, 0, 0, "Set up whe.el worker thread", NULL);
	DEFUN("wheel-stop", wheel_stop, 0, 0, "Stop whe.el worker thread", NULL);
	DEFUN("wheel-button", wheel_button, 1, 1, "Check if button is pressed.", NULL);
	DEFUN("wheel-axis", wheel_axis, 1, 1, "Get axis value.", NULL);	
	provide(env, "wheel");
	return 0;
}
