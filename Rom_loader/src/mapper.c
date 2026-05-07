// mapper.c
#include "mapper.h"
#include <stdlib.h>

Mapper *mapper0_create(void);
Mapper *mapper1_create(void);
Mapper *mapper2_create(void);
Mapper *mapper3_create(void);
Mapper *mapper9_create(void);

Mapper *mapper_create(uint8_t mapper_id) {
    switch (mapper_id) {
        case 0: return mapper0_create();
        case 1: return mapper1_create();
        case 2: return mapper2_create();
        case 3: return mapper3_create();
        case 9: return mapper9_create();
        default: return NULL;
    }
}
void mapper_destroy(Mapper *m) {
    if (!m) return;
    if (m->destroy) m->destroy(m);
    free(m);
}
