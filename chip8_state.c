#include "chip8_state.h"
#include "SDL_events.h"
#include "SDL_surface.h"
#include "SDL_video.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct chip8_state_t *chip8_state_init() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return NULL;
  }
  SDL_Window *window =
      SDL_CreateWindow("chip8c", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 640, 320, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    return NULL;
  }
  SDL_Surface *surface = SDL_GetWindowSurface(window);

  struct chip8_state_t *state = malloc(sizeof(struct chip8_state_t));

  state->window = window;
  state->surface = surface;

  // Alloc 4096 bytes of memory
  state->memory = malloc(4096);
  // Assume program is loaded starting at 0x0
  state->pc = CHIP8_LOAD_OFFSET;
  state->program_size = 0x0;
  // Clear all the state
  state->i = 0x0;
  state->delay_timer = 0x0;
  state->sound_timer = 0x0;
  for (int i = 0; i < 16; i++) {
    state->regs[i] = 0;
  }
  state->execution_stack = NULL;

  // Load fonts into memory between 0x50 and 0x9f
  // Can do sizeof since chip8 is a byte array
  memcpy(state->memory + 0x50, chip8_font, sizeof(chip8_font));

  return state;
}

void chip8_state_free(struct chip8_state_t *state) {
  free(state->memory);

  SDL_DestroyWindow(state->window);
  SDL_Quit();

  free(state);
}

bool chip8_instruction_decode(struct chip8_state_t *state) {
  // REMOVE THIS (also it looks to not even work right)
  // printf("%d , %d\n", state->pc, state->program_size);
  if (state->pc > state->program_size) {
    return true;
  }
  // Fetch instruction.
  // I think this is better than trying to read a uint16_t because of
  // endianness reasons.
  uint8_t inst1 = state->memory[state->pc];
  uint8_t inst2 = state->memory[state->pc + 1];
  uint16_t inst = (inst1 << 8) + inst2;
  state->pc += 2;

  if (inst != 0x1228) {
    printf("Instruction: 0x%04x\n", inst);
  }

  // Extract x, y, n, and nnn from the opcode. These will always be in the
  // same places in the opcode

  // second nibble
  int x = (inst & 0x0f00) / 0x100;
  // third nibble
  int y = (inst & 0x00f0) / 0x10;
  // fourth nibble
  int n = (inst & 0x000f);
  // second byte
  int nn = (inst & 0x00ff);
  // 12 bit address
  int nnn = (inst & 0x0fff);

  // Implement:
  // 00E0
  // 1nnn
  // 6xnn
  // 7xnn
  // annn
  // dxyn
  // This is the first digit of the instruction
  switch (inst >> 12) {
    // Useless machine language stuff
  case 0x0:
    switch (inst) {
      // Clear screen
    case 0x00e0:
      SDL_FillRect(state->surface, NULL, 0x000000);
      break;
    }
    break;
  // Jump
  case 0x1:
    // 0x1NNN: set PC to NNN
    state->pc = nnn;
    break;
  // Set register X to NN
  case 0x6:
    state->regs[x] = nn;
    break;
  // Add NN to value of X.
  case 0x7:
    state->regs[x] += nn;
    break;
  // Set the I register to NNN
  case 0xa:
    state->i = nnn;
    break;
  case 0xd:
    // This coordinate will wrap if the value in the register is more than
    // 64
    uint16_t vx = state->regs[x] % 64;
    uint16_t vy = state->regs[y] % 32;
    state->regs[0xf] = 1;
    printf("vx, vy, n %d %d %d\n", vx, vy, n);
    printf("i reg 0x%x\n", state->i);
    // Draw n tall pixels
    for (int i = state->i; i < state->i + n; i++) {
      // Sprite is 8 horizontal bits
      int xvx = vx;
      uint8_t sprite = state->memory[i];
      for (int j = 7; j >= 0; j--) {
        if (xvx > CHIP8_WIDTH) {
          break;
        }
        // Get nth bit of sprite

        bool sprite_bit = (sprite >> (j)) & 1;

        if (state->display[vy][xvx] && sprite_bit) {
          state->display[vy][xvx] = false;
          state->regs[0xf] = 1;
        } else if (sprite_bit && !state->display[vy][xvx]) {
          state->display[vy][xvx] = true;
        }
        // If the sprite has an "on" pixel, flip the display bit
        xvx++;
      }
      vy++;
    }
    break;
  default:
    printf("Unimplemented instruction 0x%x\n", inst);
  }
  return true;
}

void chip8_draw_screen(struct chip8_state_t *state) {
  // 10x10 pixel
  SDL_Rect pixel = {0, 0, 10, 10};
  for (int i = 0; i < 64; i++) {
    for (int j = 0; j < 32; j++) {
      pixel.x = (i * 10);
      pixel.y = (j * 10);

      // "on" pixel is white, "off" pixel is black
      uint32_t color = state->display[j][i] ? 0xffffff : 0x000000;

      SDL_FillRect(state->surface, &pixel, color);
    }
  }
  SDL_UpdateWindowSurface(state->window);
}

bool chip8_handle_event(struct chip8_state_t *state) {
  SDL_Event ev;
  SDL_PollEvent(&ev);
  switch (ev.type) {
  case SDL_KEYDOWN:
    switch (ev.key.keysym.sym) {

    case SDLK_ESCAPE:
      return false;
      break;
    }
    break;
  case SDL_WINDOWEVENT:
    switch (ev.window.event) {
    case SDL_WINDOWEVENT_CLOSE:
      return false;
      break;
    }
  }
  return true;
}
