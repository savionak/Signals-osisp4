#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>

#define MAX_ERR_LENGTH (100)
#define PROCESS_COUNT (9)
#define MAX_CHILDS_COUNT (3)

const int CHILDS_COUNT[PROCESS_COUNT] =
    {1, 3, 2, 0, 0, 0, 1, 1, 0};
/*   0  1  2  3  4  5  6  7  8  */

const int CHILDS_NUMBERS[PROCESS_COUNT][MAX_CHILDS_COUNT] =
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

char *exec_name = NULL;
void print_error(const char *s_name, const char *msg, const char *f_name);

void forker(int curr_number, int childs_count);


int main(int argc, char *argv[])
{
    exec_name = basename(argv[0]);

    forker(0, CHILDS_COUNT[0]);

    pause();

    return 0;
}   /*  main    */


void print_error(const char *s_name, const char *msg, const char *f_name) {
    fprintf(stderr, "%s: %s %s\n", s_name, msg, (f_name)? f_name : "");
}   /*  print_error(const char *, const char *, const char *) */


void handler(int signum) {

}   /*  handler(int) */


void forker(int curr_number, int childs_count) {

    pid_t pid = 0;
    int i = 0;

    for (i = 0; i < childs_count; ++i) {

        if ( (pid = fork() ) == -1) {
            char *err_text = calloc(sizeof(char), MAX_ERR_LENGTH);
            sprintf(err_text, "Error while forking %d child of %d process", i+1, curr_number);
            print_error(exec_name, err_text, NULL);
            exit(1);

        } else if (pid == 0) {           // child
            curr_number = CHILDS_NUMBERS[curr_number][i];
            printf("%d process initiated!\n", curr_number);

            if (CHILDS_COUNT[curr_number] != 0)
                forker(curr_number, CHILDS_COUNT[curr_number]);
            break;

        } else {    // pid != 0 (=> parent)

            // need to save child's pid somewhere
        }
    }

    // here set signal handler

}   /*  forker(int, int)  */
