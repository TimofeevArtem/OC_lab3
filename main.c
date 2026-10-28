#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct{
    int startRow;
    int endRow;

    HANDLE startEvent;
    HANDLE doneEvent;

    int stop;
} MatrixTask;

DWORD WINAPI ThreadFunction1(LPVOID param);
DWORD WINAPI ThreadFunction2(LPVOID param);
DWORD WINAPI ThreadFunction3(LPVOID param);

int main(int argc, char *argv[]){
    if (argc != 2){
        printf("Неверный ввод\n");
        return 1;
    }

    int threadCount = atoi(argv[1]);

    if (threadCount <= 0){
        printf("Количество потоков должно быть положительным\n");
        return 1;
    }

    int rows, cols, windowSize, k;

    printf("Введите rows, cols, windowSize и k");
    if (scanf("%d %d %d %d", &rows, &cols, &windowSize, &k) != 4){
        printf("Ошибка ввода параметров\n");
        return 1;
    }

    if (rows <= 0 || cols <= 0 || windowSize <= 0 || k <= 0){
        printf("Параметры должны быть положительными\n");
        return 1;
    }

    if (threadCount > 2 * rows){
        threadCount = 2 * rows;
    }

    float matrix[rows][cols];

    printf("Введите матрицу");
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            if (scanf("%f", &matrix[i][j]) != 1){
                printf("Ошибка ввода элемента матрицы\n");
                return 1;
            }
        }
    }

    HANDLE threads[threadCount];
    MatrixTask tasks[threadCount];

    if (threadCount <= rows){
        for(int i = 0; i < threadCount; i++){
            tasks[i].startRow = rows * i / threadCount;
            tasks[i].endRow = rows * (i + 1) / threadCount;

            tasks[i].startEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            tasks[i].doneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            tasks[i].stop = 0;

            if (tasks[i].startEvent == NULL || tasks[i].doneEvent == NULL){
                printf("Ошибка создания события\n");

                if (tasks[i].startEvent != NULL){
                    CloseHandle(tasks[i].startEvent);
                }

                if (tasks[i].doneEvent != NULL){
                    CloseHandle(tasks[i].doneEvent);
                }

                for(int j = 0; j < i; j++){
                    tasks[j].stop = 1;
                    SetEvent(tasks[j].startEvent);
                }

                for(int j = 0; j < i; j++){
                    WaitForSingleObject(threads[j], INFINITE);
                    CloseHandle(threads[j]);
                    CloseHandle(tasks[j].startEvent);
                    CloseHandle(tasks[j].doneEvent);
                }

                return 1;
            }

            threads[i] = CreateThread(NULL, 0, ThreadFunction1, &tasks[i], 0, NULL);

            if (threads[i] == NULL){
                printf("Ошибка создания потока\n");

                CloseHandle(tasks[i].startEvent);
                CloseHandle(tasks[i].doneEvent);

                for(int j = 0; j < i; j++){
                    tasks[j].stop = 1;
                    SetEvent(tasks[j].startEvent);
                }

                for(int j = 0; j < i; j++){
                    WaitForSingleObject(threads[j], INFINITE);
                    CloseHandle(threads[j]);
                    CloseHandle(tasks[j].startEvent);
                    CloseHandle(tasks[j].doneEvent);
                }

                return 1;
            }
        }
    }
    else{
        int erosionThreads = threadCount / 2;
        int dilationThreads = threadCount - erosionThreads;

        for(int i = 0; i < threadCount; i++){
            int localIndex;
            int groupSize;

            if (i < erosionThreads){
                localIndex = i;
                groupSize = erosionThreads;
            }
            else{
                localIndex = i - erosionThreads;
                groupSize = dilationThreads;
            }

            tasks[i].startRow = rows * localIndex / groupSize;
            tasks[i].endRow = rows * (localIndex + 1) / groupSize;

            tasks[i].startEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            tasks[i].doneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            tasks[i].stop = 0;

            if (tasks[i].startEvent == NULL || tasks[i].doneEvent == NULL){
                printf("Ошибка создания события\n");

                if (tasks[i].startEvent != NULL){
                    CloseHandle(tasks[i].startEvent);
                }

                if (tasks[i].doneEvent != NULL){
                    CloseHandle(tasks[i].doneEvent);
                }

                for(int j = 0; j < i; j++){
                    tasks[j].stop = 1;
                    SetEvent(tasks[j].startEvent);
                }

                for(int j = 0; j < i; j++){
                    WaitForSingleObject(threads[j], INFINITE);
                    CloseHandle(threads[j]);
                    CloseHandle(tasks[j].startEvent);
                    CloseHandle(tasks[j].doneEvent);
                }

                return 1;
            }

            if (i < erosionThreads){
                threads[i] = CreateThread(NULL, 0, ThreadFunction2, &tasks[i], 0, NULL);
            }
            else{
                threads[i] = CreateThread(NULL, 0, ThreadFunction3, &tasks[i], 0, NULL);
            }

            if (threads[i] == NULL){
                printf("Ошибка создания потока\n");

                CloseHandle(tasks[i].startEvent);
                CloseHandle(tasks[i].doneEvent);

                for(int j = 0; j < i; j++){
                    tasks[j].stop = 1;
                    SetEvent(tasks[j].startEvent);
                }

                for(int j = 0; j < i; j++){
                    WaitForSingleObject(threads[j], INFINITE);
                    CloseHandle(threads[j]);
                    CloseHandle(tasks[j].startEvent);
                    CloseHandle(tasks[j].doneEvent);
                }

                return 1;
            }
        }
    }

    return 0;
}