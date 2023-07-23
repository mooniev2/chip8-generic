// include the core header to allow usage of the Core struct. 
// keep its implementation separate from the header in the cpp file, 
// since the header is exposed to the frontend.
#include<core.hpp>

// includes the STD implementation of a rand() function
#include<random>

// allows printing errors to the terminal.
#include<iostream>

// unreachable code macro used for unreachable code.
#define unreachable_code assert("unreachable code" && false)

void Core::run_for_instruction() {
    // don't do anything if waiting for a keypress
    if (this->is_waiting_for_keypress) {
        return;
    }

    // fetches the instruction.
    
    // fetches the high byte of the instruction, this increments pc.
    const uint8_t instruction_hi = this->fetch();
    // fetches the low byte of the instruction, this increments pc.
    const uint8_t instruction_lo = this->fetch();
    // compiles the low and high byte into a single instruction word (16 bits).
    const uint16_t instruction = (instruction_hi << 8) | instruction_lo;

    // decodes the fetched instruction word into its components. Uses 32 bit values due to C++ doing integer promotion anyway, 
    // as well as being faster on certain architectures
    const uint32_t nnn = instruction & 0x0fff;      // masks awaway only the relevant bits with a binary mask, which means the upper 4 bits are ignored.
    const uint32_t nn = instruction & 0x00ff;       // masks away the upper byte of the instruction word.
    const uint32_t n = instruction & 0x000f;        // masks away the upper 12 bits of the instruction word, leaving just a nibble.  

    const uint32_t x = (instruction >> 8) & 0xf;    // masks out the `x` nibble out of every instruction word.
    const uint32_t y = (instruction >> 4) & 0xf;    // masks out the 'y' nibble out of every instruction word.
    const uint8_t vx = this->reg_read(x);           // the vx register, which is the register indexed by the 'x' nibble of the instruction word.
    const uint8_t vy = this->reg_read(y);           // the vy register, which is the register indexed by the 'y' nibble of the instruction word.

    const uint8_t v0 = this->reg_read(0);           // the first reigster, or v[0], at the start of the instruction.
    const uint16_t i = this->i_get();               // the i register at the start of the instruction.
    const uint16_t pc = this->pc_get();

    const bool key_pressed = this->hexpad.is_key_pressed(vx & 0xf);

    // executes the instruction.

    // extracts the top 4 bits of the instruction. can omit masking since the value is 16 bits and therefore only 4 bits remain.
    const uint8_t in000 = instruction >> 12; 
    // extracts the bottom 4 bits of the instruction by masking.
    const uint8_t i000n = instruction & 0x000f;
    // extracts the bottom 8 bits of the instruction by masking.
    const uint8_t i00nn = instruction & 0x00ff;

    // matches on the instruction bits which instruction to execute
    switch (instruction) {
    case 0x00e0: { // 00E0
        this->fb.clear();
    } break;
    case 0x00ee: { // 00EE
        this->stack_pop();
    } break;
    default:{
        switch (in000) {
        // this instruction was intended to call COSMAC VIP instruction for the original interpreter, it is unneeded for some CHIP-8 compliant programs and won't
        // be implemented here.
        case 0: goto invalid_instr; 
        case 1:{ // 1NNN
            this->pc_set(nnn);
        } break;
        case 2:{ // 2NNN
            this->stack_push();
            this->pc_set(nnn);
        } break;
        case 3:{ // 3XNN
            if (vx == nn) this->skip_instr();
        } break;
        case 4:{ // 4XNN
            if (vx != nn) this->skip_instr();
        } break;
        case 5:{ // 5XY0
            if (vx == vy) this->skip_instr();
        } break;
        case 6:{ // 6XNN
            this->reg_write(x, nn);
        } break;
        case 7:{ // 7XNN
            this->reg_write(x, vx + nn);
        } break;
        case 8:{
            switch (i000n) {
            case 0: { // 8XY0
                this->reg_write(x, vy);
            } break;
            case 1:{ // 8XY1
                this->reg_write(x, vx | vy);
            } break;
            case 2:{ // 8XY2
                this->reg_write(x, vx & vy);
            } break;
            case 3:{ // 8XY3
                this->reg_write(x, vx ^ vy);
            } break;
            case 4:{ // 8XY4 
                uint32_t res = vx + vy; 
                bool carry = res > UINT8_MAX;
                this->reg_write(x, res);
                this->vf_set(carry);
            } break;
            case 5:{ // 8XY5
                uint32_t res = vx - vy;
                bool no_borrow = res < 0;
                this->reg_write(x, res);
                this->vf_set(no_borrow);
            } break;
            case 6:{ // 8XY6
                bool lsb = vx & 1;
                this->reg_write(x, vx >> 1);
                this->vf_set(lsb);
            } break;
            case 7:{ // 8XY7
                uint32_t res = vy - vx;
                bool no_borrow = res < 0;
                this->reg_write(x, res);
                this->vf_set(no_borrow);
            } break;
            case 0xe:{ // 8XYE
                bool msb = vx >> 7;
                this->reg_write(x, vx << 1);
                this->vf_set(msb);
            } break;
            default: goto invalid_instr;
            }
        } break;
        case 9:{ // 9XY0
            if (vx != vy) this->skip_instr();
        } break;
        case 0xa:{ // ANNN
            this->i_set(nnn);
        } break;
        case 0xb:{ // BNNN
            this->pc_set(v0 + nnn);
        } break;
        case 0xc:{ // CXNN
            this->reg_write(x, std::rand() & nn);
        } break;
        case 0xd:{ // DXYN
            bool collision = false;
            for (uint32_t h = 0; h < n; ++h){
                uint8_t row = this->mem_read(i + h); 
                for (uint32_t w = 0; w < n; ++w) {
                    uint32_t xp = (vx + w) % this->fb.width();
                    uint32_t yp = (vy + h) % this->fb.height();
                    bool new_pixel = (row & (1 << w)) != 0;
                    bool old_pixel = this->fb.pixel_status(xp, yp);
                    collision = collision || (old_pixel != new_pixel);
                    this->fb.set_pixel(x, y, true); 
                }
            }
            this->vf_set(collision);
        } break;
        case 0xe:{
            switch (i00nn) {
            case 0x9e:{ // EX9E
                if (key_pressed) this->skip_instr();
            } break;
            case 0xa1:{ // EXA1
                if (!key_pressed) this->skip_instr();
            } break;
            default: goto invalid_instr;
            }
        } break;
        case 0xf:{
            switch (i00nn) {
            case 0x07:{ // FX07
                this->reg_write(x, this->timer_delay);
            } break;
            case 0x0a:{ // FX0A
                this->is_waiting_for_keypress = true;   // enables delaying until keypress. 
                this->keypress_index_register = x;      // tells the core which register to put the key index into.
            } break;
            case 0x15:{ // FX15
                this->timer_delay = vx;
            } break;
            case 0x18:{ // FX18
                this->timer_sound = vx;
            } break;
            case 0x1e:{ // FX1E
                this->i_set(i + vx);
            } break;
            case 0x29:{ // FX29
                // points i towards the currently used hexadecimal font character.
                // the font layout is 4x5, which means it has 4 columns and 5 rows. 
                // Every row is a single byte, where the first 4 bits are the columns.
                // Every character is stored in memory from address 0 and upwards, meaning 
                // that to get i to point to the correct character, we just need to set it to the  
                // current character * 5, since 0 is the origin and every character is 5 bytes.
                this->i_set(vx * 5); 
            } break;
            case 0x33:{ // FX33
                uint8_t d0 = vx % 10;
                uint8_t d1 = (vx / 10) % 10;
                uint8_t d2 = (vx / 100) % 10;
                this->mem_write(i, d2);
                this->mem_write(i + 1, d1);
                this->mem_write(i + 2, d0);
            } break;
            case 0x55:{ // FX55
                for (uint32_t j = 0; j <= x; ++j) {
                    this->mem_write(i + j, this->reg_read(j));
                }
            } break;
            case 0x65:{ // FX65
                for (uint32_t j = 0; j <= x; ++j) {
                    this->reg_write(j, this->mem_read(i + j));
                }
            } break;
            default: goto invalid_instr;
            }
        } break;
        default: unreachable_code;
        }
        break;
        invalid_instr:
        std::cerr << "invalid instruction: [0x" << std::hex << pc - 2 << "] 0x" << instruction << std::endl;
        assert("invalid instruction" && false); 
    };
    }

}