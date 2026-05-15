#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_PROCS 64

struct procinfo {
    int  pid;
    int  state;      
    char name[16];
};

char* state_name(int state) {
    switch (state) {
        case 0: return "unused";
        case 1: return "used";
        case 2: return "sleeping";
        case 3: return "runnable";
        case 4: return "running";
        case 5: return "zombie";
        default: return "unknown";
    }
}


int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}


void usage(void) {
    fprintf(2, "Usage:\n");
    fprintf(2, "  kill <pid>              Kill a process by PID\n");
    fprintf(2, "  kill <pid1> <pid2> ...  Kill multiple PIDs at once\n");
    fprintf(2, "  kill -n <name>          Kill a process by name\n");
}

int str_to_int(const char *s) {
    int n = 0;
    if (*s == '\0') return -1;
    while (*s) {
        if (*s < '0' || *s > '9') return -1;
        n = n * 10 + (*s - '0');
        s++;
    }
    return n;
}


void kill_by_pid(int pid) {

    if (pid <= 0) {
        fprintf(2, "Error: PID must be a positive integer.\n");
        exit(1);
    }
 
    
    if (pid == 1) {
        fprintf(2, "Error: Cannot kill init (PID 1). "
                   "This would crash the system.\n");
        exit(1);
    }
 
    
    if (kill(pid) < 0) {
        fprintf(2, "Error: Failed to kill PID %d. "
                   "Process may not exist.\n", pid);
        exit(1);
    }
 
    printf("Success: Kill signal sent to PID %d.\n", pid);
}

void kill_by_name(const char *name) {
    struct procinfo table[MAX_PROCS];
 

    int n = getprocs(table);
    if (n < 0) {
        fprintf(2, "Error: getprocs() failed.\n");
        exit(1);
    }
 

    int found_pid = -1;
    int matches   = 0;
 
    for (int i = 0; i < n; i++) {
        if (table[i].state == 0) continue;   
 
        if (str_eq(table[i].name, name)) {
            matches++;
            found_pid = table[i].pid;
            printf("  Found: PID %d  state=%s  name=%s\n",
                   table[i].pid,
                   state_name(table[i].state),
                   table[i].name);
        }
    }
 
    
    if (matches == 0) {
        fprintf(2, "Error: No process named '%s' found.\n", name);
        exit(1);
    }
 
    
    if (matches > 1) {
        fprintf(2, "Warning: %d processes named '%s' found.\n",
                matches, name);
        fprintf(2, "Use 'kill <pid>' to target a specific one.\n");
        exit(1);
    }
 
    
    printf("Kill process '%s' (PID %d)? [y/n]: ", name, found_pid);
 
    char buf[4];
    int r = read(0, buf, sizeof(buf) - 1);
    if (r <= 0) {
        fprintf(2, "Error: Could not read input.\n");
        exit(1);
    }
    buf[r] = '\0';
 
    if (buf[0] != 'y' && buf[0] != 'Y') {
        printf("Aborted. No process was killed.\n");
        exit(0);
    }
 
    
    kill_by_pid(found_pid);
}


int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(2, "Error: No arguments provided.\n");
        usage();
        exit(1);
    }
    if (str_eq(argv[1], "-n")) {
        if (argc < 3) {
            fprintf(2, "Error: '-n' flag requires a process name.\n");
            usage();
            exit(1);
        }
        kill_by_name(argv[2]);
        exit(0);
    }
 

    for (int i = 1; i < argc; i++) {
        int pid = str_to_int(argv[i]);
        if (pid < 0) {
            fprintf(2, "Error: '%s' is not a valid PID. Skipping.\n", argv[i]);
            continue;
        }
        kill_by_pid(pid);
    }
 
    exit(0);

}
