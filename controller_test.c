#include <stdio.h>
#include <stdint.h>
#include "controller.h"

static int fail(const char* msg) {
    printf("FAIL: %s\n", msg);
    return 1;
}

int main(void) {
    Controller c;
    controller_init(&c);

    // Press A + Start + Right
    controller_set_button(&c, BTN_A, 1);
    controller_set_button(&c, BTN_START, 1);
    controller_set_button(&c, BTN_RIGHT, 1);

    // Latch: write 1 then 0
    controller_write(&c, 1);
    controller_write(&c, 0);

    // Read 8 bits
    uint8_t bits[8];
    for (int i = 0; i < 8; i++) {
        bits[i] = controller_read(&c) & 1;
    }

    // Expected: A B Sel Start Up Down Left Right
    // A=1, B=0, Sel=0, Start=1, Up=0, Down=0, Left=0, Right=1
    uint8_t expected[8] = {1,0,0,1,0,0,0,1};

    for (int i = 0; i < 8; i++) {
        if (bits[i] != expected[i]) {
            printf("Got:      ");
            for (int j = 0; j < 8; j++) printf("%d", bits[j]);
            printf("\nExpected: ");
            for (int j = 0; j < 8; j++) printf("%d", expected[j]);
            printf("\n");
            return fail("Shifted bits did not match expected order.");
        }
    }

    // --- Test strobe=1 behavior: always return A ---
    // Turn strobe on (continuous latch)
    controller_write(&c, 1);

    // A is pressed, so reads should always return 1
    for (int i = 0; i < 5; i++) {
        uint8_t v = controller_read(&c) & 1;
        if (v != 1) return fail("With strobe=1, read() should always return A bit.");
    }

    // Release A while strobe is still 1, should immediately read 0
    controller_set_button(&c, BTN_A, 0);
    uint8_t v = controller_read(&c) & 1;
    if (v != 0) return fail("With strobe=1, after releasing A, read() should return 0.");

    // --- Optional: after 8 reads (strobe=0), return 1s ---
    // Only keep this if your controller_read() sets shift_reg |= 0x80
    controller_write(&c, 0); // strobe off
    controller_write(&c, 1); // latch current
    controller_write(&c, 0); // latch on 1->0

    // Consume 8 reads
    for (int i = 0; i < 8; i++) (void)controller_read(&c);

    // Next few reads should be 1 (common behavior)
    for (int i = 0; i < 3; i++) {
        uint8_t r = controller_read(&c) & 1;
        if (r != 1) {
            printf("Note: post-8-reads behavior differs (not necessarily fatal).\n");
            break;
        }
    }

    printf("PASS: controller tests\n");
    return 0;
}