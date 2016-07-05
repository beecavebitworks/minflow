#define DEBUG 1
#include <statproc/statutils.h>
#include <stdio.h>
#include <assert.h>

int main(argc, char *argv[])
{
  uint32_t prev = 0xFFFFFFF0;
  uint32_t cur = 0x0F;

  uint32_t diff = u32diff(prev, cur);

  assert(diff == (0xF + 0xF));

  return 0;
}
