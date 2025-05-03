#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>

#define SHARED_MEMORY_NAME "/delivery_orders"
#define LOG_FILE "delivery.log"
#define MAX_ORDERS 100

typedef struct {
    char name[50];
    char address[100];
    char type[20];    
    char status[50];  
    time_t order_time;
} Order;

typedef struct {
    Order orders[MAX_ORDERS];
    int count;
} OrderList;

int file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

void download_csv_if_needed() {
    if (!file_exists("delivery_order.csv")) {
        printf("Downloading delivery_order.csv...\n");
        int result = system("wget -q --show-progress --no-check-certificate "
                            "'https://drive.google.com/uc?export=download&id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9' "
                            "-O delivery_order.csv");
        if (result != 0) {
            fprintf(stderr, "Download failed with code %d\n", result);
        }
    }
}

// Waktu buat log
void format_time(char *buffer, size_t size, time_t t) {
    struct tm *local = localtime(&t);
    strftime(buffer, size, "%d/%m/%Y %H:%M:%S", local);
}

// Log
void log_delivery(const char *agent, const char *type, const char *name, const char *address) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log == NULL) {
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

// Load data dari CSV ke shared memory
void load_orders_from_csv(OrderList *order_list) {
    FILE *file = fopen("delivery_order.csv", "r");
    if (file == NULL) {
        perror("Failed to open delivery_order.csv");
        exit(1);
    }
    char line[256];
    fgets(line, sizeof(line), file); // Skip header
    order_list->count = 0;
    while (fgets(line, sizeof(line), file)) {
        if (order_list->count >= MAX_ORDERS) {
            printf("Warning: Maximum order capacity reached\n");
            break;
        }
        Order *order = &order_list->orders[order_list->count];
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
        order_list->count++;
    }
    fclose(file);
}

// List
void list_orders(OrderList *order_list) {
    printf("All Orders:\n");
    printf("-------------------------------------------------\n");
    printf("%-20s %-20s %-30s\n", "Name", "Type", "Status");
    printf("-------------------------------------------------\n");
    for (int i = 0; i < order_list->count; i++) {
        printf("%-20s %-20s %-30s\n",
               order_list->orders[i].name,
               order_list->orders[i].type,
               order_list->orders[i].status);
    }
    printf("-------------------------------------------------\n");
}

// Status
void check_status(OrderList *order_list, const char *name) {
    for (int i = 0; i < order_list->count; i++) {
        if (strcmp(order_list->orders[i].name, name) == 0) {
            printf("Status for %s: %s\n", name, order_list->orders[i].status);
            return;
        }
    }
    printf("No order found for %s\n", name);
}

// Delivery
void deliver_order(OrderList *order_list, const char *name, const char *agent) {
    for (int i = 0; i < order_list->count; i++) {
        if (strcmp(order_list->orders[i].name, name) == 0 &&
            strcmp(order_list->orders[i].status, "Pending") == 0) {
            snprintf(order_list->orders[i].status, sizeof(order_list->orders[i].status),
                     "Delivered by Agent %s", agent);
            log_delivery(agent, order_list->orders[i].type, order_list->orders[i].name, order_list->orders[i].address);
            printf("Order for %s has been delivered by Agent %s\n", name, agent);
            return;
        }
    }
    printf("No pending order found for %s\n", name);
}

int main(int argc, char *argv[]) {

    download_csv_if_needed();

    // Buat shared memory POSIX
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    ftruncate(shm_fd, sizeof(OrderList));
    OrderList *order_list = mmap(NULL, sizeof(OrderList), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (order_list == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    if (order_list->count == 0) {
        load_orders_from_csv(order_list);
    }

    if (argc < 2) {
        printf("Usage:\n");
        printf("./dispatcher -deliver [Name]\n");
        printf("./dispatcher -status [Name]\n");
        printf("./dispatcher -list\n");
        munmap(order_list, sizeof(OrderList));
        close(shm_fd);
        return 1;
    }

    if (strcmp(argv[1], "-list") == 0) {
        list_orders(order_list);
    } else if (strcmp(argv[1], "-status") == 0 && argc == 3) {
        check_status(order_list, argv[2]);
    } else if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
        const char *agent = getenv("USER");
        if (!agent) agent = "UNKNOWN";
        deliver_order(order_list, argv[2], agent);
    } else {
        printf("Invalid command\n");
    }

    munmap(order_list, sizeof(OrderList));
    close(shm_fd);
    // shm_unlink(SHARED_MEMORY_NAME);

    return 0;
}
