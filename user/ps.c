#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

struct procinfo {
    int pid;
    int state;
    char name[16];
};

char* get_state_string(int state) {
    switch(state) {
        case 0: return "UNUSED";
        case 1: return "USED";
        case 2: return "SLEEPING";
        case 3: return "RUNNABLE";
        case 4: return "RUNNING";
        case 5: return "ZOMBIE";
        default: return "???";
    }
}

int main(int argc, char *argv[]) {
    struct procinfo procs[64]; 
    int num_procs;
    num_procs = getprocs(procs);
    if(num_procs < 0) {
        printf("ps: failed to retrieve processes\n");
        exit(1);
    }
    printf("PID    STATE       NAME\n");
    for(int i = 0; i < num_procs; i++) {
        char *state = get_state_string(procs[i].state);
        printf("%d      %s", procs[i].pid, state);
        int len = strlen(state);
        for(int j = len; j < 12; j++) printf(" ");
        printf("%s\n", procs[i].name);
    }
    exit(0);
}
