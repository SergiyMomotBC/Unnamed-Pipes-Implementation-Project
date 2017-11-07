/*
 *  Written by Sergiy Momot.
 *
 *  Custom implementation of popen() and pclose() functions from stdio.h C library.
 *
 *  pipe_open() function creates an unnamed pipe which is connected to a process
 *      specified by the command argument. pipe_open() allows many simultanious
 *      pipes to be opened until child process limit or open file limit is reached or there is no memory
 *      to store information about open pipes.
 *
 *  pipe_close() function closes the pipe previously open with pipe_open()
 *      and waits for a related child process to finish.
 *
 *  CISC 3350 Workstation programming
 *  Brooklyn College of CUNY, Spring 2017
 */

#ifndef _UNNAMED_PIPES_H_
#define _UNNAMED_PIPES_H_

#include <stdio.h>

//starts a new process and creates an unnamed pipe to communicate with it
FILE* pipe_open(const char* command, const char* type);

//closes previously open pipe and waits for the associated child process
int pipe_close(FILE* stream);

#endif
