//Sergiy Momot
//testing 'unnamed_pipes.h'

#include "unnamed_pipes.h"

#include <string.h>

int main(int argc, char** argv)
{
    const int buf_size = 128;
    char buffer[buf_size];

    //trying to open many pipe simultaniously until size is reached or pipe_open() fails
    //shuf util will generate a random number between 1 and 100
    //pipes are read from and closed

    int size = 10000;

    FILE* arr[10000];
    for(int i = 0; i < size; i++) {
        arr[i] = pipe_open("shuf -i 1-100 -n 1", "r");
        if(arr[i] == NULL) {
            size = i;
            perror("pipe_open failed");
            printf("\nSuccessfully open %d simultanious pipes.\n", size);
            break;
        }
    }

    for(int i = 0; i < size; i++) {
        fgets(buffer, buf_size, arr[i]);
        printf("%d. %s", i + 1, buffer);
        if(pipe_close(arr[i]) == -1)
            perror("close failed");
    }

    //data.txt file contains an unsorted list of country names of world soccer cup winners
    //data is sorted by country's name and multiple entries are removed
    //instead. number of times a specific country won is printed

    FILE* sort_in = pipe_open("sort < data.txt", "r");
    FILE* uniq_out = pipe_open("uniq -c", "w");

    printf("\nNational World Soccer Cup winners:\n");

    while(fgets(buffer, buf_size, sort_in) != NULL)
        fwrite(buffer, strlen(buffer), 1, uniq_out);

    if(pipe_close(sort_in) == -1)
        perror("pipe_close failed");

    if(pipe_close(uniq_out) == -1)
        perror("pipe_close failed");

    return 0;
}
