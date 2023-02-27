#pragma once
#include <stdbool.h>

enum wheel_cmd_type {
    AUTOCENTER,
    AUTOCENTER_FORCE,
    GAIN,
    RANGE,
    NONE,
};

// tagged union
struct wheel_cmd {
    enum wheel_cmd_type type;
    union {
        bool autocenter;
        int autocenter_force;
        int gain;
        int range;
    } value; 
}; 


/* struct wheel_cmd a = { */
/*     .type = AUTOCENTER, */
/*     .value = { .autocenter = true } */
/* }; */
