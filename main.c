#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <math.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>
#include <time.h>

int *readFile(int *count, const char *fileName)
{
    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }

    char firstLine[256];
    if (fgets(firstLine, sizeof(firstLine), file) == NULL)
    {
        perror("Error reading first line");
        fclose(file);
        return NULL;
    }

    int maxLength = atoi(firstLine);
    int *numbers = malloc(maxLength * sizeof(int));
    if (!numbers)
    {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    char buffer[256];
    *count = 0;
    while (fgets(buffer, sizeof(buffer), file) != NULL && *count < maxLength)
    {
        numbers[*count] = atoi(buffer);
        (*count)++;
    }

    fclose(file);
    return numbers;
}

void writer(const char *text)
{
    key_t key = 1234;
    int shmid = shmget(key, 1024, IPC_CREAT | 0666);
    if (shmid < 0)
    {
        perror("shmget failed");
        exit(1);
    }

    char *shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (char *)-1)
    {
        perror("shmat failed");
        exit(1);
    }

    strcpy(shared_memory, text);
    shmdt(shared_memory);
}

char *reader()
{
    key_t key = 1234;
    int shmid = shmget(key, 1024, 0666);
    if (shmid < 0)
    {
        perror("shmget failed");
        exit(1);
    }

    char *shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (char *)-1)
    {
        perror("shmat failed");
        exit(1);
    }

    char *result = strdup(shared_memory);
    shmdt(shared_memory);
    return result;
}

char *getSection(int sectionType)
{
    char *str = reader();
    const char delimiter[] = "-";
    char *token = strtok(str, delimiter);
    int sectionInFunction = 1;
    char *result = NULL;

    while (token != NULL)
    {
        if (sectionInFunction == sectionType)
        {
            result = strdup(token);
            break;
        }
        token = strtok(NULL, delimiter);
        sectionInFunction++;
    }

    free(str);
    return result;
}

void setSharedMemory(int section, const char *newStr)
{
    char *str = reader();
    const char delimiter[] = "-";
    char *token = strtok(str, delimiter);
    char newString[1024] = "";
    int sectionCount = 1;

    while (token != NULL)
    {
        if (sectionCount == section)
            strcat(newString, newStr);
        else
            strcat(newString, token);

        token = strtok(NULL, delimiter);
        if (token != NULL)
            strcat(newString, "-");

        sectionCount++;
    }

    writer(newString);
    free(str);
}

void initialWrite(int numberOfFork, const char *fileName)
{
    int count = 0;
    int *numbers = readFile(&count, fileName);
    if (!numbers)
        return;

    int x = ceil((double)count / numberOfFork);
    char buffer[1024] = "";

    for (int i = 0; i < numberOfFork; i++)
    {
        char str[256] = "";
        for (int j = 0; j < x; j++)
        {
            int y = x * i + j;
            if (y < count)
            {
                char temp[16];
                sprintf(temp, "%d,", numbers[y]);
                strcat(str, temp);
            }
        }
        strcat(buffer, str);
        strcat(buffer, "-");
    }

    printf("first Initial : %s\n", buffer);
    writer(buffer);
    free(numbers);
}

double calculateMid(char *str)
{
    char *token = strtok(str, ",");
    int sum = 0, count = 0;
    while (token)
    {
        sum += atoi(token);
        count++;
        token = strtok(NULL, ",");
    }
    return count ? (double)sum / count : 0;
}

double calculateMax(const char *str)
{
    char temp[strlen(str) + 1];
    strcpy(temp, str);
    char *token = strtok(temp, ",");
    int max = INT_MIN;
    while (token)
    {
        int val = atoi(token);
        if (val > max)
            max = val;
        token = strtok(NULL, ",");
    }
    return max == INT_MIN ? 0 : max;
}

void writeMaxMid(int numberOfFork)
{
    char *section = getSection(numberOfFork);
    if (!section)
        return;

    double average = calculateMid(strdup(section));
    double max = calculateMax(section);

    char result[128];
    sprintf(result, "%.2f,%.2f", average, max);

    printf("in fork %d → %s\n", numberOfFork, result);
    setSharedMemory(numberOfFork, result);
    free(section);
}

void calculateAverageAndMax()
{
    char *input = reader();
    double average = 0;
    double max = INT_MIN;

    char buffer[1024];
    strcpy(buffer, input); // چون strtok رشته اصلی رو تغییر میده

    const char *section_delim = "-";
    char *section = strtok(buffer, section_delim);

    double leftSum = 0;
    int leftCount = 0;

    while (section != NULL)
    {
        // جدا کردن بخش چپ و راست با ','
        char *comma = strchr(section, ',');
        if (comma != NULL)
        {
            *comma = '\0'; // جدا کردن بخش چپ و راست
            double left = atof(section);
            double right = atof(comma + 1);

            leftSum += left;
            leftCount++;

            if (right > max)
                max = right;
        }

        section = strtok(NULL, section_delim);
    }

    if (leftCount > 0)
        average = leftSum / leftCount;
    else
        average = 0;

    printf("%f , %f\n", average, max);
}

double get_time_ms() // 👈 بیار بالا
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(int argc, char const *argv[])
{
    double start_time = get_time_ms(); // ✅ شروع تایم
    int numberOfFork = atoi(argv[1]);
    const char *filename = argv[2];
    const char *x = argv[3];

    if (!x)
        initialWrite(numberOfFork, filename);

    if (numberOfFork == 0)
        return 0;

    if (numberOfFork == 1)
    {
        printf("→ Base case (1): parent\n");
        writeMaxMid(numberOfFork);
        if (!x)
        {
            printf("Final reader: %s\n", reader());
            double end_time = get_time_ms(); // ✅ پایان تایم
            printf("⏱ Execution time: %.3f ms\n", end_time - start_time);
        }
        return 0;
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        printf("→ Child process for fork %d\n", numberOfFork);
        writeMaxMid(numberOfFork);

        char str_int[20];
        int newNumber = numberOfFork - 2;
        sprintf(str_int, "%d", newNumber);

        execl("./main", "./main", str_int, filename, "x", NULL);
        fprintf(stderr, "execl failed: %s\n", strerror(errno));
        exit(1);
    }
    else if (pid > 0)
    {
        printf("→ Parent process for fork %d\n", numberOfFork);
        writeMaxMid(numberOfFork - 1);
        wait(NULL);
        printf("→ Finished all forks.\n");
    }
    else
    {
        perror("fork failed");
        exit(1);
    }

    if (!x)
        calculateAverageAndMax();

    double end_time = get_time_ms(); // ✅ پایان تایم
    printf("⏱ Execution time: %.3f ms\n", end_time - start_time);
    return 0;
}