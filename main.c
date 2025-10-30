#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <math.h>

// int makeFork(int numberOfFork)
// {
//     int x = 1;
//     int numberOfLoopForMakeFork = 0;
//     for (size_t i = 0; i < 20; i++)
//     {
//         if (x >= numberOfFork)
//         {
//             numberOfLoopForMakeFork = i;
//             break;
//         }
//         x *= 2;
//     }

//     for (size_t i = 0; i < numberOfLoopForMakeFork; i++)
//         fork();

//     return numberOfLoopForMakeFork;
// }

// char *makeOne(int numberOfFork)
// {
//     char *str = malloc((numberOfFork + 1) * sizeof(char)); // Allocate memory for the string

//     for (size_t i = 0; i < numberOfFork; i++)
//     {
//         str[i] = '1';
//     }

//     str[numberOfFork] = '\0';

//     return str;
// }

// int findFirstOne()
// {
//     char *token = getSection(2);
//     char *pos = strchr(token, '1');

//     if (pos != NULL)
//     {
//         size_t index = pos - token;
//         return index;
//     }
//     else
//     {
//         printf("Character '1' not found in the string.\n");
//     }
// }

// void changeStatus()
{
    char *token = getSection(1);

    printf("%s\n", token);
}
// ------------------------------------------------
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

    // اختصاص حافظه دقیقاً به اندازه تعداد اعداد
    int *numbers = malloc(maxLength * sizeof(int));
    if (numbers == NULL)
    {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    char buffer[256];
    *count = 0;

    // خواندن اعداد از خطوط بعدی
    while (fgets(buffer, sizeof(buffer), file) != NULL && *count < maxLength)
    {
        numbers[*count] = atoi(buffer);
        (*count)++;
    }

    fclose(file);
    return numbers;
}

void writer(char *text)
{
    key_t key = 1234;
    int shmid;
    char *shared_memory;
    shmid = shmget(key, 1024, IPC_CREAT | 0666);
    if (shmid < 0)
    {
        perror("shmget failed");
        exit(1);
    }

    shared_memory = (char *)shmat(shmid, NULL, 0);
    if (shared_memory == (char *)-1)
    {
        perror("shmat failed");
        exit(1);
    }

    strcpy(shared_memory, text);
    printf("Written: '%s'\n", shared_memory);

    shmdt(shared_memory);
}

char *reader()
{
    key_t key = 1234;
    int shmid;
    char *shared_memory;

    shmid = shmget(key, 1024, 0666);
    if (shmid < 0)
    {
        perror("shmget failed");
        exit(1);
    }

    shared_memory = (char *)shmat(shmid, NULL, 0);
    if (shared_memory == (char *)-1)
    {
        perror("shmat failed");
        exit(1);
    }

    // Copy the data before detaching
    char *result = strdup(shared_memory); // ✅ duplicate string into heap memory

    shmdt(shared_memory); // detach safely
    // shmctl(shmid, IPC_RMID, NULL); // delete shared memory

    return result; // return safe copy
}

void setSharedMemory(int section, char *newStr)
{
    char *str = reader();                 // رشته اصلی
    const char delimiter[] = "-";         // جداکننده‌ها
    char *token = strtok(str, delimiter); // اولین قسمت را جدا می‌کنیم

    // یک متغیر برای ساخت رشته جدید
    char newString[256] = "";

    // شمارنده برای اینکه مشخص کنیم کدام قسمت را باید تغییر دهیم
    int sectionInFunction = 1;

    // هر قسمت از رشته را جدا کرده و پردازش می‌کنیم
    while (token != NULL)
    {
        if (section == sectionInFunction)
        {
            // وقتی به بخش دوم می‌رسیم، آن را با "13,14" جایگزین می‌کنیم
            strcat(newString, newStr);
        }
        else
        {
            // سایر بخش‌ها را بدون تغییر به رشته جدید اضافه می‌کنیم
            strcat(newString, token);
        }

        // به بخش بعدی می‌رویم
        token = strtok(NULL, delimiter);

        if (token != NULL)
        {
            // به هر بخش جدا شده یک "-" اضافه می‌کنیم
            strcat(newString, "-");
        }

        sectionInFunction++;
    }

    // چاپ رشته جدید
    writer(newString);
}

char *getSection(int seciontType)
{
    char *str = reader();
    const char delimiter[] = "-";
    char *token = strtok(str, delimiter);

    int sectionInFunction = 1;
    char *secondSection = NULL;

    while (token != NULL)
    {
        if (sectionInFunction == seciontType)
        {
            secondSection = strdup(token); // ✅ کپی امن
            break;                         // چون فقط بخش دوم رو می‌خوایم، نیازی به ادامه نیست
        }

        token = strtok(NULL, delimiter);
        sectionInFunction++;
    }

    return secondSection; // ✅ رشته‌ی معتبر در heap
}

void initialWrite(int numberOfFork, char *fileName)
{
    int count = 0;
    int *number = readFile(&count, fileName);

    printf("%s\n", number);
    // char str[256]; // enough space for all parts

    // strcpy(str, "1-");        // start with "1-"
    // strcat(str, numberOfOne); // add your generated string
    // strcat(str, "-");         // add last '-'

    // for (size_t i = 0; i < numberOfFork; i++)
    //     strcat(str, " -");

    writer("H");
}

// void deleteSharedMemory()
// {
//     shmctl()
// }
int main(int argc, char const *argv[])
{
    int numberOfFork = atoi(argv[1]);
    const char *filename = argv[2];

    initialWrite(numberOfFork, filename);

    // printf("%s\n", reader());
    // makeFork(numberOfFork);

    // int rs = findFirstOne();

    // printf("%d", rs);
    // setSharedMemory(2, "00001");

    // printf("%d\n", findFirstOne());
    // changeStatus();

    // makeFork(numberOfFork);

    // char *result = reader();

    // // Don't forget to free dynamically allocated memory
    // free(numberOfOne);
    // free(result);

    // findFirstOne();

    return 0;
}
