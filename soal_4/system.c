#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define MAX_HUNTERS 100
#define MAX_DUNGEONS 100
#define MAX_NAME_LENGTH 64

typedef struct {
    char name[MAX_NAME_LENGTH];
    int level;
    int exp;
    int atk;
    int hp;
    int def;
    int banned;
    int notify;
    int used;
    key_t key;
} Hunter;

typedef struct {
    char name[MAX_NAME_LENGTH];
    int min_level;
    int atk_reward;
    int hp_reward;
    int def_reward;
    int exp_reward;
    key_t key;
    int used;
} Dungeon;

key_t shm_key_hunters = 1234;
key_t shm_key_dungeons = 5678;

Hunter *hunters;
Dungeon *dungeons;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void load_shared_memory() {
    int shmid_hunters = shmget(shm_key_hunters, MAX_HUNTERS * sizeof(Hunter), IPC_CREAT | 0666);
    if (shmid_hunters < 0) {
        perror("shmget hunters");
        exit(EXIT_FAILURE);
    }
    hunters = (Hunter *)shmat(shmid_hunters, NULL, 0);
    if (hunters == (void *)-1) {
        perror("shmat hunters");
        exit(EXIT_FAILURE);
    }

    int shmid_dungeons = shmget(shm_key_dungeons, MAX_DUNGEONS * sizeof(Dungeon), IPC_CREAT | 0666);
    if (shmid_dungeons < 0) {
        perror("shmget dungeons");
        exit(EXIT_FAILURE);
    }
    dungeons = (Dungeon *)shmat(shmid_dungeons, NULL, 0);
    if (dungeons == (void *)-1) {
        perror("shmat dungeons");
        exit(EXIT_FAILURE);
    }
}

void cleanup(int signum) {
    pthread_mutex_destroy(&lock);

    int shmid_hunters = shmget(shm_key_hunters, 0, 0666);
    if (shmid_hunters >= 0) {
        shmctl(shmid_hunters, IPC_RMID, NULL);
    }
    int shmid_dungeons = shmget(shm_key_dungeons, 0, 0666);
    if (shmid_dungeons >= 0) {
        shmctl(shmid_dungeons, IPC_RMID, NULL);
    }
    printf("\n[INFO] Semua shared memory telah dihapus. Program keluar.\n");
    exit(0);
}

void print_hunters() {
    pthread_mutex_lock(&lock);
    printf("\n=== List Hunter Terdaftar ===\n");
    int found = 0;
    for (int i = 0; i < MAX_HUNTERS; i++) {
        if (hunters[i].used) {
            found = 1;
            printf("Name: %s | Lv: %d | EXP: %d | ATK: %d | HP: %d | DEF: %d | %s\n",
                hunters[i].name, hunters[i].level, hunters[i].exp, hunters[i].atk,
                hunters[i].hp, hunters[i].def,
                hunters[i].banned ? "[BANNED]" : "");
        }
    }
    if (!found) printf("Tidak ada hunter terdaftar.\n");
    pthread_mutex_unlock(&lock);
}

void print_dungeons() {
    pthread_mutex_lock(&lock);
    printf("\n=== List Dungeon Tersedia ===\n");
    int found = 0;
    for (int i = 0; i < MAX_DUNGEONS; i++) {
        if (dungeons[i].used) {
            found = 1;
            printf("Key: %d | Name: %s | Min Level: %d | +ATK %d | +HP %d | +DEF %d | +EXP %d\n",
                dungeons[i].key, dungeons[i].name, dungeons[i].min_level,
                dungeons[i].atk_reward, dungeons[i].hp_reward,
                dungeons[i].def_reward, dungeons[i].exp_reward);
        }
    }
    if (!found) printf("Tidak ada dungeon tersedia.\n");
    pthread_mutex_unlock(&lock);
}

void generate_dungeon() {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_DUNGEONS; i++) {
        if (!dungeons[i].used) {
            dungeons[i].used = 1;
            // Contoh nama dungeon acak dari daftar
            const char *names[] = {"Demon Castle", "Bramak Mountain", "Rad Gate", "Dark Forest", "Ice Cave"};
            strcpy(dungeons[i].name, names[rand() % 5]);

            dungeons[i].min_level = (rand() % 5) + 1;
            dungeons[i].atk_reward = 100 + rand() % 51;
            dungeons[i].hp_reward = 50 + rand() % 51;
            dungeons[i].def_reward = 25 + rand() % 26;
            dungeons[i].exp_reward = 150 + rand() % 151;
            dungeons[i].key = 10000 + rand() % 90000;
            printf("[INFO] Dungeon baru dibuat: %s (Min Level: %d)\n", dungeons[i].name, dungeons[i].min_level);
            break;
        }
    }
    pthread_mutex_unlock(&lock);
}

void *dungeon_generator_thread(void *arg) {
    while (1) {
        generate_dungeon();
        sleep(10);
    }
    return NULL;
}

void ban_unban_hunter() {
    char name[MAX_NAME_LENGTH];
    printf("Masukkan nama hunter untuk ban/unban: ");
    scanf("%s", name);

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_HUNTERS; i++) {
        if (hunters[i].used && strcmp(hunters[i].name, name) == 0) {
            hunters[i].banned = !hunters[i].banned;
            printf("Hunter %s telah %s.\n", name, hunters[i].banned ? "dibanned" : "diunban");
            pthread_mutex_unlock(&lock);
            return;
        }
    }
    printf("Hunter tidak ditemukan.\n");
    pthread_mutex_unlock(&lock);
}

void reset_hunter() {
    char name[MAX_NAME_LENGTH];
    printf("Masukkan nama hunter untuk reset: ");
    scanf("%s", name);

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_HUNTERS; i++) {
        if (hunters[i].used && strcmp(hunters[i].name, name) == 0) {
            hunters[i].level = 1;
            hunters[i].exp = 0;
            hunters[i].atk = 10;
            hunters[i].hp = 100;
            hunters[i].def = 5;
            hunters[i].banned = 0;
            printf("Hunter %s telah direset ke stats awal.\n", name);
            pthread_mutex_unlock(&lock);
            return;
        }
    }
    printf("Hunter tidak ditemukan.\n");
    pthread_mutex_unlock(&lock);
}

void system_menu() {
    int choice;
    do {
        printf("\n=== SYSTEM MENU ===\n");
        printf("1. Lihat Hunter\n");
        printf("2. Lihat Dungeon\n");
        printf("3. Generate Dungeon Manual\n");
        printf("4. Ban/Unban Hunter\n");
        printf("5. Reset Hunter\n");
        printf("6. Exit\n");
        printf("Pilihan: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: print_hunters(); break;
            case 2: print_dungeons(); break;
            case 3: generate_dungeon(); break;
            case 4: ban_unban_hunter(); break;
            case 5: reset_hunter(); break;
            case 6: printf("Keluar dari sistem...\n"); break;
            default: printf("Pilihan tidak valid!\n");
        }
    } while (choice != 6);
}

int main() {
    signal(SIGINT, cleanup);
    srand(time(NULL));
    load_shared_memory();

    pthread_t dungeon_thread;
    if (pthread_create(&dungeon_thread, NULL, dungeon_generator_thread, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    system_menu();

    // Saat keluar, cleanup akan dipanggil oleh signal handler
    raise(SIGINT);

    return 0;
}
