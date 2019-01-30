#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/repl.h"
#include "src/vm.h"

int main(int argc, const char** argv) {
  int ret = 0;
  vm* cvm = init_vm();

  switch (argc) {
  case 1: repl(cvm); break;
  case 2: run_file(cvm, argv[1]); break;
  default:
    fprintf(stderr, "Usage: %s [file]\n", argv[0]);
    fputs(
      "  The [file] variable is optional.\n"
      "  Specifying it will result in that file being run, running clox without it will\n"
      "  start a REPL.\n",
      stderr);
    ret = 1;;
  }

  free_vm(cvm);

  return ret;
}
