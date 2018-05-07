#include <stdlib.h>

#include "readline_hack.h"
#include "repl.h"

void repl(vm* cvm) {
  char* line;
  for (;;) {
    if (!(line = readline("> "))) {
      printf("\n");
      break;
    }

    interpret(cvm, line);
    add_history(line);
    free(line);
  }
}

static char* read_file(const char* path) {
  FILE* file = fopen(path, "rb");

  if (!file) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char* buffer = (char*) malloc(file_size + 1);

  if (!buffer) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(74);
  }

  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);

  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }

  buffer[bytes_read] = '\0';

  fclose(file);
  return buffer;
}

void run_file(vm* cvm, const char* path) {
  char* source = read_file(path);
  interpret_result result = interpret(cvm, source);
  free(source);

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}
