#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/proc.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  uint64 pinfo_addr;

  if (argc < 2) {
    fprintf(2, "Usage: procinfo <pinfo pointer>\n");
    return -1;
  }

  pinfo_addr = (uint64)atoi(argv[1]);

  return procinfo((struct pinfo *)pinfo_addr);
}