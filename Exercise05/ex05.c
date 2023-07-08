#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

// Reset global var macro
#define reset() global_var = 0
#define reset_barrier() barriers = 0
#define reset_time() time_spent = 0.0
// Standard pthread barrier as a reference
pthread_barrier_t barrier;

// Global increment var
int global_var = 0;
int barriers = 0;
double time_spent = 0.0;
// Global variables for the centralized barrier implementation
int count = 0;
int processors = 0;
bool sense = true;

// Thread struct that expands the pthread_t struct
typedef struct {
    __uint8_t parentsense;
    __uint8_t* parent;
    __uint8_t* children[2];

    union {
        __uint8_t single[4];
        __uint32_t all;
    } havechild;

    union {
        __uint8_t single[4];
        __uint32_t all;
    } childnotready;

    __uint8_t sense;
    __uint8_t dummy;
    unsigned long int id;
} thread_t;

int thread_id = 0;

// Struct that hold the number of incrementations and amount of threads
struct args {
    int C;
    int N;
};

// Struct for the tree barrier type struct
struct thread_t_args {
    thread_t* t;
    int C;
    int N;
};

static inline void initialize_central_barrier(int processors_) {
    sense = true;
    processors = processors_;
}

void central_barrier() {
    bool local_sense = false;
    if (__sync_fetch_and_sub(&processors, 1) == 1) {
        count = processors;
        sense = local_sense;
    } else {
        while (sense != local_sense) __asm("");
    }
}

// Reference barrier
void* pthread_barrier(void* args) {
    printf("Before barrier..\n");
    barriers += 1;
    clock_t begin_reference_barrier = clock();
    pthread_barrier_wait(&barrier);
    clock_t end_reference_barrier = clock();
    time_spent += (double)(end_reference_barrier - begin_reference_barrier) / CLOCKS_PER_SEC;
    printf("After barrier..\n");
    for (int i = 0; i <= (((struct args *)args)->C / ((struct args *)args)->N) - 1; i++) {
        global_var++;
    }

    return NULL;
}

void* own_barrier(void* args) {
    printf("Before barrier..\n");
    barriers += 1;
    clock_t begin_own_barrier = clock();
    central_barrier();
    clock_t end_own_barrier = clock();
    time_spent += (double)(end_own_barrier - begin_own_barrier) / CLOCKS_PER_SEC;
    printf("After barrier..\n");
    for (int i = 0; i <= (((struct args *)args)->C / ((struct args *)args)->N) - 1; i++) {
        global_var++;
    }

    return NULL;
}

void initialize_tree_barrier(thread_t* tree_barriers, int num_threads) {  
    for (int i = 0; i < num_threads; i++) {
        thread_t* t = &tree_barriers[i];
        for (int j = 0; j < 4; j++) {
            t->havechild.single[j] = (4*i+j < (num_threads-1)) ? true : false;
        }
        t->parent = (i != 0) ? &tree_barriers[(i-1)/4].childnotready.single[(i-1)%4] : &t->dummy;
        t->children[0] = (2*i+1 < num_threads) ? &tree_barriers[2*i+1].parentsense : &t->dummy;
        t->children[1] = (2*i+2 < num_threads) ? &tree_barriers[2*i+2].parentsense : &t->dummy;
        t->childnotready.all = t->havechild.all;
        t->parentsense = 0x0;
        t->sense = 0x1;
        t->dummy = 0x0;
        t->id = i;
    }
}

void* tree_barrier(void* args) {
    printf("Before barrier..\n");
    barriers += 1;
    clock_t begin_tree_barrier = clock();
    thread_t* t = ((struct thread_t_args *)args)->t;
    while (t->childnotready.all != 0x0);
    t->childnotready.all = t->havechild.all;
    *t->parent = 0x0;
    if (t->id != 0) {
        while (t->parentsense != t->sense);
    }
    *t->children[0] = t->sense;
    *t->children[1] = t->sense;
    t->sense = !t->sense;
    clock_t end_tree_barrier = clock();
    time_spent += (double)(end_tree_barrier - begin_tree_barrier) / CLOCKS_PER_SEC;
    printf("After barrier..\n");

    for (int i = 0; i <= (((struct thread_t_args *)args)->C / ((struct thread_t_args *)args)->N) - 1; i++) {
        global_var++;
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) return -1;

    // Initialize cli args
    const int increment_count = atoi(argv[1]);
    const int num_threads = atoi(argv[2]);

    // Create array of pthreads
    pthread_t threads[num_threads];
 
    // Create struct which holds information for each thread
    struct args *thread_args = (struct args *)malloc(sizeof(struct args));
    thread_args->C = increment_count;
    thread_args->N = num_threads;

    // Run the reference barrier
    printf("Running reference barrier..\n");
    pthread_barrier_init(&barrier, NULL, num_threads);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, pthread_barrier, (void *)thread_args);
        sleep(1);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("Average barrier latency of reference barrier %f\n", time_spent / barriers);
    printf("\n");

    // Reset global var
    reset();
    reset_barrier();
    reset_time();

    // Run the own barrier
    printf("Running own barrier 1..\n");
    initialize_central_barrier(num_threads);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, own_barrier, (void *)thread_args);
        sleep(1);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Average barrier latency of own barrier %f\n", time_spent / barriers);
    printf("\n");

    reset();
    reset_barrier();
    reset_time();

    // Run the own barrier
    printf("Running own barrier 2..\n");
    initialize_central_barrier(num_threads);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, own_barrier, (void *)thread_args);
        sleep(1);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("\n");

    reset();
    reset_barrier();
    reset_time();

    // Create tree barrier
    thread_t tree_barriers_1[num_threads];
    initialize_tree_barrier(tree_barriers_1, num_threads);

    // Running tree based barrier
    printf("Running tree based barrier 1..\n");
    for (int i = 0; i < num_threads; i++) {
        struct thread_t_args *tree_args = (struct thread_t_args *)malloc(sizeof(struct thread_t_args));
        tree_args->N = num_threads;
        tree_args->C = increment_count;
        tree_args->t = &tree_barriers_1[thread_id];
        pthread_create(&threads[i], NULL, tree_barrier, (void*)tree_args);
        sleep(1);
        __sync_fetch_and_add(&thread_id, 1);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Average barrier latency of tree barrier %f\n", time_spent / barriers);
    printf("\n");

    reset();
    reset_barrier();
    reset_time();

    // Create another tree barrier
    thread_t tree_barriers_2[num_threads];
    initialize_tree_barrier(tree_barriers_2, num_threads);
    thread_id = 0;
    
    printf("Running tree based barrier 2..\n");
    initialize_tree_barrier(tree_barriers_2, num_threads);
    for (int i = 0; i < num_threads; i++) {
        struct thread_t_args *tree_args = (struct thread_t_args *)malloc(sizeof(struct thread_t_args));
        tree_args->N = num_threads;
        tree_args->C = increment_count;
        tree_args->t = &tree_barriers_2[thread_id];
        pthread_create(&threads[i], NULL, tree_barrier, (void*)tree_args);
        sleep(1);
        __sync_fetch_and_add(&thread_id, 1);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }   
    return 0;
}
