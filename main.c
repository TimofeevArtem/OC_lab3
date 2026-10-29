#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct{
    int startRow;
    int endRow;

    HANDLE startEvent;
    HANDLE doneEvent;
    int stop;

    int rows;
    int cols;
    int windowSize;

    float *matrix;
    float *erosionResult;
    float *dilationResult;

    float *erosionSource;
    float *dilationSource;
} MatrixTask;

void erosion(int rows, int cols, int windowSize, int startRow, int endRow, float matrix[rows][cols], float result[rows][cols]){
    int offsetStart = -((windowSize - 1) / 2);
    int offsetEnd = offsetStart + windowSize - 1;

    for(int i = startRow; i < endRow; i++){
        for(int j = 0; j < cols; j++){
            float minValue;
            int found = 0;

            for(int di = offsetStart; di <= offsetEnd; di++){
                for(int dj = offsetStart; dj <= offsetEnd; dj++){
                    int ni = i + di;
                    int nj = j + dj;

                    if (ni < 0 || ni >= rows || nj < 0 || nj >= cols){
                        continue;
                    }

                    if (!found){
                        minValue = matrix[ni][nj];
                        found = 1;
                        continue;
                    }
                    if (matrix[ni][nj] < minValue){
                        minValue = matrix[ni][nj];
                    }

                }
            }

            result[i][j] = minValue;
        }
    }
}

void dilation(int rows, int cols, int windowSize, int startRow, int endRow, float matrix[rows][cols], float result[rows][cols]){
    int offsetStart = -((windowSize - 1) / 2);
    int offsetEnd = offsetStart + windowSize - 1;

    for(int i = startRow; i < endRow; i++){
        for(int j = 0; j < cols; j++){
            float maxValue;
            int found = 0;

            for(int di = offsetStart; di <= offsetEnd; di++){
                for(int dj = offsetStart; dj <= offsetEnd; dj++){
                    int ni = i + di;
                    int nj = j + dj;

                    if (ni < 0 || ni >= rows || nj < 0 || nj >= cols){
                        continue;
                    }

                    if (!found){
                        maxValue = matrix[ni][nj];
                        found = 1;
                        continue;
                    }

                    if (matrix[ni][nj] > maxValue){
                        maxValue = matrix[ni][nj];
                    }
                }
            }

            result[i][j] = maxValue;
        }
    }
}

DWORD WINAPI ThreadFunction1(LPVOID lpParam){
    MatrixTask *task = (MatrixTask *)lpParam;

    while(1){
        WaitForSingleObject(task->startEvent, INFINITE);

        if (task->stop){
            break;
        }

        erosion(task->rows, task->cols, task->windowSize, task->startRow, task->endRow, (float (*)[task->cols])task->erosionSource, (float (*)[task->cols])task->erosionResult);

        dilation(task->rows, task->cols, task->windowSize, task->startRow, task->endRow, (float (*)[task->cols])task->dilationSource, (float (*)[task->cols])task->dilationResult);

        SetEvent(task->doneEvent);
    }

    return 0;
}

DWORD WINAPI ThreadFunction2(LPVOID lpParam){
    MatrixTask *task = (MatrixTask *)lpParam;

    while(1){
        WaitForSingleObject(task->startEvent, INFINITE);

        if (task->stop){
            break;
        }

        erosion(task->rows, task->cols, task->windowSize, task->startRow, task->endRow, (float (*)[task->cols])task->erosionSource, (float (*)[task->cols])task->erosionResult);

        SetEvent(task->doneEvent);
    }

    return 0;
}

DWORD WINAPI ThreadFunction3(LPVOID lpParam){
    MatrixTask *task = (MatrixTask *)lpParam;

    while(1){
        WaitForSingleObject(task->startEvent, INFINITE);

        if (task->stop){
            break;
        }

        dilation(task->rows, task->cols, task->windowSize, task->startRow, task->endRow, (float (*)[task->cols])task->dilationSource, (float (*)[task->cols])task->dilationResult);

        SetEvent(task->doneEvent);
    }

    return 0;
}

