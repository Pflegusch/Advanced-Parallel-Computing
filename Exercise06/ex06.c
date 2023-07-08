#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>

// Shared tmp array
size_t* tmp = NULL;
size_t* in_arr = NULL;
size_t* out_arr = NULL;
char num_threads = 0;

// Thread synchronization
pthread_barrier_t barrier;

size_t* random_array(const int array_length, unsigned int seed) {
    // Make the random numbers deterministic by providing a seed
    srand(seed);

    // Initialize array and fill it with random numbers
    size_t* arr = (size_t *)malloc(sizeof(size_t) * array_length);
    for (unsigned int i = 0; i < array_length; ++i) {
        arr[i] = rand();
    }
    
    return arr;
}

void* parallel_sum(void* args) {
    int thread_id = *(int *)args;
    int offset = 1;
    int n = num_threads;

    tmp[2 * thread_id] = in_arr[2 * thread_id];
    tmp[2 * thread_id + 1] = in_arr[2 * thread_id + 1];

    for (int d = n>>1; d > 0; d >>= 1) {
        pthread_barrier_wait(&barrier);
        if (thread_id < d) {
            int ai = offset * (2 * thread_id + 1) - 1;
            int bi = offset * (2 * thread_id + 2) - 1;

            tmp[bi] += tmp[ai];
        }
        offset *= 2;
    }

    if (thread_id == 0) {
        tmp[n - 1] = 0;
    }

    for (int d = 1; d < n; d *= 2) {
        offset >>=1; 
        pthread_barrier_wait(&barrier);

        if (thread_id < d) {
            int ai = offset * (2 * thread_id + 1) - 1;
            int bi = offset * (2 * thread_id + 2) - 1;

            float t = tmp[ai];
            tmp[ai] = tmp[bi];
            tmp[bi] += t;
        }
    }

    pthread_barrier_wait(&barrier);
    out_arr[2 * thread_id] = tmp[2 * thread_id];
    out_arr[2 * thread_id + 1] = tmp[2 * thread_id + 1];

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) return -1;

    // Initialize cli args
    const unsigned int array_length = atoi(argv[1]);
    const unsigned int num_threads = atoi(argv[2]);

    // Restrict array length to be a power of 2
    if ((array_length & (array_length - 1)) > 0) {
        printf("Array length should be power of two\n");
        return -1;
    }

    // Create random array with length given from first cli arg and a seed
    // and a second output array for the sum
    in_arr = random_array(array_length, 420);
    out_arr = (size_t *)malloc(sizeof(size_t) * array_length);
    tmp = (size_t *)malloc(sizeof(size_t) * array_length);
    out_arr[0] = 0;

    // Parallel sum for reference
    clock_t begin_reference = clock();
    printf("Begin reference is %f\n", (double)begin_reference);
    for(int i = 1; i <= array_length; ++i) {
        out_arr[i] = in_arr[i-1] + out_arr[i-1];
    }
    clock_t end_reference = clock();
    printf("End reference is %f\n", (double)end_reference);
    double time_spent_reference = (double)(end_reference - begin_reference) / CLOCKS_PER_SEC;
    printf("Reference version:         %zu\n", out_arr[array_length - 1]);
    printf("Total execution time of the reference version %f\n", time_spent_reference);

    // Create array of pthreads
    pthread_t threads[num_threads];

    int thread_ids[num_threads];
    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
    }
    clock_t begin_threading = clock();
    printf("Begin threading is %f\n", (double)begin_threading);
    pthread_barrier_init(&barrier, NULL, num_threads);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, parallel_sum, (void *)&thread_ids[i]);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    clock_t end_threading = clock();
    printf("End threading is %f\n", (double)end_threading);
    double time_spent_threading = (double)(end_threading - begin_threading) / CLOCKS_PER_SEC;
    printf("Thread parallized version: %zu\n", out_arr[array_length - 1]);
    printf("Total execution time of the atomic lock mechanism %f\n", time_spent_threading);

    return 0;
}

// Compile and run with clang ex02.c -O3 -pthread -o ex02.out && ./ex02.out 65536 128
// clang => cap -> gme wont squeeze
