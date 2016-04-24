#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <wait.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define PROC_COUNT (9)
#define STARTING_PROC_ID (1)
#define MAX_CHILDS_COUNT (3)

#define DEBUG

const unsigned char CHILDS_COUNT[PROC_COUNT] =
    { 1, 3, 2, 0, 0, 0, 1, 1, 0 };
/*    0  1  2  3  4  5  6  7  8  */

const unsigned char CHILDS_IDS[PROC_COUNT][MAX_CHILDS_COUNT] =
{
    {1},        /* 0 */
    {2, 3, 4},  /* 1 */
    {5, 6},     /* 2 */
    {0},        /* 3 */
    {0},        /* 4 */
    {0},        /* 5 */
    {7},        /* 6 */
    {8},        /* 7 */
    {0}         /* 8 */
};

/* Group types:
 *
 * 0 = pid;
 * 1 = parent's pgid
 * 2 = previous child group
 */
const unsigned char GROUP_TYPE[] =
    { 0, 1, 0, 2, 0, 0, 0, 1, 1 };
/*    0  1  2  3  4  5  6  7  8  */

/* Whome to send signal:
 *
 * 0 = none
 * positive = pid
 * negative = processes group
 *
 * f/ex: "-x" means, that signal is sent to the group, process with pid = x is member of
 */
const char NEXT_RECEIVER[] =
    { 0, -6, 1, 2, -3, 0, 4, 4, 4 };
/*    0,  1, 2, 3,  4, 5, 6, 7, 8  */

const int SIGNALS[] =
{
    0,          /* 0 */
    SIGUSR1,    /* 1 */
    SIGUSR2,    /* 2 */
    0,          /* 3 */
    SIGUSR1,    /* 4 */
    0,          /* 5 */
    SIGUSR1,    /* 6 */
    SIGUSR2,    /* 7 */
    SIGUSR1     /* 8 */
};

const char SIGNALS_COUNT[] =
    { 0, 0, 1, 1, 3, 0, 1, 1, 1 };
/*    0, 1, 2, 3, 4, 5, 6, 7, 8  */


char *exec_name = NULL;
void print_error(const char *s_name, const char *msg, const int proc_num);

int proc_id = 0;
void forker(int curr_number, int childs_count);

char *pids_list_path = "/tmp/pids.log";

void wait_for_children();
pid_t *pids_list = NULL;

int main(int argc, char *argv[])
{
    exec_name = basename(argv[0]);

    pids_list = (pid_t*)mmap(pids_list, (PROC_COUNT+1)*sizeof(pid_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    int i = 0;
                                                        // initialize pids list with zeros [1]
    for (i = 0; i <= PROC_COUNT; ++i) {
        pids_list[i] = 0;
    }


    forker(0, CHILDS_COUNT[0]);                         // create processes tree [2]


    if (proc_id == 0) {                                 // main process waits [0]
        wait_for_children();
        munmap(pids_list, (PROC_COUNT+1)*sizeof(pid_t));
        return 0;
    }

    on_exit(&wait_for_children, NULL);

    pids_list[proc_id] = getpid();          // save pid to list

#ifdef DEBUG
    printf("%d\tpid: %d\tppid: %d\tpgid: %d\n", proc_id, getpid(), getppid(), getpgid(0));
#endif

//  =======================================================================================================

    if (proc_id == STARTING_PROC_ID) {                  // starter waits for all pids available [3]
        do{
            for (i = 1; (i <= PROC_COUNT) && (pids_list[i] != 0); ++i) {

                if (pids_list[i] == -1) {
                    print_error(exec_name, "Error: not all processes forked or initialized.\nHalting.", 0);
                    exit(1);
                }
            }
//            printf("%d\t", i);
        } while (i < PROC_COUNT);

#ifdef DEBUG
        printf("All pids are set!\n");

        for (i = 0; (i < PROC_COUNT); ++i) {
            printf("%d - %d\n", i, pids_list[i]);
        }

        int times = 0;
#endif

        pids_list[0] = 1;           // all pids are set

        // <------------------ set starter handler here (may be the same as usual handler)

        pids_list[STARTING_PROC_ID] = 0;                // handler is set

        do{                                             // starter waits for all handlers to be set [6]
#ifdef DEBUG
        ++times;
#endif
            for (i = 1; (i < PROC_COUNT) && (pids_list[i] == 0); ++i) {
            }
//            printf(">%d\n", i);
        } while (pids_list[i] != 0);

        #ifdef DEBUG
                printf("Ready to start signals-flow after %d rotations\n", times);

                for (i = 0; (i < PROC_COUNT); ++i) {
                    printf("%d - %d\n", i, pids_list[i]);
                }
        #endif
        // <<=========   start signal ping-pong here [7]

        return 0;
    }

// other processes

    do {
        // wait for all pids to be written
    } while (pids_list[0] == 0);

    // <---------------------- set handler here

    pids_list[proc_id] = 0;             // notify starting_proc, that handler is set

/*
    while (1) {
        pause();  // start cycle
    }
*/

    return 0;
}   /*  main   */


void wait_for_children() {

    // add sending SIGINT to all children before waiting for them

    int i = CHILDS_COUNT[proc_id];
    while (i > 0) {
        wait(NULL);
        --i;
    }
}   /*  wait_for_children  */


void print_error(const char *s_name, const char *msg, const int proc_num) {
    fprintf(stderr, "%s: %s %d\n", s_name, msg, proc_num);

    pids_list[proc_num] = -1;

    exit(1);
}   /*  print_error */


void handler(int signum) {

}   /*  handler  */


void forker(int curr_number, int childs_count) {

    pid_t pid = 0;
    int i = 0;

    for (i = 0; i < childs_count; ++i) {

        int chld_id = CHILDS_IDS[curr_number][i];
        if ( (pid = fork() ) == -1) {

            print_error(exec_name, "Can't fork!", chld_id);

        } else if (pid == 0) {  /*  child    */
            proc_id = chld_id;

            if (CHILDS_COUNT[proc_id] != 0) {
                forker(proc_id, CHILDS_COUNT[proc_id]);         // fork children
            }

            break;

        } else {    // pid != 0 (=> parent)
            static int prev_chld_grp = 0;

            int grp_type = GROUP_TYPE[chld_id];

            if (grp_type == 0) {
                if (setpgid(pid, pid) == -1) {
                    print_error(exec_name, "Can't set group", chld_id);
                } else {
                    prev_chld_grp = pid;
                }

            } else if (grp_type == 1) {
                if (setpgid(pid, getpgid(0)) == -1) {
                    print_error(exec_name, "Can't set group", chld_id);
                }
            } else if (grp_type == 2) {
                if (setpgid(pid, prev_chld_grp) == -1) {
                    print_error(exec_name, "Can't set group", chld_id);
                }
            }

        }   // parent branch

    }   // for

}   /*  forker  */
