#include "chip8_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  // Init chip8 state
  struct chip8_state_t *state = chip8_state_init();
  if (state == NULL) {
    printf("chip8_state_init returned NULL pointer\n");
    return EXIT_FAILURE;
  }
  // Load the "ROM" (not ROM) into the CHIP8 memory
  if (argc < 2) {
    printf("Usage: chip8c [romfile]\n");
    return EXIT_FAILURE;
  }
  char *romfile_path = argv[1];
  FILE *romfile = fopen(romfile_path, "r");

  // get the size of the rom
  fseek(romfile, 0, SEEK_END);
  size_t romfile_len = ftell(romfile);
  rewind(romfile);

  // read the rom into memory starting at 0x200
  fread(state->memory + CHIP8_LOAD_OFFSET, romfile_len, 1, romfile);
  state->program_size = CHIP8_LOAD_OFFSET + romfile_len;
  fclose(romfile);

  // state->memory[0x1ff] = 3;

  // Main loop
  while (true) {
    // Fetch decode execute
    chip8_instruction_decode(state);
    chip8_draw_screen(state);
    /*if (!chip8_handle_event(state)) {
      break;
    }*/
    // 700 instructions per second
    usleep(1.0 / 700.0);
  }

  chip8_state_free(state);
}
