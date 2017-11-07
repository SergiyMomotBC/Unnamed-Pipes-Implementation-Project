#include "unnamed_pipes.h"

//Linux and glibc libraries
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>

//current pipe_list capacity; initially 4
static int alloc_elems = 4;

//struct that is used to store information about pipes open by pipe_open()
struct pipe_list {
    int fd;
    int pid;
} *open_pipes;

//popen() implementation
FILE* pipe_open(const char* command, const char* type)
{
    int fds[2], child_pipe_id, child_pid, available_slot;

    //check if type argument is correct
    if(type[0] != 'r' && type[0] != 'w') {
        errno = EINVAL;
        return NULL;
    }

    //manage stored pipes information
    if(open_pipes != NULL) {
        //looking for the first available slot to save FD
        //available slot has 'fd' member set to -1
        int i;
        for(i = 0; open_pipes[i].fd != -1 && i < alloc_elems; i++);

        //more memory required to store new records
        if(i == alloc_elems) {
            //save in case realloc() fails
            struct pipe_list* temp = open_pipes;

            open_pipes = (struct pipe_list*) realloc(open_pipes, alloc_elems * 2 * sizeof(struct pipe_list));

            //realloc failed; user should pipe_close() before calling pipe_open() again
            //errno is set by realloc()
            if(open_pipes == NULL) {
                open_pipes = temp;
                return NULL;
            }

            //make new records available
            for(int j = alloc_elems; j < alloc_elems * 2; j++)
                open_pipes[j].fd = -1;

            alloc_elems *= 2;
        }

        //use found slot
        available_slot = i;

    } else {
        //allocate memory for opened pipes list and make all available
        open_pipes = (struct pipe_list*) malloc(alloc_elems * sizeof(struct pipe_list));

        //malloc failed; errno is set by malloc()
        if(open_pipes == NULL)
            return NULL;

        //make all slots available
        for(int i = 0; i < alloc_elems; i++)
            open_pipes[i].fd = -1;

        //use first slot
        available_slot = 0;
    }

    //open an undirectional pipe; on error, errno is set by pipe()
    if(pipe(fds) == -1)
        return NULL;

    //decide which pipe's file descriptor will be used by the child
    child_pipe_id = (type[0] == 'w') ? 0 : 1;

    //forking
    if((child_pid = fork()) == 0) {
        //redirect pipe fd to either stdin or stdout depending on type argument
        dup2(fds[child_pipe_id], child_pipe_id);

        //not needed any more
        close(fds[0]);
        close(fds[1]);

        //pass command to the standard shell
        execl("/bin/sh", "sh", "-c", command, NULL);

        //shell is unavailable or command is not recognized
        exit(EXIT_FAILURE);

    } else if(child_pid > 0) {
        //save pipe information
        open_pipes[available_slot].fd = fds[!child_pipe_id];
        open_pipes[available_slot].pid = child_pid;

        //close unused pipe and return a FILE* pointer to the one that is used
        close(fds[child_pipe_id]);
        return fdopen(fds[!child_pipe_id], type);

    } else {
        //fork failed; fork errno should be set before return
        int tmp = errno;
        close(fds[0]);
        close(fds[1]);
        errno = tmp;
        return NULL;
    }
}

//pclose() implementation
int pipe_close(FILE* stream)
{
    //pipe_open() was not called before a call to pipe_close()
    if(open_pipes == NULL) {
        errno = EINVAL;
        return -1;
    }

    int pid = -1, fd = fileno(stream);

    //find specified pipe info by the file descriptor
    for(int i = 0; i < alloc_elems; i++)
        if(open_pipes[i].fd == fd) {
            pid = open_pipes[i].pid;
            open_pipes[i].fd = -1;
            break;
        }

    //this FILE* is not associated with a pipe open with pipe_open()
    if(pid == -1) {
        errno = EINVAL;
        return -1;
    }

    //close pipe
    fclose(stream);

    //wait until the child is done; on error, errno is set by waitpid()
    return waitpid(pid, NULL, 0) > 0 ? 0 : -1;
}
