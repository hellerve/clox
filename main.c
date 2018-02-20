#include "src/chunk.h"
#include "src/common.h"
#include "src/debug.h"

int main(int argc, const char** argv) {
  chunk c;
  int i;
  init_chunk(&c);
  for (i = 1; i < 258; i++) write_constant(&c, 1.2*i, i);
  write_chunk(&c, OP_RETURN, i);
  disassemble_chunk(&c, "test chunk");
  free_chunk(&c);
  return 0;
}