int main(int argc, char *argv[]){
    if (argc != 2){
        printf("Incorrect input\n");
        return 1;
    }

    int threadCount = atoi(argv[1]);

    if (threadCount <= 0){
        printf("Count of threads must be positive\n");
        return 1;
    }

    int rows, cols, windowSize, k;

    printf("Enter rows, cols, windowSize and k\n");
    if (scanf("%d %d %d %d", &rows, &cols, &windowSize, &k) != 4){
        printf("Incorrect parametrs\n");
        return 1;
    }

    if (rows <= 0 || cols <= 0 || windowSize <= 0 || k <= 0){
        printf("Paramets must be positive\n");
        return 1;
    }

    if (threadCount > 2 * rows){
        threadCount = 2 * rows;
    }

    float matrix[rows][cols];

    float erosionA[rows][cols];
    float erosionB[rows][cols];

    float dilationA[rows][cols];
    float dilationB[rows][cols];

    printf("Enter the matrix\n");
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            if (scanf("%f", &matrix[i][j]) != 1){
                printf("Incorrect matrix input\n");
                return 1;
            }

            erosionA[i][j] = matrix[i][j];
            dilationA[i][j] = matrix[i][j];
        }
    }

    HANDLE threads[threadCount];
    MatrixTask tasks[threadCount];

    int erosionThreads = threadCount;
    int dilationThreads = 0;

    if (threadCount > rows){
        erosionThreads = threadCount / 2;
        dilationThreads = threadCount - erosionThreads;
    }

    if (threadCount <= rows){
        for(int i = 0; i < threadCount; i++){
            tasks[i].startRow = rows * i / threadCount;
            tasks[i].endRow = rows * (i + 1) / threadCount;

            tasks[i].rows = rows;
            tasks[i].cols = cols;
            tasks[i].windowSize = windowSize;

            tasks[i].startEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            tasks[i].doneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            tasks[i].stop = 0;

            if (tasks[i].startEvent == NULL || tasks[i].doneEvent == NULL){
                printf("CreateEvent error\n");

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
                printf("CreateThread error\n");
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
    else {
        for (int i = 0; i < threadCount; i++) {
            int localIndex;
            int groupSize;

            if (i < erosionThreads) {
                localIndex = i;
                groupSize = erosionThreads;
            }
            else {
                localIndex = i - erosionThreads;
                groupSize = dilationThreads;
            }

            tasks[i].startRow = rows * localIndex / groupSize;
            tasks[i].endRow = rows * (localIndex + 1) / groupSize;

            tasks[i].rows = rows;
            tasks[i].cols = cols;
            tasks[i].windowSize = windowSize;

            tasks[i].startEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            tasks[i].doneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            tasks[i].stop = 0;

            if (tasks[i].startEvent == NULL || tasks[i].doneEvent == NULL) {
                printf("CreateEvent error\n");

                if (tasks[i].startEvent != NULL) {
                    CloseHandle(tasks[i].startEvent);
                }

                if (tasks[i].doneEvent != NULL) {
                    CloseHandle(tasks[i].doneEvent);
                }

                for (int j = 0; j < i; j++) {
                    tasks[j].stop = 1;
                    SetEvent(tasks[j].startEvent);
                }

                for (int j = 0; j < i; j++) {
                    WaitForSingleObject(threads[j], INFINITE);
                    CloseHandle(threads[j]);
                    CloseHandle(tasks[j].startEvent);
                    CloseHandle(tasks[j].doneEvent);
                }

                return 1;
            }

            if (i < erosionThreads) {
                threads[i] = CreateThread(
                    NULL, 0, ThreadFunction2, &tasks[i], 0, NULL
                );
            }
            else {
                threads[i] = CreateThread(
                    NULL, 0, ThreadFunction3, &tasks[i], 0, NULL
                );
            }

            if (threads[i] == NULL) {
                printf("CreateThread error\n");

                CloseHandle(tasks[i].startEvent);
                CloseHandle(tasks[i].doneEvent);

                for (int j = 0; j < i; j++) {
                    tasks[j].stop = 1;
                    SetEvent(tasks[j].startEvent);
                }

                for (int j = 0; j < i; j++) {
                    WaitForSingleObject(threads[j], INFINITE);
                    CloseHandle(threads[j]);
                    CloseHandle(tasks[j].startEvent);
                    CloseHandle(tasks[j].doneEvent);
                }

                return 1;
            }
        }
    }

    float (*erosionCurrent)[cols] = erosionA;
    float (*erosionNext)[cols] = erosionB;

    float (*dilationCurrent)[cols] = dilationA;
    float (*dilationNext)[cols] = dilationB;

    LARGE_INTEGER frequency, startTime, endTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startTime);

    for(int iteration = 0; iteration < k; iteration++){
        for(int i = 0; i < threadCount; i++){
            tasks[i].erosionSource = &erosionCurrent[0][0];
            tasks[i].erosionResult = &erosionNext[0][0];

            tasks[i].dilationSource = &dilationCurrent[0][0];
            tasks[i].dilationResult = &dilationNext[0][0];
        }

        for(int i = 0; i < threadCount; i++){
            SetEvent(tasks[i].startEvent);
        }

        for(int i = 0; i < threadCount; i++){
            WaitForSingleObject(tasks[i].doneEvent, INFINITE);
        }

        float (*tempErosion)[cols] = erosionCurrent;
        erosionCurrent = erosionNext;
        erosionNext = tempErosion;

        float (*tempDilation)[cols] = dilationCurrent;
        dilationCurrent = dilationNext;
        dilationNext = tempDilation;
    }

    QueryPerformanceCounter(&endTime);
    double elapsedSeconds =(double)(endTime.QuadPart - startTime.QuadPart) / (double)frequency.QuadPart;

    

    printf("\nResult of erosian:\n");
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            printf("%f ", erosionCurrent[i][j]);
        }
        printf("\n");
    }

    printf("\nResult of dilation:\n");
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            printf("%f ", dilationCurrent[i][j]);
        }
        printf("\n");
    }
    printf("\nComputation time: %f seconds\n", elapsedSeconds);
    for(int i = 0; i < threadCount; i++){
        tasks[i].stop = 1;
        SetEvent(tasks[i].startEvent);
    }

    for(int i = 0; i < threadCount; i++){
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
        CloseHandle(tasks[i].startEvent);
        CloseHandle(tasks[i].doneEvent);
    }

    return 0;
}