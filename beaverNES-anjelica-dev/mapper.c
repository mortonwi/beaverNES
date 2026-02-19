// mapper.c
#include "mapper.h"
#include <stdlib.h>

// Factories implemented in mapper_0.c / mapper_2.c
Mapper *mapper0_create(void);
Mapper *mapper2_create(void);

Mapper *mapper_create(uint8_t mapper_id) {
    switch (mapper_id) {
        case 0: return mapper0_create();
        case 2: return mapper2_create();
        default: return NULL;
    }
}
// Default destroy just frees the Mapper struct, but some mappers may have additional state to free.
void mapper_destroy(Mapper *m) {
    if (!m) return;
    if (m->destroy) m->destroy(m);
    free(m);
}
