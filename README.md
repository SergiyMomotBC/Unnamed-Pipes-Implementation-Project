# Unnamed Pipes Implementation

Custom implementation of popen() and pclose() functions from stdio.h C library.

pipe_open() function creates an unnamed pipe which is connected to a process specified by the command argument. 
pipe_open() allows many simultanious pipes to be opened until child process limit or open file limit is reached or there is no memory to store information about open pipes.

pipe_close() function closes the pipe previously open with pipe_open() and waits for a related child process to finish.
