#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define SHARED_MEMORY_NAME "/delivery_orders"
#define LOG_FILE "delivery.log"
#define MAX_ORDERS 100
#define NUM_AGENTS 3

typedef struct {
    char name[50];
    char address[100];
    char type[20];    
    char status[50];  
    time_t order_time;
} Order;

typedef struct {
    pthread_mutex_t mutex;  // Mutex buat sinkronisasi akses shared memory
    Order orders[MAX_ORDERS];
    int count;
} SharedData;

// Waktu buat log
void format_time(char *buffer, size_t size, time_t t) {
    struct tm *local = localtime(&t);
    strftime(buffer, size, "%d/%m/%Y %H:%M:%S", local);
}

// Open log
void write_log(const char *agent, const char *type, const char *name, const char *address) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) {
        perror("Failed to open log file");
        return;
    }
    char timebuf[32];
    time_t now = time(NULL);
    format_time(timebuf, sizeof(timebuf), now);
    fprintf(log, "[%s] [AGENT %s] %s package delivered to %s in %s\n",
            timebuf, agent, type, name, address);
    fclose(log);
}

void load_orders_from_csv(SharedData *shared_data) {
    FILE *file = fopen("delivery_order.csv", "r");
    if (!file) {
        perror("Failed to open delivery_order.csv");
        exit(EXIT_FAILURE);
    }

    char line[256];
    fgets(line, sizeof(line), file); 

    pthread_mutex_lock(&shared_data->mutex);
    shared_data->count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (shared_data->count >= MAX_ORDERS) {
            printf("Warning: Maximum order capacity reached\n");
            break;
        }
        Order *order = &shared_data->orders[shared_data->count];
        char *token = strtok(line, ",");
        if (!token) continue;
        strncpy(order->name, token, sizeof(order->name));
        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(order->address, token, sizeof(order->address));
        token = strtok(NULL, ",\n");
        if (!token) continue;
        strncpy(order->type, token, sizeof(order->type));
        strcpy(order->status, "Pending");
        order->order_time = time(NULL);
        shared_data->count++;
    }

    pthread_mutex_unlock(&shared_data->mutex);
    fclose(file);
}

void *agent_thread(void *arg) {
    char agent_name = *(char *)arg;

    // Buka shared memory
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open in agent");
        pthread_exit(NULL);
    }

    SharedData *shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap in agent");
        close(shm_fd);
        pthread_exit(NULL);
    }

    srand(time(NULL) ^ (agent_name << 8)); 

    while (1) {
        pthread_mutex_lock(&shared_data->mutex);
        int found = 0;
        for (int i = 0; i < shared_data->count; i++) {
            if (strcmp(shared_data->orders[i].type, "Express") == 0 &&
                strcmp(shared_data->orders[i].status, "Pending") == 0) {
                pthread_mutex_unlock(&shared_data->mutex);
                sleep(1 + rand() % 3);
                pthread_mutex_lock(&shared_data->mutex);

                // Update status
                snprintf(shared_data->orders[i].status, sizeof(shared_data->orders[i].status),
                         "Delivered by Agent %c", agent_name);

                write_log(&agent_name, "Express", shared_data->orders[i].name, shared_data->orders[i].address);

                printf("Agent %c delivered package to %s\n", agent_name, shared_data->orders[i].name);
                found = 1;
                break; 
            }
        }
        pthread_mutex_unlock(&shared_data->mutex);

        if (!found) {
            sleep(1);
        }
    }

    munmap(shared_data, sizeof(SharedData));
    close(shm_fd);
    pthread_exit(NULL);
}

int main() {
        int shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("shm_open");
            exit(EXIT_FAILURE);
        }
        if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
            perror("ftruncate");
            close(shm_fd);
            exit(EXIT_FAILURE);
        }
    
        SharedData *shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shared_data == MAP_FAILED) {
            perror("mmap");
            close(shm_fd);
            exit(EXIT_FAILURE);
        }
    
        static pthread_mutexattr_t mutex_attr;
        static int mutex_initialized = 0;
        if (!mutex_initialized) {
            pthread_mutexattr_init(&mutex_attr);
            pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
            pthread_mutex_init(&shared_data->mutex, &mutex_attr);
            mutex_initialized = 1;
        }
    
        pthread_mutex_lock(&shared_data->mutex);
        int need_load = (shared_data->count == 0);
        pthread_mutex_unlock(&shared_data->mutex);
    
        if (need_load) {
            load_orders_from_csv(shared_data);
        }
    
        pthread_t agents[NUM_AGENTS];
        char agent_names[NUM_AGENTS] = {'A', 'B', 'C'};
    
        for (int i = 0; i < NUM_AGENTS; i++) {
            if (pthread_create(&agents[i], NULL, agent_thread, &agent_names[i]) != 0) {
                perror("Failed to create agent thread");
                munmap(shared_data, sizeof(SharedData));
                close(shm_fd);
                exit(EXIT_FAILURE);
            }
        }
    
        printf("Delivery agents A, B, and C are now active...\n");
    
        for (int i = 0; i < NUM_AGENTS; i++) {
            pthread_join(agents[i], NULL);
        }
    
        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
        shm_unlink(SHARED_MEMORY_NAME);
    
        return 0;
    }
