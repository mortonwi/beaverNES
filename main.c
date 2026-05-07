#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

// Nuklear and Tinyfiledialogs
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION

#include "nuklear.h"
#include "nuklear_sdl_renderer.h"
#include "tinyfiledialogs.h"

// Emulator components
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "rom_loader.h"
#include "ppu.h"
#include "opcodes.h"
#include "apu.h"
#include "controller.h"

#define SCALE 2.5
#define MENU_BAR_HEIGHT 26

#define AUDIO_BUFFER_SAMPLES 1024
#define SAMPLE_RATE 48000
#define CPU_HZ 1789773.0f

/*  
 * Emulator Utility Functions
 */

// helper to find controller inputs
// Credit goes to rubenwardy 
// https://blog.rubenwardy.com/2023/01/24/using_sdl_gamecontroller/
SDL_GameController *findController() {
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            return SDL_GameControllerOpen(i);
        }
    }

    return NULL;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s rom.nes\n", argv[0]);
        printf("No ROM provided. Starting with empty screen\n");
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "beaverNES",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        PPU_WIDTH * SCALE,
        PPU_HEIGHT * SCALE + MENU_BAR_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // VSYNC helps keep video pacing sane without manual delays
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STREAMING,
        PPU_WIDTH,
        PPU_HEIGHT
    );
    if (!texture) {
        printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // --- Nuklear init ---
    struct nk_context *ctx = nk_sdl_init(window, renderer);

    /* Dark theme that looks good over an NES screen */
    struct nk_color bg_dark  = nk_rgba(30,  30,  30,  255);
    struct nk_color bg_mid   = nk_rgba(45,  45,  45,  255);
    struct nk_color bg_hover = nk_rgba(58,  111, 216, 255);
    struct nk_color text_col = nk_rgba(220, 220, 220, 255);
    struct nk_color border   = nk_rgba(80,  80,  80,  255);
 
    struct nk_style *s = &ctx->style;
    s->window.fixed_background    = nk_style_item_color(bg_dark);
    s->window.border_color        = border;
    s->window.border              = 1.0f;
 
    s->menu_button.normal         = nk_style_item_color(bg_dark);
    s->menu_button.hover          = nk_style_item_color(bg_hover);
    s->menu_button.active         = nk_style_item_color(bg_hover);
    s->menu_button.text_normal    = text_col;
    s->menu_button.text_hover     = nk_rgba(255,255,255,255);
    s->menu_button.text_active    = nk_rgba(255,255,255,255);
    s->menu_button.border         = 0.0f;
 
    s->contextual_button.normal   = nk_style_item_color(bg_mid);
    s->contextual_button.hover    = nk_style_item_color(bg_hover);
    s->contextual_button.active   = nk_style_item_color(bg_hover);
    s->contextual_button.text_normal  = text_col;
    s->contextual_button.text_hover   = nk_rgba(255,255,255,255);
    s->contextual_button.text_active  = nk_rgba(255,255,255,255);
    s->contextual_button.border   = 0.0f;
 
    struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();

    // --- Audio (queued) ---
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq     = SAMPLE_RATE; // request 48k
    want.format   = AUDIO_F32SYS;
    want.channels = 1;
    want.samples  = AUDIO_BUFFER_SAMPLES;
    want.callback = NULL;

    SDL_AudioDeviceID device =
        SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (!device) {
        printf("Failed to open audio device: %s\n", SDL_GetError());
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    printf("Audio device opened at %d Hz\n", have.freq);
    SDL_PauseAudioDevice(device, 0);

    // --- Emulator Init ---
    Memory *memory = memory_create();
    APU *apu = apu_create();
    Bus *bus = bus_create(memory, apu);
    CPU *cpu = cpu_create(bus);

    // Controller 1
    Controller pad1;
    controller_init(&pad1);

    Cartridge cart;
    char err[256];
    bool rom_loaded = false;

    // check rom loading through the command line
    if (argc >= 2) {
        if (rom_load(argv[1], &cart, err, sizeof(err))) {
            printf("PRG size: %u\n", (unsigned)cart.prg_size);
            printf("CHR size: %u\n", (unsigned)cart.chr_size);
            bus->rom = &cart;
            ppu_init();
            ppu_connect_cartridge(&cart);
            init_opcode_table();
            cpu_reset(cpu);
            rom_loaded = true;
        } else {
            printf("Command line ROM load failed: %s\n", err);
        }
    }

    // cycles per audio sample (~37.287 at 48k)
    const double CPU_PER_SAMPLE = (double)CPU_HZ / (double)have.freq;
    double cpu_sample_frac = 0.0;

    float audio_buffer[AUDIO_BUFFER_SAMPLES];

    // Keep ~80ms queued to avoid huge latency
    const Uint32 TARGET_QUEUED_BYTES = (Uint32)(have.freq * (int)sizeof(float) * 0.08f);

    int pending_cpu_cycles = 0;
    bool running = true;
    SDL_Event event;

    // Position the emulator window below the menu
    SDL_Rect nes_rect = {
        0,
        MENU_BAR_HEIGHT,
        PPU_WIDTH  * SCALE,
        PPU_HEIGHT * SCALE
    };

    // To pause emulation
    bool paused = false;
    
    // Video/Audio menu stuff
    bool show_av_settings = false;
    float volume = 0.5f;

    // Controller menu stuff
    bool show_control_settings = false;
    int waiting_for_key = -1;

    SDL_Scancode key_A = SDL_SCANCODE_X;
    SDL_Scancode key_B = SDL_SCANCODE_Z;
    SDL_Scancode key_SELECT = SDL_SCANCODE_RSHIFT;
    SDL_Scancode key_START = SDL_SCANCODE_RETURN;
    SDL_Scancode key_UP = SDL_SCANCODE_UP;
    SDL_Scancode key_DOWN = SDL_SCANCODE_DOWN;
    SDL_Scancode key_LEFT = SDL_SCANCODE_LEFT;
    SDL_Scancode key_RIGHT = SDL_SCANCODE_RIGHT;

    // Main Emulator Loop
    while (running)
    {
        nk_input_begin(ctx);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;

            // --- Pause/Resume Hotkey ---
            if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {

                if (waiting_for_key != -1) {
                SDL_Scancode pressed = event.key.keysym.scancode;

                if (waiting_for_key == BTN_A) key_A = pressed;
                else if (waiting_for_key == BTN_B) key_B = pressed;
                else if (waiting_for_key == BTN_SELECT) key_SELECT = pressed;
                else if (waiting_for_key == BTN_START) key_START = pressed;
                else if (waiting_for_key == BTN_UP) key_UP = pressed;
                else if (waiting_for_key == BTN_DOWN) key_DOWN = pressed;
                else if (waiting_for_key == BTN_LEFT) key_LEFT = pressed;
                else if (waiting_for_key == BTN_RIGHT) key_RIGHT = pressed;

                waiting_for_key = -1;
            continue;
}           
                if (event.key.keysym.scancode == SDL_SCANCODE_P) {
                    paused = !paused;

                    if (paused)
                        SDL_PauseAudioDevice(device, 1);
                    else
                        SDL_PauseAudioDevice(device, 0);
                }
            }

            nk_sdl_handle_event(&event);
        }
        nk_input_end(ctx);

        // --- Controller input (keyboard → NES controller) ---
        uint8_t buttons = 0;

        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        if (keys[key_A])      buttons |= (1u << BTN_A);
        if (keys[key_B])      buttons |= (1u << BTN_B);
        if (keys[key_SELECT]) buttons |= (1u << BTN_SELECT);
        if (keys[key_START])  buttons |= (1u << BTN_START);

        if (keys[key_UP])     buttons |= (1u << BTN_UP);
        if (keys[key_DOWN])   buttons |= (1u << BTN_DOWN);
        if (keys[key_LEFT])   buttons |= (1u << BTN_LEFT);
        if (keys[key_RIGHT])  buttons |= (1u << BTN_RIGHT);

        SDL_GameController *controller = findController();

        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A))           buttons |= (1u << BTN_A);
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B))           buttons |= (1u << BTN_B);
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK))        buttons |= (1u << BTN_SELECT);
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START))       buttons |= (1u << BTN_START);

        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))     buttons |= (1u << BTN_UP);
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))   buttons |= (1u << BTN_DOWN);
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))   buttons |= (1u << BTN_LEFT);
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))  buttons |= (1u << BTN_RIGHT);

        controller_set_state(&bus->pad1, buttons);

        // --- Nuklear UI input ---
        if (nk_begin(ctx, "MenuBar",
            nk_rect(0, 0, PPU_WIDTH * SCALE, MENU_BAR_HEIGHT),
            NK_WINDOW_NO_SCROLLBAR))
        {
            nk_menubar_begin(ctx);
            nk_layout_row_static(ctx, 20, 60, 5);   // last parameter controls number of elements in the menu

            if(nk_menu_begin_label(ctx, "Options", NK_TEXT_CENTERED, nk_vec2(120, 120))) {
                nk_layout_row_dynamic(ctx, 25, 1);

                if (nk_menu_item_label(ctx, "Audio", NK_TEXT_LEFT)) {
                    show_av_settings = true;
                }

                if (nk_menu_item_label(ctx, "Video", NK_TEXT_LEFT)) {
                    show_av_settings = true;
                }

                if (nk_menu_item_label(ctx, "Quit", NK_TEXT_LEFT)) {
                    running = false;
                }

                nk_menu_end(ctx);
            }

            if (nk_menu_begin_label(ctx, "File", NK_TEXT_CENTERED, nk_vec2(120, 120))) {
                nk_layout_row_dynamic(ctx, 25, 1);

                if (nk_menu_item_label(ctx, "Open ROM", NK_TEXT_LEFT)) {                    
                    const char *filterpatterns[] = { "*.nes" };
                    const char *ROMpath = tinyfd_openFileDialog(
                        "Open NES ROM",     // dialog title
                        "",                 // default path
                        1,                  // number of filters
                        filterpatterns,     // .nes file filter
                        ".nes",             // filter description
                        0                   // allow multiple selection
                    );

                    // if user cancelled — do nothing
                    if (!ROMpath) {
                        // Keep paused state exactly as it was and just return to the menu
                        goto skip_rom_load;
                    }

                    // free previously loaded rom
                    if (rom_loaded) rom_free(&cart);

                    // load new rom
                    if (rom_load(ROMpath, &cart, err, sizeof(err))) {
                        printf("PRG size: %u\n", (unsigned)cart.prg_size);
                        printf("CHR size: %u\n", (unsigned)cart.chr_size);
                        bus->rom = &cart;
                        ppu_init();
                        ppu_connect_cartridge(&cart);
                        init_opcode_table();
                        cpu_reset(cpu);
                        rom_loaded = true;
                    } else {
                        // error messagebox
                        SDL_ShowSimpleMessageBox(
                            SDL_MESSAGEBOX_ERROR,
                            " File Error",
                            "Unable to load target ROM file",
                            NULL
                        );
                    }

                    skip_rom_load: ;
                }

                nk_menu_end(ctx);
            }

            if(nk_menu_begin_label(ctx, "Controls", NK_TEXT_CENTERED, nk_vec2(120,120))) {
                show_control_settings = true;
                nk_menu_end(ctx);
            }

            if(nk_menu_begin_label(ctx, "About", NK_TEXT_CENTERED, nk_vec2(120, 120))) {
                nk_layout_row_dynamic(ctx, 25, 1);

                if (nk_menu_item_label(ctx, "Wiki", NK_TEXT_LEFT)) {
                    SDL_OpenURL("https://github.com/mortonwi/beaverNES/wiki");
                }

                nk_menu_end(ctx);
            }

            // Pause/Resume standalone button
            if (nk_button_label(ctx, paused ? "Resume" : "Pause")) {
                paused = !paused;

                if (paused)
                    SDL_PauseAudioDevice(device, 1);
                else
                    SDL_PauseAudioDevice(device, 0);
            }

            nk_menubar_end(ctx);
        }
        nk_end(ctx);

        // Audio video settings menu
        if (show_av_settings) {
            if (nk_begin(ctx, "Audio/Video Settings",
                nk_rect(200, 100, 400, 300),
                NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE))
            {
                nk_layout_row_begin(ctx, NK_DYNAMIC, 25, 2);
                {
                    nk_layout_row_push(ctx, 0.7f);
                    nk_label(ctx, "Master Volume", NK_TEXT_LEFT);

                    nk_layout_row_push(ctx, 0.3f);

                    char buf[32];
                    snprintf(buf, sizeof(buf), "%d%%", (int)(volume * 100.0f));
                    nk_label(ctx, buf, NK_TEXT_RIGHT);
                }
                nk_layout_row_end(ctx);

                nk_layout_row_dynamic(ctx, 25, 1);
                nk_slider_float(ctx, 0.0f, &volume, 1.0f, 0.01f);

                nk_layout_row_dynamic(ctx, 30, 1);
                if (nk_button_label(ctx, "Close")) {
                    show_av_settings = false;
                }
            }
            nk_end(ctx);
        }

        // Control settings menu
