#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/repl.h"
#include "src/vm.h"

int main(int argc, const char** argv) {
  vm* cvm = init_vm();

  switch (argc) {
  case 1: repl(cvm); break;
  case 2: run_file(cvm, argv[1]); break;
  default:
    fprintf(stderr, "Usage: clox [path]\n");
    exit(64);
  }

  free_vm(cvm);
  return 0;
}
