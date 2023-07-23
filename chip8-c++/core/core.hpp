// no duplicate includes (sorta redundant in this project as there is only one header file, but good practice).
#pragma once

// gives the basic integer types with set bit width like int32_t, meaning it's guaranteed to be 32 bits.
#include<stdint.h>
#include<stddef.h>
// gives the templated `std::array` type, which demonstrates modern C++ programming with prefering the STL over C types.
#include<array>
// gives the assert function.
#include<assert.h>
// gives the log2 function.
#include<cmath>

/// the framebuffer type encapsulates accessing (modifying and reading) from the framebuffer
/// to avoid dealing with pointers as much as possible.
struct Framebuffer {
    /// the default constructor of the Framebuffer type. It's implicit but specified explictly for clarity.
    Framebuffer() = default;

    /// the value of a pixel that is off.
    static const uint32_t PIXEL_OFF = 0;
    /// the value of a pixel that is on.
    static const uint32_t PIXEL_ON = UINT32_MAX;

    /// @brief checks the status of a pixel in the framebuffer if it is on or off.
    /// @param x the c coordinate of the pixel, must be [0, framebuffer.width())
    /// @param y the y coordinate of the pixel, must be [0, framebuffer.height()] 
    /// @return whether the pixel is on `true` or off `false`
    bool pixel_status(size_t x, size_t y) const {
        assert(x < FB_WIDTH && y < FB_HEIGHT);
        return this->pixel_array[y * FB_WIDTH + x] == PIXEL_ON;
    }

    /// @brief sets the status of a pixel in the framebuffer to either on or off.
    /// @param x the c coordinate of the pixel, must be [0, framebuffer.width())
    /// @param y the y coordinate of the pixel, must be [0, framebuffer.height()] 
    /// @param status the value to set the pixel to, on `true` or off `false`
    void set_pixel(size_t x, size_t y, bool status) {
        assert(x < FB_WIDTH && y < FB_HEIGHT);
        this->pixel_array[y * FB_WIDTH + x] = status ? PIXEL_ON : PIXEL_OFF;
    }

    /// clears the entire framebuffer.
    void clear() {
        this->pixel_array = {PIXEL_OFF};
    }

    /// the width of the framebuffer.
    size_t width() const {
        return FB_WIDTH;
    }

    /// the height of the framebuffer.
    size_t height() const {
        return FB_HEIGHT;
    }

    /// the length of the intenral framebuffer pixel array.
    size_t len() const {
        return this->height() * this->width();
    }

    /// a pointer to the internal framebuffer start.
    const uint32_t* ptr_begin() const {
        return this->pixel_array.begin();
    }

    /// a pointer to the internal framebuffer end.
    const uint32_t* ptr_end() const {
        return this->pixel_array.end();
    }
private:
    /// the framebuffer width.
    static const size_t FB_WIDTH = 60;
    /// the framebuffer height.
    static const size_t FB_HEIGHT = 60;
    /// the internal pixel array.
    std::array<uint32_t, FB_HEIGHT * FB_WIDTH> pixel_array;
};

struct Hexpad {
    /// the default constructor of the Framebuffer type. It's implicit but specified explictly for clarity.
    Hexpad() = default;

    uint16_t bitmap() const {
        return this->hexpad_bitmap;
    }

    bool is_key_pressed(size_t index) const {
        assert(index < 16);
        return (this->hexpad_bitmap & (1 << index)) != 0;
    }

    void update_hexpad(uint16_t bitmap) {
        this->hexpad_bitmap = bitmap;
    }
private:
    uint16_t hexpad_bitmap;
};

/// the main core struct.
/// this is the CHIP-8 implementation core struct, which will be driven by the frontend in `frontend.cpp`.
struct Core {
    /// @brief creates a CHIP-8 core to emulate the CHIP-8 specification. Performs basic initialization of the core before it returns.
    /// @return the initialized CHIP-8 core
    static Core create(char rom[], size_t rom_length) {
        auto core = Core();
        // assert rom isn't too large.
        if (rom_length > 4096 - 512) {
            assert("ROM is too large to be loaded" && false);
        }
        // copy from into memory.
        for (uint32_t i = 0; i < rom_length; ++i) {
            core.mem_write(i + 512, rom[i]);
        }
        // copy font data into memory.
        for (uint32_t i = 0; i < FONT_DATA.size(); ++i) {
            core.mem_write(i, FONT_DATA[i]);
        }
        core.pc = 0x200; // pc starts at address 512 (0x200) in the CHIP-8 spec.
        return core;
    }