if (show_control_settings) {
    if (nk_begin(ctx, "Control Settings",
        nk_rect(200, 100, 400, 360),
        NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE))
    {
        nk_layout_row_dynamic(ctx, 25, 1);

        if (waiting_for_key != -1) {
            nk_label(ctx, "Press any key to bind...", NK_TEXT_CENTERED);
        } else {
            nk_label(ctx, "Click a button, then press a key.", NK_TEXT_CENTERED);
        }

        nk_layout_row_dynamic(ctx, 30, 2);

        nk_label(ctx, "A", NK_TEXT_LEFT);
        if (nk_button_label(ctx, SDL_GetScancodeName(key_A))) {
            waiting_for_key = BTN_A;
        }

        nk_label(ctx, "B", NK_TEXT_LEFT);
        if (nk_button_label(ctx, SDL_GetScancodeName(key_B))) {
            waiting_for_key = BTN_B;
        }

        nk_label(ctx, "Select", NK_TEXT_LEFT);
        if (nk_button_label(ctx, SDL_GetScancodeName(key_SELECT))) {
            waiting_for_key = BTN_SELECT;
        }

        nk_label(ctx, "Start", NK_TEXT_LEFT);
        if (nk_button_label(ctx, SDL_GetScancodeName(key_START))) {
            waiting_for_key = BTN_START;
        }

        nk_label(ctx, "Up", NK_TEXT_LEFT);
        if (nk_button_label(ctx, SDL_GetScancodeName(key_UP))) {
            waiting_for_key = BTN_UP;
        }

        nk_label(ctx, "Down", NK_TEXT_LEFT);
        if (nk_button_label(ctx, SDL_GetScancodeName(key_DOWN))) {
            waiting_for_key = BTN_DOWN;
        }

        nk_label(ctx, "Left", NK_TEXT_LEFT);
        if (nk_button_label(ctx, SDL_GetScancodeName(key_LEFT))) {
            waiting_for_key = BTN_LEFT;
        }

        nk_label(ctx, "Right", NK_TEXT_LEFT);
        if (nk_button_label(ctx, SDL_GetScancodeName(key_RIGHT))) {
            waiting_for_key = BTN_RIGHT;
        }

        nk_layout_row_dynamic(ctx, 30, 1);
        if (nk_button_label(ctx, "Close")) {
            show_control_settings = false;
        }
    }
    nk_end(ctx);
}

        // --- Emulation step + audio generation ---
        if (rom_loaded && !paused) {
            if (SDL_GetQueuedAudioSize(device) < TARGET_QUEUED_BYTES) {
                for (int i = 0; i < AUDIO_BUFFER_SAMPLES; i++) {
                    cpu_sample_frac += CPU_PER_SAMPLE;
                    int cycles_to_run = (int)cpu_sample_frac;
                    cpu_sample_frac -= (double)cycles_to_run;

                    float accum = 0.0f;
                    int accum_count = 0;

                    while (cycles_to_run > 0) {
                        if (pending_cpu_cycles == 0 && bus->dmc_stall_cycles == 0) {
                            pending_cpu_cycles = cpu_step(cpu);
                            if (pending_cpu_cycles < 1) pending_cpu_cycles = 1;
                        }

                        // PPU: 3 cycles per CPU cycle
                        for (int p = 0; p < 3; p++) {
                            ppu_clock();
                            if (ppu_poll_nmi())
                                cpu_nmi(cpu);
                        }

                        // APU: tick once per CPU cycle
                        apu_tick(apu);
                        bus_service_dmc_dma(bus);

                        // Consume one cycle — either a real CPU cycle or a stall cycle
                        if (bus->dmc_stall_cycles > 0) {
                            // stall cycle: bus_service_dmc_dma already decremented it
                        } else if (pending_cpu_cycles > 0) {
                            pending_cpu_cycles--;
                        }

                        // Accumulate current APU mix (0..1)
                        accum += apu_get_output(apu);
                        accum_count++;
                        cycles_to_run--;
                    }

                    float s = (accum_count > 0) ? (accum / (float)accum_count) : 0.0f;
                    audio_buffer[i] = s * volume;
                }

                SDL_QueueAudio(device, audio_buffer, sizeof(audio_buffer));
            }
        }

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
 
        // render the emulator output
        if (rom_loaded) {
            SDL_UpdateTexture(
                texture, NULL,
                ppu_get_framebuffer(),
                PPU_WIDTH * (int)sizeof(uint32_t)
            );
            SDL_RenderCopy(renderer, texture, NULL, &nes_rect);
        }
 
        nk_sdl_render(NK_ANTI_ALIASING_ON);
        SDL_RenderPresent(renderer);
    }
 
    nk_sdl_shutdown();
    SDL_CloseAudioDevice(device);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
 
    if (rom_loaded) rom_free(&cart);
    apu_free(apu);
    return 0;
}
