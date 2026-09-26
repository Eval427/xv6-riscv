#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int n=0;
    if (argc >= 2) n = atoi(argv[1]);
    else {
        printf("Usage: sysinfo <0/1/2>\n"); // active processes/num syscalls/free mem pages
        return -1;
    }
    
    return sysinfo(n);
}