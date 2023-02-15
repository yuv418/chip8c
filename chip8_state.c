#include "chip8_state.h"
#include "SDL_events.h"
#include "SDL_keycode.h"
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
  // Clear display state
  for (int i = 0; i < CHIP8_HEIGHT; i++) {
    for (int j = 0; j < CHIP8_WIDTH; j++) {
      state->display[i][j] = 0;
    }
  }
  state->execution_stack = NULL;

  // Load fonts into memory between 0x50 and 0x9f
  // Can do sizeof since chip8 is a byte array
  memcpy(state->memory + 0x50, chip8_font, sizeof(chip8_font));

  return state;
}

void chip8_get_key(struct chip8_state_t *state, int x) {
  SDL_Event event;
  // Because this doesn't increment the program counter, or something
  // state->pc -= 2;
  uint8_t key_val = 0x0;
  while (event.type != SDL_KEYUP) {
    SDL_WaitEvent(&event);
    printf("Event handled\n");
  }
  printf("key %d\n", event.key.keysym.sym);
  switch (event.key.keysym.sym) {
  case SDLK_1:
    key_val = 0;
    break;
  case SDLK_2:
    key_val = 1;
    break;
  case SDLK_3:
    key_val = 2;
    break;
  case SDLK_4:
    key_val = 3;
    break;
  case SDLK_q:
    key_val = 4;
    break;
  case SDLK_w:
    key_val = 5;
    break;
  case SDLK_e:
    key_val = 6;
    break;
  case SDLK_r:
    key_val = 7;
    break;
  case SDLK_a:
    key_val = 8;
    break;
  case SDLK_s:
    key_val = 9;
    break;
  case SDLK_d:
    key_val = 0xa;
    break;
  case SDLK_f:
    key_val = 0xb;
    break;
  case SDLK_z:
    key_val = 0xc;
    break;
  case SDLK_x:
    key_val = 0xd;
    break;
  case SDLK_c:
    key_val = 0xe;
    break;
  case SDLK_v:
    key_val = 0xf;
    break;
  }
  //  state->pc += 2;

  state->regs[x] = key_val;
  printf("state->regs[%d] (%d) = %d", x, state->regs[x], key_val);
}

void chip8_state_free(struct chip8_state_t *state) {
  free(state->memory);

  SDL_DestroyWindow(state->window);
  SDL_Quit();

  free(state);
}

// skip 1 instruction
void chip8_skip(struct chip8_state_t *state) { state->pc += 2; }

void chip8_8xyi(struct chip8_state_t *state, int x, int y, int i) {

  // Last nibble of inst
  printf("last nibble of 0x8 inst is %d\n", i);
  switch (i) {
  // VX = VY
  case 0x0:
    printf("state->regs[%d] (%d) = state->regs[%d] (%d)\n", x, state->regs[x],
           y, state->regs[y]);
    state->regs[x] = state->regs[y];
    break;
  // VX = VX | VY
  case 0x1:
    // I realize that I can do state->regs[x] |= state->regs[y] but this is
    // more literate.
    state->regs[x] = state->regs[x] | state->regs[y];
    break;
  // VX = VX & VY
  case 0x2:
    state->regs[x] = state->regs[x] & state->regs[y];
    break;
  // VX = VX ^ VY
  case 0x3:
    state->regs[x] = state->regs[x] ^ state->regs[y];
    break;
  // VX = VX + VY
  case 0x4:
    // Set carry flag if overflow
    if (255 - state->regs[y] < state->regs[x]) {
      state->regs[0xf] = 1;
    }
    state->regs[x] = state->regs[x] + state->regs[y];

    // CARRY FLAG
    // if (state -)
    break;
  // VX = VX - VY
  case 0x5:
    state->regs[x] = state->regs[x] - state->regs[y];
    if (state->regs[x] > state->regs[y]) {
      state->regs[0xf] = 1;
    }
    break;
  // VX = VY - VX
  case 0x7:
    state->regs[x] = state->regs[y] - state->regs[x];
    if (state->regs[y] > state->regs[x]) {
      state->regs[0xf] = 1;
    }
    break;
  // TODO the following two may have BUGS.
  // Right shift
  case 0x6:
    // TODO broken
    state->regs[x] = state->regs[y];
    state->regs[0xf] = ((state->regs[x] >> 1) << 1) != state->regs[x];
    state->regs[x] = state->regs[x] >> 1;
    break;
  // Left shift
  case 0xe:
    state->regs[x] = state->regs[y];
    // I have no idea how to get the "lost bit" right now
    // it's late my brain doesn't work at these ungodly times
    state->regs[0xf] = ((state->regs[x] << 1) >> 1) != state->regs[x];
    state->regs[x] = state->regs[x] << 1;
    break;
  }
}

