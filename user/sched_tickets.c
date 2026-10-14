#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int n;

    if (argc >= 2) n = atoi(argv[1]);
    else {
        printf("Usage: sched_tickets <tickets>\n");
        return -1;
    }

    return sched_tickets(n);
}