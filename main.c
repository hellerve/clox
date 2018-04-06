#include "src/chunk.h"
#include "src/common.h"
#include "src/debug.h"
#include "src/vm.h"

int main(int argc, const char** argv) {
  chunk c;
  int i = 1;
  vm* cvm = init_vm();
  init_chunk(&c);
  for (int _ = 0; _ < 255; _++) write_constant(&c, 1.2, i);
  write_constant(&c, 3.4, i);

  write_chunk(&c, OP_ADD, i);

  write_constant(&c, 5.6, i);

  write_chunk(&c, OP_DIVIDE, i);
  write_chunk(&c, OP_NEGATE, i);
  write_chunk(&c, OP_RETURN, i);
  interpret(cvm, &c);
  free_vm(cvm);
  free_chunk(&c);
  return 0;
}
