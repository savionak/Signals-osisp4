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

/* Receivers numbers:
 *
 * 0 = none
 * positive = pid
 * negative = processes group
 *
 * f/ex: "-x" means, that signal is sent to the group, process with pid = x is member of
 */
const char RECEIVER[] =
    { 0, -6, 1, 2, -3, 0, 4, 4, 4 };
/*    0,  1, 2, 3,  4, 5, 6, 7, 8  */


char *exec_name = NULL;
void print_error(const char *s_name, const char *msg, const int proc_num);

int proc_id = 0;
void forker(int curr_number, int childs_count);

char *pids_list_path = "/tmp/pids.log";
int pids_list = NULL;

void wait_for_children();


int main(int argc, char *argv[])
{
    exec_name = basename(argv[0]);

    if ( (pids_list = open(pids_list_path, O_RDWR | O_CREAT | O_TRUNC) ) == -1) {
        print_error(exec_name, "Can't open temp file", 0);
        exit(1);
    }

    mmap(pids_list, (PROC_COUNT+1)*sizeof(pid_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    int i = 0;
    pid_t p = 0;

    lseek(pids_list, 0, SEEK_SET);                      // initialize pids list with zeros [1]
    for (i = 0; i < PROC_COUNT; ++i) {
        write(pids_list, &p, sizeof(pid_t));
    }


    forker(0, CHILDS_COUNT[0]);                         // create processes tree [2]


    if (proc_id == 0) {                                 // main process waits [0]
        wait_for_children();
        munmap(pids_list, (PROC_COUNT+1)*sizeof(pid_t));
        return 0;
    }

    on_exit(&wait_for_children, NULL);


    p = getpid();                                       // save pid to list
    lseek(pids_list, proc_id*sizeof(pid_t), SEEK_SET);
    write(pids_list, &p, sizeof(pid_t));

#ifdef DEBUG
    printf("%d\tpid: %d\tppid: %d\tpgid: %d\n", proc_id, getpid(), getppid(), getpgid(0));
#endif

//  =======================================================================================================

    if (proc_id == STARTING_PROC_ID) {                  // starter waits for all pids available [3]
        do{
            p = 1;
            lseek(pids_list, sizeof(pid_t), SEEK_SET);

            for (i = 0; (i < PROC_COUNT) && (p != 0); ++i) {
                read(pids_list, &p, sizeof(pid_t));

                if (p == -1) {
                    print_error(exec_name, "Error: not all processes forked or initialized.\nHalting.", 0);
                    exit(1);
                }
            }
//            printf("%d\t", i);
        } while (p == 0);

#ifdef DEBUG
        printf("All pids are set!\n");
//        int times = 0;
#endif

/*
        // <------------------ set starter handler here (may be the same as usual handler)

        p = 0;
        lseek(pids_list, STARTING_PROC_ID*sizeof(pid_t), SEEK_SET);   // handler is set
        write(pids_list, &p, sizeof(pid_t));


        do{                                             // starter waits for all handlers to be set [6]
#ifdef DEBUG
        ++times;
#endif
            p = 0;
            lseek(pids_list, sizeof(pid_t), SEEK_SET);
            for (i = 0; (i < PROC_COUNT) && (p == 0); ++i) {
                read(pids_list, &p, sizeof(pid_t));
            }
            printf(">%d\n", i);
        } while (p != 0);

#ifdef DEBUG
        printf("Ready to start signals-flow after %d rotations\n", times);
#endif

        // <<=========   start signal ping-pong here [7]
*/
        return 0;
    }

// other processes

    do {                                // wait for all pids to be written
        lseek(pids_list, 0, SEEK_SET);
        read(pids_list, &p, sizeof(pid_t));
    } while (p == 0);

    // <---------------------- set handler here
/*
    p = 0;
    lseek(pids_list, (proc_id-1)*sizeof(pid_t), SEEK_SET);  // reset pid - notify, that handler is set [5]
    write(pids_list, &p, sizeof(pid_t));
*/

//    pause();

    close (pids_list);
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

    int err = -1;
    lseek(pids_list, proc_num*sizeof(pid_t), SEEK_SET);
    write(pids_list, &err, sizeof(pid_t));

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