    /// runs `instructions` amount of instructions in our core.
    void run_for_instructions(size_t instructions) {
        while (instructions--) {
            this->run_for_instruction();
        }
    }

    /// tick the timers (`timer_sound`, `timer_logic`) of the CHIP-8. Should be done
    /// in combination of presenting the frame.
    void tick_timers() {
        this->timer_delay = this->timer_delay > 0 ? this->timer_delay - 1 : this->timer_delay;
        this->timer_sound = this->timer_sound > 0 ? this->timer_sound - 1 : this->timer_sound;
    }

    /// implicitly inlined. Runs `instructions` amount of instructions and then ticks the timers.
    void run_for_instructions_then_tick_timers(size_t instructions){
        this->run_for_instructions(instructions);
        this->tick_timers();
    }

    /// allows the frontend to access the framebuffer in an immutable way.
    const Framebuffer& framebuffer() const {
        return this->fb;
    }

    /// @brief updates the hexpand using an array of bools, where each corresponding index is the corresponding hexpad key,
    /// @brief going from top left down to bottom right of the hexpad.
    /// @param hexpad the array of bools corresponding to each key of the hexpad, with pressed being `true` and released being `false`.
    void update_hexpad(
        std::array<bool, 16> hexpad
    ) {
        // forms a bitmap from the array of bools
        uint16_t bitmap = hexpad[0] 
            | hexpad[1] << 1
            | hexpad[2] << 2
            | hexpad[3] << 3
            | hexpad[4] << 4
            | hexpad[5] << 5
            | hexpad[6] << 6
            | hexpad[7] << 7
            | hexpad[8] << 8
            | hexpad[9] << 9
            | hexpad[10] << 10
            | hexpad[11] << 11
            | hexpad[12] << 12
            | hexpad[13] << 13
            | hexpad[14] << 14
            | hexpad[15] << 15;
        this->update_hexpad_bitmap(bitmap);
    }
private:
    /// the default constructor of the Core type. It's implicit but specified explictly for clarity.
    /// it's private as to force other initailization of the core, but allow to easily have default values internally.
    Core() = default;

    /// the stack size, which determine the call depth on the core. In other words, how many nested calls we can do before running out of stack space.
    static const size_t STACK_SIZE = 16;