void chip8_dxyn(struct chip8_state_t *state, int x, int y, int n) {
  // This coordinate will wrap if the value in the register is more than
  // 64
  uint16_t vx = state->regs[x] % 64;
  uint16_t vy = state->regs[y] % 32;
  state->regs[0xf] = 0;

  printf("vx, vy, n %d %d %d\n", vx, vy, n);
  printf("i reg 0x%x\n", state->i);
  // Draw n tall pixels
  for (int i = state->i; i < state->i + n; i++) {
    // Sprite is 8 horizontal bits
    uint8_t sprite = state->memory[i];
    for (int j = 0; j < 8; j++) {
      /* if (vx + i > CHIP8_WIDTH) { */
      /*   break; */
      /* } */
      /* // Get nth bit of sprite */
      // bool sprite_bit = (sprite >> (7 - j)) & 1;
      uint8_t sprite_bit = sprite & (0x80 >> j);

      // If the sprite has an "on" pixel, flip the display bit
      if (sprite_bit) {
        if (state->display[vy][vx + j]) {
          state->regs[0xf] = 1;
        }
        state->display[vy][vx + j] ^= 1;
        printf("Set display bit at %d %d to %d\n", vy, vx + j,
               state->display[vy][vx + j]);
      }
    }
    vy++;
  }
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
    // Return from a subroutine (function)
    case 0x00ee:
      state->pc = pop(&state->execution_stack);
      printf("Subroutine returned %d from stack\n", state->pc);
      break;
    }
    break;
  // Jump
  case 0x1:
    // 0x1NNN: set PC to NNN
    state->pc = nnn;
    break;
  // Execute a subroutine (function)
  case 0x2:
    printf("Pushing %d onto stack for subroutine\n", state->pc);
    state->execution_stack = push(state->execution_stack, state->pc);
    state->pc = nnn;
    break;
  case 0x3:
    if (state->regs[x] == nn) {
      printf("Skipping instruction as state->regs[%d] (%d) == %d\n", x,
             state->regs[x], nn);
      chip8_skip(state);
    }
    break;
  case 0x4:
    if (state->regs[x] != nn) {
      printf("Skipping instruction as state->regs[%d] (%d) != %d\n", x,
             state->regs[x], nn);
      chip8_skip(state);
    }
    break;
  case 0x5:
    if (state->regs[x] == state->regs[y]) {
      chip8_skip(state);
    }
    break;
  case 0x9:
    if (state->regs[x] != state->regs[y]) {
      chip8_skip(state);
    }
    break;

  // Set register X to NN
  case 0x6:
    printf("Setting state->regs[%d] (%d) = %d\n", x, state->regs[x], nn);
    state->regs[x] = nn;
    break;
  // Add NN to value of X.
  case 0x7:
    state->regs[x] += nn;
    break;
  // Arithmetic handlers
  case 0x8:
    printf("Arithmetic instruction called!\n");
    chip8_8xyi(state, x, y, n);
    break;
  // Set the I register to NNN
  case 0xa:
    state->i = nnn;
    break;
  // TODO "ambiguous instruction"
  case 0xb:
    state->pc = nnn + state->regs[0];
  // Random number generator. TODO Does this work?
  case 0xc:
    state->regs[x] = random() & nn;
    break;
  case 0xd:
    chip8_dxyn(state, x, y, n);
    break;
  case 0xe: {
    int subinst = inst & 0x00ff;
    SDL_Event event;
    printf("subinst is 0x%x\n", subinst);
    if (SDL_PollEvent(&event)) {
      if ((subinst == 0x9e && event.type == SDL_KEYUP) ||
          (subinst == 0xa1 && event.type != SDL_KEYUP)) {
        printf("skipping instruction\n");
        chip8_skip(state);
      }
    }
    break;
  }

  case 0xf:
    switch (inst & 0x00ff) {
    // Add X to i register
    case 0x1e:
      state->i = state->i + state->regs[x];
      break;
    // Fetch key event
    case 0x0a:
      printf("chip8_get_key\n");
      chip8_get_key(state, x);
      break;
    // TODO ambiguous instruction
    case 0x55:
      // Saves every register to memory
      for (int i = 0; i <= x; i++) {
        state->memory[state->i + i] = state->regs[i];
      }
      break;
    // TODO ambiguous instruction
    case 0x65:
      printf("Loading registers 0 to %d from memory\n", x);
      for (int i = 0; i <= x; i++) {
        printf("state->regs[%d] = state->memory[%d + %d] (%d)\n", i, state->i,
               i, state->memory[state->i + i]);
        state->regs[i] = state->memory[state->i + i];
      }
    case 0x33: {
      uint8_t vx = state->regs[x];
      // This is a bad implementation
      if (vx < 100) {
        // Tens
        state->memory[state->i] = vx / 10;
        // Ones
        state->memory[state->i + 1] = vx % 10;
      } else if (vx < 10) {
        // Ones
        state->memory[state->i + 1] = vx;
      } else {
        // Hundreds
        state->memory[state->i] = vx / 100;
        // Tens
        state->memory[state->i + 1] = (vx % 100) / 10;
        // Ones
        state->memory[state->i + 2] = vx % 10;
      }
    }
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
