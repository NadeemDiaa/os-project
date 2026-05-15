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
        case 0: return "UNUSED";
        case 1: return "USED";
        case 2: return "SLEEPING";
        case 3: return "RUNNABLE";
        case 4: return "RUNNING";
        case 5: return "ZOMBIE";
        default: return "UNKNOWN";
    }
}
void print_line(void) {
    printf("------------------------------------\n");
}
int print_table(struct procinfo *table) {
    int n = getprocs(table);
    if (n < 0) {
        fprintf(2, "Error: getprocs() failed.\n");
        return -1;
    }
 
    // Count by state for the summary line
    int running  = 0;
    int sleeping = 0;
    int zombie   = 0;
    int total    = 0;
 
    print_line();
    printf("  PID    STATE       NAME\n");
    print_line();
 
    for (int i = 0; i < n; i++) {
        if (table[i].state == 0) continue;   // skip UNUSED
 
        printf("  %d      %s      %s\n",
               table[i].pid,
               state_name(table[i].state),
               table[i].name);
 
        total++;
        if (table[i].state == 4) running++;
        if (table[i].state == 2) sleeping++;
        if (table[i].state == 5) zombie++;
    }
 
    print_line();
 
    // Summary bar — like htop's footer counts
    printf("  Total: %d | Running: %d | Sleeping: %d",
           total, running, sleeping);
 
    if (zombie > 0)
        printf(" | Zombie: %d  <-- WARNING", zombie);
 
    printf("\n");
    print_line();
 
    return total;
}


void do_kill(int pid) {
    if (pid <= 0) {
        printf("Error: PID must be a positive integer.\n");
        return;
    }
    if (pid == 1) {
        printf("Error: Cannot kill init (PID 1). "
               "This would crash the system.\n");
        return;
    }
    if (kill(pid) < 0) {
        printf("Error: Failed to kill PID %d. "
               "Process may not exist.\n", pid);
        return;
    }
    printf("Success: Kill signal sent to PID %d.\n", pid);
}


int read_line(char *buf, int len) {
    int i = 0;
    char c;
    while (i < len - 1) {
        int r = read(0, &c, 1);
        if (r <= 0) break;
        if (c == '\n') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
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
int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}
int split(char *buf, char *argv[], int maxargs) {
    int argc = 0;
    while (*buf) {
        // skip spaces
        while (*buf == ' ') buf++;
        if (*buf == '\0') break;
        // start of a token
        argv[argc++] = buf;
        if (argc >= maxargs) break;
        // find end of token
        while (*buf && *buf != ' ') buf++;
        if (*buf == ' ') *buf++ = '\0';
    }
    return argc;
}
void handle_kill_by_name(const char *name, struct procinfo *table, int n) {
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
        printf("  Error: No process named '%s' found.\n", name);
        return;
    }
    if (matches > 1) {
        printf("  Warning: %d processes named '%s' found.\n", matches, name);
        printf("  Use a PID to target a specific one.\n");
        return;
    }
 
    // Exactly one match — confirm
    char buf[4];
    printf("  Kill '%s' (PID %d)? [y/n]: ", name, found_pid);
    read_line(buf, sizeof(buf));
 
    if (buf[0] == 'y' || buf[0] == 'Y') {
        do_kill(found_pid);
    } else {
        printf("  Aborted.\n");
    }
}





int main(void) {
    struct procinfo table[MAX_PROCS];
    char input[64];
    char *argv[16];
 
    while (1) {
        
        printf("\n");
        printf("====================================\n");
        printf("   XV6 PROCESS VIEWER & TERMINATOR  \n");
        printf("====================================\n");
 
        int n = getprocs(table);
        if (n < 0) { fprintf(2, "Error: getprocs() failed.\n"); exit(1); }
 
        print_table(table);
 
        
        printf("\n");
        printf("  Commands:\n");
        printf("    <pid>            Kill by PID\n");
        printf("    <pid> <pid> ...  Kill multiple PIDs\n");
        printf("    -n <name>        Kill by name\n");
        printf("    r                Refresh\n");
        printf("    q                Quit\n");
        printf("\n");
        printf("> ");
 
        
        int len = read_line(input, sizeof(input));
        if (len == 0) continue;
 
        int argc = split(input, argv, 16);
        if (argc == 0) continue;
 
        if (str_eq(argv[0], "q")) {
            printf("Goodbye.\n");
            exit(0);
        }
 
        if (str_eq(argv[0], "r")) continue;
 
        if (str_eq(argv[0], "-n")) {
            if (argc < 2) {
                printf("  Error: '-n' requires a process name.\n");
                printf("  Example: -n sh\n");
            } else {
                handle_kill_by_name(argv[1], table, n);
            }
            printf("Press Enter to continue...\n");
            read_line(input, sizeof(input));
            continue;
        }
 
        int valid = 0;
        int pids[16];
        int pidcount = 0;
 
        for (int i = 0; i < argc; i++) {
            int pid = str_to_int(argv[i]);
            if (pid < 0) {
                printf("  Error: '%s' is not a valid PID. Skipping.\n", argv[i]);
            } else {
                pids[pidcount++] = pid;
                valid++;
            }
        }
 
        if (pidcount == 0) {
            printf("Press Enter to continue...\n");
            read_line(input, sizeof(input));
            continue;
        }
 
        printf("  About to kill %d process(es): ", pidcount);
        for (int i = 0; i < pidcount; i++) {
            printf("%d", pids[i]);
            if (i < pidcount - 1) printf(", ");
        }
        printf("\n");
 
        char confirm[4];
        printf("  Confirm? [y/n]: ");
        read_line(confirm, sizeof(confirm));
 
        if (confirm[0] == 'y' || confirm[0] == 'Y') {
            for (int i = 0; i < pidcount; i++) {
                do_kill(pids[i]);
            }
        } else {
            printf("  Aborted.\n");
        }
 
        printf("Press Enter to continue...\n");
        read_line(input, sizeof(input));
    }
 
    exit(0);
}
