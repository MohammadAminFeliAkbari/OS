#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/time.h>

typedef struct
{
    double avg;
    int max;
} Result;

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <num_processes> <input_file>\n", argv[0]);
        return 1;
    }

    int num_processes = atoi(argv[1]);
    const char *filename = argv[2];

    // --- خواندن فایل برای شمردن تعداد اعداد ---
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening file");
        return 1;
    }

    int value, total_numbers = 0;
    while (fscanf(file, "%d", &value) == 1)
    {
        total_numbers++;
    }
    rewind(file); // برگرد به ابتدای فایل

    // --- خواندن دوباره اعداد ---
    int *numbers = malloc(total_numbers * sizeof(int));
    for (int i = 0; i < total_numbers; i++)
    {
        fscanf(file, "%d", &numbers[i]);
    }
    fclose(file);

    // --- ساخت Shared Memory ---
    int shmid = shmget(IPC_PRIVATE, num_processes * sizeof(Result), IPC_CREAT | 0666);
    Result *results = (Result *)shmat(shmid, NULL, 0);

    // --- ساخت Shared Memory دوم برای داده‌ها (اختیاری ولی مفید) ---
    int shmid_data = shmget(IPC_PRIVATE, total_numbers * sizeof(int), IPC_CREAT | 0666);
    int *shared_numbers = (int *)shmat(shmid_data, NULL, 0);
    for (int i = 0; i < total_numbers; i++)
        shared_numbers[i] = numbers[i];

    int chunk_size = total_numbers / num_processes;

    struct timeval start, end;
    gettimeofday(&start, NULL);

    // --- ایجاد فرزندها ---
    for (int i = 0; i < num_processes; i++)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            int start_index = i * chunk_size;
            int end_index = (i == num_processes - 1) ? total_numbers : (i + 1) * chunk_size;

            char str_start[10], str_end[10], str_shmid[20], str_i[10], str_shdata[20];
            sprintf(str_start, "%d", start_index);
            sprintf(str_end, "%d", end_index);
            sprintf(str_shmid, "%d", shmid);
            sprintf(str_shdata, "%d", shmid_data);
            sprintf(str_i, "%d", i);

            execl("./child", "./child", str_start, str_end, str_shmid, str_i, str_shdata, NULL);
            perror("execl failed");
            exit(1);
        }
    }

    // --- صبر برای تمام فرزندها ---
    while (wait(NULL) > 0)
        ;

    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    // --- ترکیب نتایج ---
    double global_avg = 0;
    int global_max = -999999;
    for (int i = 0; i < num_processes; i++)
    {

        int start_index = i * chunk_size;
        int end_index = (i == num_processes - 1) ? total_numbers : (i + 1) * chunk_size;

        global_avg += results[i].avg * (end_index - start_index);
        if (results[i].max > global_max)
        {
            global_max = results[i].max;
        }
    }

    global_avg /= total_numbers;

    printf("=== Final Results ===\n");
    printf("Total Numbers: %d\n", total_numbers);
    printf("Global Average: %.2f\n", global_avg);
    printf("Global Maximum: %d\n", global_max);
    printf("Execution Time: %.3f seconds\n", elapsed);
    printf("=====================\n");

    shmdt(results);
    shmdt(shared_numbers);
    shmctl(shmid, IPC_RMID, NULL);
    shmctl(shmid_data, IPC_RMID, NULL);
    free(numbers);
    return 0;
}
