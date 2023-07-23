// gives the io streams, which has the ability to write to the terminal.
#include<iostream>

// includes the core header for us to implement in our frontend.
#include<core.hpp>

// SDL2 header for rendering and audio.
#include<SDL2/SDL.h>

// gives the std::mempcy function.
#include<cstring>

// gives the filestreams to read files from the host system.
#include<fstream>

// gives access to the std::vector type.
#include<vector>

/// the main entry point of the program and the frontend of the CHIP-8 emulator. 
int main(int argc, char* argv[]) {
    std::cout << "running chip8-c++-sdl" << std::endl;
    if (argc < 2) {
        std::cout << "expected rom path as argument." << std::endl;
        exit(-1);
        assert("unreachable" && false);
    } else if (argc > 2) {
        std::cout << "got multiple paths to ROM" << std::endl;
        exit(-1);
    }
    
    char* rom_path = argv[1];
    std::cout << "reading ROM from path: " << rom_path << std::endl;

    SDL_Init(SDL_INIT_EVERYTHING); // initialize all components of SDL2.
    
    std::ifstream ifstream(rom_path, std::ios::binary | std::ios::ate); // input file stream to read binary files, begins with the cursor at the end of the file.
    auto file_size = ifstream.tellg();              // gets the length of the file. because the cursor is at tne end we get the distance to the cursor, thus the length of the file.
    ifstream.seekg(std::ios::beg);                  // puts the cursor back to tghe begining of the file.
    auto byte_array = std::vector<char>();          // creates a vector of chars to store our read file in.
    byte_array.resize(file_size);                   // reserves enough space in the vector to store the file.
    ifstream.read(byte_array.data(), file_size);    // read the file and store it into the vector.
    ifstream.close();                               // closes the file as to not hoard resources.

    // create the core struct, which represents the backend of our emulator.    
    auto core = Core::create(byte_array.data(), byte_array.size());

    // the SDL window will automatically upscale or downscale, so we can make it any size we want. It starts 5x the size of the original CHIP-8 display. 
    uint32_t texture_width = core.framebuffer().width();    // the width of the CHIP-8 display.
    uint32_t texture_height = core.framebuffer().height();  // the height of the CHIP-8 display.
    uint32_t window_width = texture_width * 5;              // the rendered window's width. 
    uint32_t window_height = texture_height * 5;            // the rendered window's height.
    // create the SDL window and renderer.
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer(window_width, window_height, SDL_WINDOW_RESIZABLE, &window, &renderer);
    SDL_SetWindowTitle(window, "chip8-c++-sdl"); // name out window something meaningful.
    // create our SDL texture.
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, texture_width, texture_height);
    // gets a persistant pointer to the SDL2 keyboard state.
    auto keyboard = SDL_GetKeyboardState(NULL);
    // preparing for the main loop.
    uint32_t instructions_per_frame = 60; // the instructions to execute per frame, helps determine for how long to delay given 60fps. 
    while (true) {
        // poll all SDL2 events.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_WINDOWEVENT:{
                switch (event.window.type) {
                case SDL_WINDOWEVENT_CLOSE: goto exit; // quits the application when closing the window.
                default:;
                }
            }break;
            case SDL_QUIT: goto exit;
            default:;
            }
        }

        // run the core for a set amount of instructions
        core.run_for_instructions_then_tick_timers(instructions_per_frame);

        // update the chip8 hexmap from the SDL2 keys.
        core.update_hexpad({
            keyboard[SDL_SCANCODE_0] != 0,
            keyboard[SDL_SCANCODE_1] != 0,
            keyboard[SDL_SCANCODE_2] != 0,
            keyboard[SDL_SCANCODE_3] != 0,

            keyboard[SDL_SCANCODE_Q] != 0,
            keyboard[SDL_SCANCODE_W] != 0,
            keyboard[SDL_SCANCODE_E] != 0,
            keyboard[SDL_SCANCODE_R] != 0,
            
            keyboard[SDL_SCANCODE_A] != 0,
            keyboard[SDL_SCANCODE_S] != 0,
            keyboard[SDL_SCANCODE_D] != 0,
            keyboard[SDL_SCANCODE_F] != 0,

            keyboard[SDL_SCANCODE_Z] != 0,
            keyboard[SDL_SCANCODE_X] != 0,
            keyboard[SDL_SCANCODE_C] != 0,
            keyboard[SDL_SCANCODE_V] != 0,
        });

        // updates the texture. the texture will be rendered by itself
        const uint32_t* core_pixel_data = core.framebuffer().ptr_begin();
        const size_t pixel_size = sizeof(*core_pixel_data); // 4
        void* texture_pixel_data = 0;
        int pitch = 0;
        assert(!SDL_LockTexture(texture, NULL, &texture_pixel_data, &pitch)); // locks the SDL2 texture for modification.
        for (size_t i = 0; i < texture_height; ++i) {
            // copies the core framebuffer over to the diplayed SDL2 texture.
            void* dest_ptr = reinterpret_cast<unsigned char*>(texture_pixel_data) + pitch * i; // casting to unsigned char allows is defined behaviour.
            const void* src_ptr = core_pixel_data + i * texture_width;
            std::memcpy(dest_ptr, src_ptr, texture_width * pixel_size);
        }
        SDL_UnlockTexture(texture); // unlocks the texture, uploading the changes.
        
        // presents the rendered frame.
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        // sleep for a whole frame.
        SDL_Delay(17 /* 1000 miliseconds is one second, divided by 60 is rougly 17ms */);

        __asm__ volatile (""); // prevents infinite loops from being optimized out.
    }
    exit:;

    // code cleanup.
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
}