/*
 *  Written by Sergiy Momot.
 *
 *  The 'runsim' utility creates and manages a group of processes
 *  that are running in parallel using fork(), exec*() and wait*() system calls.
 *
 *  CISC 3350 Workstation programming
 *  Brooklyn College of CUNY, Spring 2017
 */

//Linux libraries
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>

//c libraries
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <time.h>

void check_for_quotes(char** arguments);

//child returns this code if exec failed
#define EXEC_ERROR_CODE 100

//waits for a specified number of children
//returns the number of actually finished children
unsigned long wait_for_children(unsigned long count, int options);

//returns a NULL-terminated array of arguments from the given line
char** get_arguments(const char* line);

//statistics of submitted tasks' execution
struct {
    //all submitted tasks
    unsigned long long all_count;

    //tasks that returned or exited with EXIT_SUCCESS
    unsigned long long success_count;

    //tasks that returned or exited with EXIT_FAILURE
    unsigned long long failure_count;

    //tasks that were skipped due to fork() or exec*() failures
    unsigned long long skipped_count;
} task_stats;

int main(int argc, char** argv)
{
    //maximum simultanesously running child precesses allowed
    unsigned long child_limit;

    //currently running child processed
    unsigned long active_children = 0;

    //maximum length of a line to read
    unsigned long max_line_length = ULONG_MAX - 1;

    //execution start time stamp
    time_t start_time = time(NULL);

    //get the maximum number of children a process can have on a given system
    struct rlimit system_child_limit;
    getrlimit(RLIMIT_NPROC, &system_child_limit);

    //check if the limit argument was specified and is it correct
    if(argc < 2) {
        printf("No child processes limit parameter.\n");
        exit(EXIT_FAILURE);
    } else {
        //will return 0 if not a number so no error check needed
        child_limit = atoi(1[argv]);

        //check if achild limit rgument is valid
        if(child_limit < 1 || child_limit > system_child_limit.rlim_cur) {
            printf("Limit parameter is incorrect: it should be in range [1, %lu].\n", system_child_limit.rlim_cur);
            exit(EXIT_FAILURE);
        }
    }

    //storage for a line read from STDIN
    char* line = (char*) malloc(sizeof(char) * (max_line_length + 1));

    printf("\nStarting executing tasks:\n\n");

    //main loop
    while(1)
    {
        if(active_children < child_limit)
        {
            //read a line from standard input
            int read_count = getline(&line, &max_line_length, stdin);

            //quit the loop if EOF or terminate if reading error
            if(read_count == -1)
                if(!feof(stdin)) {
                    perror("Error reading from standard input");
                    exit(EXIT_FAILURE);
                } else
                    break;

            //since getline() includes \n character the read line is empty
            if(read_count == 1)
                continue;

            //to stop when using direct keyboard input
            if(read_count == 2 && line[0] == 'q')
                break;

            //make a string with current time formatted
            time_t current_time = time(NULL);
            char time_string[12];
            strftime(time_string, 12, "%l:%M:%S %p", localtime(&current_time));

            //there is a new process to run
            task_stats.all_count++;

            printf("%7llu. %s - Starting a new process: %s", task_stats.all_count, time_string, line);

            //forking a child
            pid_t child_pid = fork();

            if(child_pid == 0) {
                char** arguments = get_arguments(line);

                //first try pass process name as path
                //if 'no such file or directory' error then try filename version of exec
                execv(arguments[0], arguments);
                if(errno == ENOENT)
                    execvp(0[arguments], arguments);

                //if reached this point then exec failed
                printf("\t*** The following process will be skipped because exec failed: %s\t*** Reason: %s\n", line, strerror(errno));

                //when stdin is redirected to a file the weird error happens
                //if fclose() is not called: stdin repeates itself forever
                //even though exit() is called from a child process
                fclose(stdin);

                //forked child should terminate with defined error code
                exit(EXEC_ERROR_CODE);

            } else if(child_pid > 0)
                //fork succeeded
                active_children++;
            else
                //fork failed
                printf("\t*** The following process will be skipped because fork failed: %s\t*** Reason: %s\n", line, strerror(errno));

            //check if any of active children had exited without hanging
            active_children -= wait_for_children(active_children, WNOHANG);
        }
        else
            //wait until one child process terminates
            //parameters 1 and 0 (see prototype) make this function to act like wait()
            active_children -= wait_for_children(1, 0);
    }

    //not necessary but adds to the portability
    free(line);

    //wait for all remaining active children
    active_children -= wait_for_children(active_children, 0);

    printf("\nFinished executing tasks.\n\n");

    //prints executed processes summary
    printf("\nExecuted processes statistics:\n");
    printf("\t* Completed with success code\t %llu\n", task_stats.success_count);
    printf("\t* Completed with failure code\t %llu\n", task_stats.failure_count);
    printf("\t* Could not be started\t\t %llu\n", task_stats.skipped_count);
    printf("\t* Submitted processes\t\t %llu\n\n", task_stats.all_count);

    //time from starting of this process till last child had finished
    //only valid if stdin is redirected to a file
    printf("Total execution time: %d secods.\n\n", (int) difftime(time(NULL), start_time));

    //something went really wrong if there are still active children at this point
    if(active_children == 0)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}

unsigned long wait_for_children(unsigned long count, int options)
{
    //child's return status
    int status;

    //children that terminated
    unsigned long finished_count = 0;

    for(int i = 0; i < count; i++) {
        //wait for any child of the central parent process
        //there is no need for a regular wait(int* status) call
        //since waitpid call with -1 for pid and 0 for options behaves the same
        int pid = waitpid(-1, &status, options);

        if(pid == -1) {
            perror("waitpid failed");
            exit(EXIT_FAILURE);
        }

        //child finished
        if(pid > 0)
        {
            finished_count++;

            //determine child's exit status
            if(WIFEXITED(status))
                if(WEXITSTATUS(status) == EXEC_ERROR_CODE)
                    task_stats.skipped_count++;
                else if(WEXITSTATUS(status) == EXIT_FAILURE)
                    task_stats.failure_count++;
                else if(WEXITSTATUS(status) == EXIT_SUCCESS)
                    task_stats.success_count++;
        }

        //only if WNOHANG option: zero indicates that no child finished at the moment
        //no need to continue checking the remaining children since -1 was passed to waitpid()
        if(pid == 0)
            break;
    }

    return finished_count;
}

char** get_arguments(const char* line)
{
    const int MAX_ARGC = 1024;

    //space as a delimeter
    const char* delim = " ";

    //allocated space for maximum number of arguments, +1 for NULL
    char** arguments = (char**) malloc(sizeof(char*) * (MAX_ARGC + 1));

    //arguments count
    unsigned argc = 0;

    //get the first token (parameter)
    //line is duplicated because strtok() takes a mutable string
    char* arg = strtok(strndup(line, strlen(line) - 1), delim);

    //read the rest of arguments
    while(arg != NULL && argc < MAX_ARGC) {
        arguments[argc++] = arg;
        arg = strtok(NULL, delim);
    }

    //put a NULL pointer as the last argument
    arguments[argc++] = NULL;

    //shrink allocated memory to the actual size of an array before returning
    return (char**) realloc(arguments, sizeof(char*) * argc);
}
