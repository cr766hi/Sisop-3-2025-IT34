#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MEMORY_SIZE 1024

typedef struct {
    char hunter_name[50];
    int lvl;
    int experience;
    int attack;
    int health;
    int defense;
    int state;    
    int unique_id; 
} Hunter;

int main() {
    key_t shm_key = 5678; // Ganti dengan key unik lain
    int shared_mem_id;
    Hunter *shared_hunter;

    // Membuat atau membuka shared memory dengan permission read-write
    shared_mem_id = shmget(shm_key, MEMORY_SIZE, IPC_CREAT | 0666);
    if (shared_mem_id < 0) {
        perror("Gagal membuat/mengakses shared memory");
        exit(EXIT_FAILURE);
    }

    shared_hunter = (Hunter *) shmat(shared_mem_id, NULL, 0);
    if (shared_hunter == (void *) -1) {
        perror("Gagal mengaitkan shared memory");
        exit(EXIT_FAILURE);
    }

    printf("Masukkan nama hunter: ");
    scanf("%49s", shared_hunter->hunter_name);

    shared_hunter->lvl = 1;
    shared_hunter->experience = 0;
    shared_hunter->attack = 10;
    shared_hunter->health = 100;
    shared_hunter->defense = 5;
    shared_hunter->state = 0;
    shared_hunter->unique_id = rand() % 100000; 

    printf("Pendaftaran berhasil! ID Hunter: %d\n", shared_hunter->unique_id);

    // Lepas kaitan shared memory
    if (shmdt(shared_hunter) == -1) {
        perror("Gagal melepaskan shared memory");
        exit(EXIT_FAILURE);
    }

    return 0;
}
