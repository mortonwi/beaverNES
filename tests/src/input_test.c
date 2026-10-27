#include <stdio.h>
#include <stdint.h>

#include "controller.h"
#include "input.h"
#include <SDL2/SDL.h>

static void print_bits(uint8_t buttons) {
    printf("buttons: [A=%d B=%d Sel=%d Start=%d Up=%d Down=%d Left=%d Right=%d]  raw=0x%02X\n",
           (buttons >> BTN_A) & 1,
           (buttons >> BTN_B) & 1,
           (buttons >> BTN_SELECT) & 1,
           (buttons >> BTN_START) & 1,
           (buttons >> BTN_UP) & 1,
           (buttons >> BTN_DOWN) & 1,
           (buttons >> BTN_LEFT) & 1,
           (buttons >> BTN_RIGHT) & 1,
           buttons);
}

static void print_serial_read(Controller* c) {
    // Latch: write 1 then 0
    controller_write(c, 1);
    controller_write(c, 0);

    printf("serial: ");
    for (int i = 0; i < 8; i++) {
        uint8_t v = controller_read(c) & 1;
        printf("%d", v);
        if (i != 7) printf(" ");
    }
    printf("  (order: A B Sel Start Up Down Left Right)\n");
}

int main(void) {
    if (input_init() != 0) {
        printf("FAIL: SDL init\n");
        return 1;
    }

    Controller c;
    controller_init(&c);

    printf("Input test running.\n");
    printf("Keys: X=A, Z=B, RShift=Select, Enter=Start, Arrows=D-pad, Esc=Quit\n\n");

    uint8_t last_buttons = 0xFF;   // force first print

while (1) {

    if (input_update(&c)) break;

    if (c.buttons != last_buttons) {
        last_buttons = c.buttons;

        print_bits(c.buttons);
        print_serial_read(&c);
        printf("----\n");
    }

    SDL_Delay(16);   // ~60fps
}

    input_shutdown();
    printf("Done.\n");
    return 0;
}