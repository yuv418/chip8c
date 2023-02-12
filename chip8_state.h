#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <SDL2/SDL.h>

#include "stack.h"

#define CHIP8_WIDTH 64
#define CHIP8_HEIGHT 32
#define CHIP8_SDL_SCALE 10
#define CHIP8_LOAD_OFFSET 0x200

typedef struct chip8_state_t {
  SDL_Window *window;
  SDL_Surface *surface;

  uint8_t *memory;
  // 0 black, 1 white
  bool display[32][64];
  // Program counter
  uint16_t pc;
  // Used to point at locations in memory
  uint16_t i;
  // Delay timer
  uint8_t delay_timer;
  // Sound timer
  uint8_t sound_timer;
  // the 16 registers
  uint8_t regs[16];
  // This isn't actually that useful,
  // but just... it could be before we implement some stuff.
  uint16_t program_size;
  // the stack.
  struct stack_t *execution_stack;
} chip8_state;

static uint8_t chip8_font[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

struct chip8_state_t *chip8_state_init();
void chip8_state_free(struct chip8_state_t *state);
// Can fetch the instruction here, from the program counter
bool chip8_instruction_decode(struct chip8_state_t *state);
void chip8_draw_screen(struct chip8_state_t *state);
bool chip8_handle_event(struct chip8_state_t *state);
