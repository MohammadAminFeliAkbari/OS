#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>

typedef struct
{
    double avg;
    int max;
} Result;

int main(int argc, char *argv[])
{
    if (argc < 6)
    {
        fprintf(stderr, "Usage: child <start> <end> <shmid> <index> <shdata>\n");
        return 1;
    }

    int start = atoi(argv[1]);
    int end = atoi(argv[2]);
    int shmid = atoi(argv[3]);
    int index = atoi(argv[4]);
    int shmid_data = atoi(argv[5]);

    Result *results = (Result *)shmat(shmid, NULL, 0);
    int *numbers = (int *)shmat(shmid_data, NULL, 0);

    int sum = 0;
    int max = -999999;
    for (int i = start; i < end; i++)
    {
        sum += numbers[i];
        if (numbers[i] > max)
            max = numbers[i];
    }

    results[index].avg = (double)sum / (end - start);
    results[index].max = max;

    shmdt(results);
    shmdt(numbers);
    return 0;
}
