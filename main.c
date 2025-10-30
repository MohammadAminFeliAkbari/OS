#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <math.h>
#include <sys/wait.h>
#include <limits.h>

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
    // printf("Written: '%s'\n", shared_memory);

    shmdt(shared_memory);
}

void deleteSharedMemory()
{
    key_t key = 1234;
    int shmid;
    char *shared_memory;

    shmid = shmget(key, 1024, 0666);
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

void initialWrite(int numberOfFork, const char *fileName)
{
    int count = 0;
    int *numbers = readFile(&count, fileName);

    if (numbers == NULL)
    {
        printf("Error reading file.\n");
        return;
    }

    int x = ceil((double)count / numberOfFork);
    char buffer[1024] = "";

    for (size_t i = 0; i < numberOfFork; i++)
    {
        char str[256] = "";

        for (size_t j = 0; j < x; j++)
        {
            int y = x * i + j;

            if (y < count)
            {
                char temp[16] = "";
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
    const char delimiter[] = ",";
    char *token = strtok(str, delimiter);
    int sum = 0;
    int count = 0;

    // جدا کردن و پردازش هر عدد
    while (token != NULL)
    {
        sum += atoi(token);              // تبدیل هر بخش به عدد و افزودن به مجموع
        count++;                         // افزایش تعداد اعداد
        token = strtok(NULL, delimiter); // ادامه دادن به بخش بعدی
    }

    // محاسبه میانگین
    if (count > 0)
    {
        float average = (float)sum / count;
        // printf("Average: %.2f\n", average);
        return average;
    }
    else
    {
        printf("No numbers to calculate average\n");
    }
}

double calculateMax(const char *str)
{
    // 🔹 کپی از رشته‌ی اصلی چون strtok رشته را تغییر می‌دهد
    char temp[strlen(str) + 1];
    strcpy(temp, str);

    const char delimiter[] = ",";
    char *token = strtok(temp, delimiter);
    int max = INT_MIN;

    while (token != NULL)
    {
        int currentNum = atoi(token);
        if (currentNum > max)
            max = currentNum;

        token = strtok(NULL, delimiter);
    }

    if (max != INT_MIN)
    {
        // printf("Max: %d\n", max);
        return max;
    }
    else
    {
        printf("No numbers to calculate max\n");
        return -1;
    }
}

void writeMaxMid(int numberOfFork)
{
    double average = calculateMid(getSection(numberOfFork));
    char averageToString[50]; // کافیست یک آرایه کافی برای ذخیره نتیجه ایجاد کنید
    sprintf(averageToString, "%f", average);

    double max = calculateMax(getSection(numberOfFork));
    char maxToString[50]; // کافیست یک آرایه کافی برای ذخیره نتیجه ایجاد کنید
    sprintf(maxToString, "%f", max);

    char result[1024] = ""; // حافظه برای نگهداری رشته ترکیب شده

    // ابتدا یک رشته خالی
    result[0] = '\0';

    strcat(result, averageToString);
    strcat(result, ",");
    strcat(result, maxToString);

    printf("in fork %d ,%s\n", numberOfFork, result);
    setSharedMemory(numberOfFork, result);
}

void printResults(int numberOfFork)
{
    char *result = reader(); // no strcat needed

    double max = INT_MIN;
    double sum = 0;

    for (size_t i = 1; i < numberOfFork + 1; i++)
    {
        const char delimiter[] = ",";
        char *token = strtok(result, delimiter); // Use 'result' instead of 'str'
        int section = 0;

        printf("token : %s\n", token);
        // جدا کردن و پردازش هر عدد
        while (token != NULL)
        {
            if (section == 0)
            {
                printf("atoi: %f , %s\n", atoi(token), token);
                sum += atoi(token);
                token = strtok(NULL, delimiter);
                section++;
            }
            else
            {
                double test = atoi(token);
                if (test > max)
                    max = test;
                token = strtok(NULL, delimiter);
            }
        }
    }

    printf("sum : %f\n", sum);
    // printf("avg : %f\n", sum / numberOfFork);
}

int main(int argc, char const *argv[])
{
    int numberOfFork = atoi(argv[1]);
    const char *filename = argv[2];
    const char *x = argv[3];

    if (!x)
    {
        initialWrite(numberOfFork, filename);
    }

    if (numberOfFork == 0)
        return 0;

    if (numberOfFork == 1)
    {
        printf("I'M parent now run 22\n");
        writeMaxMid(numberOfFork);

        if (!x)
            printf("Reader is : %s\n", reader());
        return 0;
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        printf("I'M child now run 00\n");
        writeMaxMid(numberOfFork);
        
        printf("xxxxxxxx\n");
        char str_int[20] = "";
        int newNumber = numberOfFork - 2;
        sprintf(str_int, "%d", newNumber);
        printf("newNumber %d\n", newNumber);

        execl("./main", "./main", str_int, filename, "x", NULL);
        perror("execl failed");
        exit(1);
    }
    else if (pid < 0)
    {
        perror("fork failed");
        exit(1);
    }
    else
    {
        printf("I'M parent now run 11\n");
        writeMaxMid(numberOfFork - 1);

        while (wait(NULL) > 0)
            ;

        printf("finished all\n");
    }

    if (!x)
    {
        printf("result is -: %s\n", reader());
        // deleteSharedMemory();
    }
    return 0;
}