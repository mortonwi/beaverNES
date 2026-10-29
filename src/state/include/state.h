#ifndef STATE_H
#define STATE_H

#include <stdbool.h>
#include "bus.h"

bool save_state(const char *path, Bus *bus);
bool load_state(const char *path, Bus *bus);

#endif