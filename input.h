#ifndef INPUT_H
#define INPUT_H

#include "controller.h"

// Returns 0 on success, nonzero on failure
int  input_init(void);

// Poll events + update controller buttons from keyboard
// Returns 1 if user requested quit (Esc or window close), else 0
int  input_update(Controller* c);

// Cleanup
void input_shutdown(void);

#endif