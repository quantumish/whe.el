#include <cstdint>
#include <stdint.h>
#include <thread>
#include <memory>
#include <vector>
#include <cstdio>
#include <cerrno>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <mutex>

#include "linux/joystick.h"
#include "emacs-module.h"

extern "C" {
	#include "xdo.h"
    #include "wheelmesg.h"
}

#define VERSION 0.1
int plugin_is_GPL_compatible;

#define S(s) (env->intern(env, s)) 

// Set the function cell of the symbol named NAME to SFUN using the 'fset' function.
static void bind_function (emacs_env *env, const char *name, emacs_value Sfun) {
	emacs_value Qfset = env->intern (env, "fset");
	emacs_value Qsym = env->intern (env, name);
	emacs_value args[] = { Qsym, Sfun };
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
std::mutex cmd_mutex;
struct wheel_cmd cmd;

void grab_values() {
	xdo_t* x = xdo_new(NULL);	
	int fd = open("/dev/input/js0", O_RDONLY | O_NONBLOCK);

	int s = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in addr;
	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET; // Specify address family.
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY just 0.0.0.0, machine IP address
	addr.sin_port = htons(8070); // Specify port.

    connect(s, (struct sockaddr *)&addr, sizeof(addr));
	
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
		if (cmd.type != NONE) {
			sendto(s, (void*)&cmd, sizeof(struct wheel_cmd), 0, NULL, sizeof(addr));
			cmd.type = NONE;
			cmd_mutex.unlock();
		}
		if (stop_worker) return;
	}
}

static emacs_value wheel_init (emacs_env *env, ptrdiff_t nargs, emacs_value* args, void* data) noexcept {
	std::vector<bool> tmp_buttons(12);
	std::vector<int16_t> tmp_axes(5);
	std::fill(tmp_buttons.begin(), tmp_buttons.end(), false);
	std::fill(tmp_axes.begin(), tmp_axes.end(), 0);	
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

static emacs_value wheel_set_autocenter (emacs_env *env, ptrdiff_t nargs, emacs_value* args, void* data) noexcept {	
	bool val = env->is_not_nil(env, args[0]);
	// cmd_mutex.lock();
	cmd = {
		.type = AUTOCENTER,
		.value = {.autocenter = val}
	};
	// cmd_mutex.unlock();
	return S("nil");
}

static emacs_value wheel_set_range (emacs_env *env, ptrdiff_t nargs, emacs_value* args, void* data) noexcept {	
	int val = env->extract_integer(env, args[0]);
	if (val < 270 || val > 1080) return S("nil");
	cmd = {
		.type = RANGE,
		.value = {.range = val}
	};
	return S("nil");
}

static emacs_value wheel_set_gain (emacs_env *env, ptrdiff_t nargs, emacs_value* args, void* data) noexcept {	
	int val = env->extract_integer(env, args[0]);
	if (val < 0 || val > 100) return S("nil");
	cmd = {
		.type = GAIN,
		.value = {.gain = val}
	};
	return S("nil");
}

static emacs_value wheel_set_autocenter_force (emacs_env *env, ptrdiff_t nargs, emacs_value* args, void* data) noexcept {
	int val = env->extract_integer(env, args[0]);
	if (val < 0 || val > 100) return S("nil");
	cmd = {
		.type = AUTOCENTER_FORCE,
		.value = {.autocenter_force = val}
	};
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
	DEFUN("wheel-set-autocenter", wheel_set_autocenter, 1, 1, "Enable/disable autocentering.", NULL);
	DEFUN("wheel-set-autocenter-force", wheel_set_autocenter_force, 1, 1, "", NULL);
	DEFUN("wheel-set-range", wheel_set_range, 1, 1, "", NULL);
	DEFUN("wheel-set-gain", wheel_set_gain, 1, 1, "", NULL);
	DEFUN("wheel-axis", wheel_axis, 1, 1, "Get axis value.", NULL);	
	provide(env, "wheel");
	return 0;
}
