// mapper.c
#include "mapper.h"
#include <stdlib.h>

// Forward-declared factory for Mapper 0
Mapper *mapper0_create(void);

Mapper *mapper_create(uint8_t mapper_id) {
    switch (mapper_id) {
        case 0:
            return mapper0_create();
        default:
            return NULL;
    }
}

void mapper_destroy(Mapper *m) {
    if (!m) return;
    if (m->destroy) {
        m->destroy(m);
    }
    free(m);
}
