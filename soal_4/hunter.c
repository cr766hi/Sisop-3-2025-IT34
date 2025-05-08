#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>

#define MAX_NAME_LENGTH 64
#define MAX_HUNTERS 100
#define MAX_DUNGEONS 100
#define NOTIFICATION_INTERVAL 3

typedef struct {
    char name[MAX_NAME_LENGTH];
    int level, exp, atk, hp, def;
    int banned;
    int notify;
    int used;
    key_t key;
    int shmid;
} Hunter;

typedef struct {
    char name[MAX_NAME_LENGTH];
    int min_level;
    int atk_reward, hp_reward, def_reward, exp_reward;
    key_t key;
    int used;
} Dungeon;

key_t shm_key_hunters = 1234;
key_t shm_key_dungeons = 5678;
Hunter *hunters;
Dungeon *dungeons;
Hunter *me = NULL;
int running = 1;

void load_shared_memory() {
    int shmid_hunters = shmget(shm_key_hunters, MAX_HUNTERS * sizeof(Hunter), 0666);
    if (shmid_hunters == -1) {
        perror("shmget hunters failed");
        exit(1);
    }
    hunters = (Hunter *)shmat(shmid_hunters, NULL, 0);
    if (hunters == (void *)-1) {
        perror("shmat hunters failed");
        exit(1);
    }

    int shmid_dungeons = shmget(shm_key_dungeons, MAX_DUNGEONS * sizeof(Dungeon), 0666);
    if (shmid_dungeons == -1) {
        perror("shmget dungeons failed");
        exit(1);
    }
    dungeons = (Dungeon *)shmat(shmid_dungeons, NULL, 0);
    if (dungeons == (void *)-1) {
        perror("shmat dungeons failed");
        exit(1);
    }
}

void register_hunter() {
    char name[MAX_NAME_LENGTH];
    printf("Masukkan nama: ");
    scanf(" %[^\n]", name); // Perbaikan di sini
    
    for (int i = 0; i < MAX_HUNTERS; i++) {
        if (!hunters[i].used) {
            strncpy(hunters[i].name, name, MAX_NAME_LENGTH);
            hunters[i].level = 1;
            hunters[i].exp = 0;
            hunters[i].atk = 10;
            hunters[i].hp = 100;
            hunters[i].def = 5;
            hunters[i].banned = 0;
            hunters[i].notify = 0;
            hunters[i].used = 1;
            hunters[i].key = 1000 + rand() % 9000;
            me = &hunters[i];
            printf("Berhasil registrasi sebagai %s. Key kamu: %d\n", name, me->key);
            return;
        }
    }
    printf("Pendaftaran gagal, kapasitas penuh.\n");
}

void login() {
    int key;
    printf("Masukkan key: ");
    scanf("%d", &key);
    
    for (int i = 0; i < MAX_HUNTERS; i++) {
        if (hunters[i].used && hunters[i].key == key) {
            if (hunters[i].banned) {
                printf("Hunter ini dibanned!\n");
                return;
            }
            me = &hunters[i];
            printf("Login berhasil sebagai %s\n", me->name);
            return;
        }
    }
    printf("Login gagal.\n");
}

void list_available_dungeons() {
    printf("\n=== Dungeon untuk %s (Level %d) ===\n", me->name, me->level);
    int count = 0;
    
    for (int i = 0; i < MAX_DUNGEONS; i++) {
        if (dungeons[i].used && dungeons[i].min_level <= me->level) {
            printf("%d. Key: %d | %s [Lv %d] [+%d ATK, +%d HP, +%d DEF, +%d EXP]\n",
                ++count, dungeons[i].key, dungeons[i].name, dungeons[i].min_level,
                dungeons[i].atk_reward, dungeons[i].hp_reward,
                dungeons[i].def_reward, dungeons[i].exp_reward);
        }
    }
    
    if (count == 0) {
        printf("Tidak ada dungeon yang tersedia untuk level Anda.\n");
    }
}

void raid_dungeon() {
    list_available_dungeons();
    
    int key;
    printf("Masukkan key dungeon: ");
    scanf("%d", &key);
    
    for (int i = 0; i < MAX_DUNGEONS; i++) {
        if (dungeons[i].used && dungeons[i].key == key && dungeons[i].min_level <= me->level) {
            me->atk += dungeons[i].atk_reward;
            me->hp += dungeons[i].hp_reward;
            me->def += dungeons[i].def_reward;
            me->exp += dungeons[i].exp_reward;
            
            printf("\nRaid sukses! Stat baru:\n");
            printf("ATK: %d (+%d)\n", me->atk, dungeons[i].atk_reward);
            printf("HP: %d (+%d)\n", me->hp, dungeons[i].hp_reward);
            printf("DEF: %d (+%d)\n", me->def, dungeons[i].def_reward);
            printf("EXP: %d (+%d)\n", me->exp, dungeons[i].exp_reward);
            
            if (me->exp >= 500) {
                me->exp = 0;
                me->level++;
                printf("\nLevel up! Sekarang level %d\n", me->level);
            }
            
            dungeons[i].used = 0;
            return;
        }
    }
    printf("Dungeon tidak valid atau level kurang.\n");
}

void show_stats() {
    printf("\n=== Stat %s ===\n", me->name);
    printf("Level: %d\n", me->level);
    printf("EXP: %d/500\n", me->exp);
    printf("ATK: %d\n", me->atk);
    printf("HP: %d\n", me->hp);
    printf("DEF: %d\n", me->def);
    printf("Status: %s\n", me->banned ? "BANNED" : "Active");
}

void hunter_menu() {
    int choice;
    do {
        printf("\n=== Menu Hunter ===\n");
        printf("[1] Lihat Stat\n");
        printf("[2] Raid Dungeon\n");
        printf("[3] Logout\n");
        printf(">> ");
        scanf("%d", &choice);
        
        switch(choice) {
            case 1:
                show_stats();
                break;
            case 2:
                raid_dungeon();
                break;
            case 3:
                me = NULL;
                printf("Logout berhasil.\n");
                break;
            default:
                printf("Pilihan tidak valid.\n");
        }
    } while (me != NULL);
}

int main() {
    srand(time(NULL));
    load_shared_memory();
    
    int choice;
    while (1) {
        if (!me) {
            printf("\n=== Menu Utama ===\n");
            printf("[1] Register\n");
            printf("[2] Login\n");
            printf("[3] Keluar\n");
            printf(">> ");
            scanf("%d", &choice);
            
            switch(choice) {
                case 1:
                    register_hunter();
                    break;
                case 2:
                    login();
                    break;
                case 3:
                    printf("Keluar dari program.\n");
                    return 0;
                default:
                    printf("Pilihan tidak valid.\n");
            }
        } else {
            hunter_menu();
        }
    }
    
    return 0;
}