    /// core font data.
    static constexpr std::array<uint8_t, 80> FONT_DATA = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0 font data.
        0x20, 0x60, 0x20, 0x20, 0x70, // 1 font data.
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2 font data.
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3 font data.
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4 font data.
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5 font data.
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6 font data.
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7 font data.
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8 font data.
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9 font data.
        0xF0, 0x90, 0xF0, 0x90, 0x90, // a font data.
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // b font data.
        0xF0, 0x80, 0x80, 0x80, 0xF0, // c font data.
        0xE0, 0x90, 0x90, 0x90, 0xE0, // d font data.
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // e font data.
        0xF0, 0x80, 0xF0, 0x80, 0x80, // f font data.
    };

    /// @brief updates the hexpad using a bitmap.
    /// @param bitmap the bitmap corresponding to the hexpad.
    void update_hexpad_bitmap(uint16_t bitmap) {
        uint16_t old_bitmap = this->hexpad.bitmap();
        // checks whether or not a new key was pressed.
        // if a new key is pressed, then set `is_waiting for_keypress` to `false`, otherwise
        // let `is_watiing_for_keypress` keep its original value.
        uint16_t diff = ((bitmap ^ old_bitmap) & bitmap);
        if (this->is_waiting_for_keypress && diff != 0) {
            this->is_waiting_for_keypress = false;
            auto log2 = std::log2(diff); // gets the recently toggled leftmost bit which will be the index sent to the vx register.
            this->reg_write(this->keypress_index_register, log2);
        }        
        this->hexpad.update_hexpad(bitmap);
    }
    
    /// fetches, decodes and executes a single CHIP-8 instruction
    void run_for_instruction();

    /// @brief reads from a register.
    /// @param index the register index, cannot be above 15
    /// @return the value of the register
    uint8_t reg_read(size_t index) const {
        assert(index < 0x10);
        return this->v[index];
    }

    /// @brief writes to a register.
    /// @param index the register index, cannot be above 15
    /// @param value the value that is written to the register
    void reg_write(size_t index, uint8_t value) {
        assert(index < this->v.size());
        this->v[index] = value;
    }

    /// @brief reads from main memory.
    /// @param address the address of the access, must be below 4096 (0x1000)
    /// @return returns the byte at the `address`
    uint8_t mem_read(uint16_t address) const {
        assert(address < this->main_memory.size());
        return this->main_memory[address];
    }

    /// @brief writes to main memory.
    /// @param address the address of the access, must be below 4096 (0x1000)
    /// @param value the value that is being written to `address`
    void mem_write(uint16_t address, uint8_t value) {
        assert(address < this->main_memory.size());
        this->main_memory[address] = value;
    }

    /// @brief gets the core flag register.
    /// @return gives back either `true` (non-zero) or `false` (zero)
    bool vf_get() const {
        return this->v[15];
    }

    /// @brief sets the flag register.
    /// @param value the value to be put into the vf register
    void vf_set(bool value) {
        this->v[15] = value;
    }

    /// @brief gets the i register.
    /// @return the value of the 12 bit i register
    uint16_t i_get() const {
        return this->i;
    }

    /// @brief sets the i register to a wrapped 12 bit value.
    /// @param value the new i register value wrapped to 12 bits
    void i_set(uint16_t value) {
        this->i = value & 0x0fff;
    }

    /// @brief gets the pc register.
    /// @return the value of the 12 bit pc register
    uint16_t pc_get() const {
        return this->pc;
    }

    /// @brief sets the pc register to a wrapped 12 bit value.
    /// @param value the new pc register value wrapped to 12 bits
    void pc_set(uint16_t value) {
        this->pc = value & 0x0fff;
    }

    /// @brief fetches a byte from memory at the current pc value, then increments pc.
    /// @return the fetcvhed byte pointed to be pc before being incremented
    uint8_t fetch() {
        uint16_t pc = this->pc_get();
        this->pc_set(pc + 1);
        return this->mem_read(pc);
    }

    /// skips the instruction pointed to by pc.
    void skip_instr() {
        this->pc_set(this->pc_get() + 2);
    }

    /// pushes pc onto the stack.
    void stack_push() {
        assert(this->sp < STACK_SIZE);
        this->stack[sp++] = this->pc;
    }

    /// pops pc off of the stack.
    void stack_pop() {
        assert(this->sp > 0);
        this->stack[--sp] = this->pc;
    }

    /// the 16 (0x10) addresable CHIP-8 registers of the core.
    std::array<uint8_t, 0x10> v;
    /// the pc register of the core. Points towards the next instruction to be fetched.
    uint16_t pc;
    /// the i register of the core. Used by memory operations as a pointer to main memory.
    uint16_t i;
    /// the main memory of the core, which is 4096 (0x1000) bytes.
    std::array<uint8_t, 0x1000> main_memory;
    /// the internal stack pointer which is opaque to the CHIP-8 spec, and simply an implementation detail of the core,
    size_t sp;
    std::array<uint16_t, STACK_SIZE> stack;
    /// the sound timer which constantly ticks down at 60hz (it decrements at 60 times a second),
    /// while it is above zero. Once it hits zero, the CHIP-8 will play a sound. The CHIP-8 can set
    /// the sound timer through instructions.
    uint8_t timer_sound;
    /// the delay timer. This timer can be written and read by the core, and just like the sound timer 
    /// it ticks down at 60hz until it reaches zero. Unlike the sound timer, however, it has no immediate
    /// effect, and is instead read by the currently running program in order to determine timing sensitive
    /// logic, such as moving the player or enemies every second.
    uint8_t timer_delay;
    /// the framebuffer of the core. It describes pixel data to be rendered by the frontend, and is updated
    /// by the graphical instructions of the core. 
    Framebuffer fb;
    /// the chip8 hexpad. Is updated by the frontend, preferably whenever a frame is presented.
    Hexpad hexpad;
    /// signaling whether the core is delaying or waiting for a keypress.
    bool is_waiting_for_keypress;
    /// tells the core which register to update the recent keypress index to.
    size_t keypress_index_register;
};