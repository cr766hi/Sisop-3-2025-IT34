#include <stdio.h>
#include <string.h>

#define NUM_WEAPONS 5
#define MAX_INVENTORY 10

typedef struct {
    char name[50];
    int price;
    int damage;
    char passive[100];
    int has_passive;
} Weapon;

typedef struct {
    char name[50];
    int damage;
    char passive[100];
    int has_passive;
} InventoryItem;

typedef struct {
    int gold;
    InventoryItem equipped;
    int base_damage;
    int kills;
    InventoryItem inventory[MAX_INVENTORY];
    int inventory_count;
} Player;

Player create_new_player() {
    Player p;
    p.gold = 500;
    strcpy(p.equipped.name, "Fists");
    p.equipped.damage = 5;
    strcpy(p.equipped.passive, "None");
    p.equipped.has_passive = 0;
    p.base_damage = 5;
    p.kills = 0;
    p.inventory_count = 0;
    return p;
}

Weapon weapons[NUM_WEAPONS] = {
    {"Terra Blade", 50, 10, "None", 0},
    {"Flint & Steel", 150, 25, "None", 0},
    {"Kitchen Knife", 200, 35, "None", 0},
    {"Staff of Light", 120, 20, "10% Insta-Kill Chance", 1},
    {"Dragon Claws", 300, 50, "+30% Crit Chance", 1}
};

void display_weapons() {
    printf("\n=== WEAPON SHOP ===\n");
    for (int i = 0; i < NUM_WEAPONS; i++) {
        printf("[%d] %s - Price: %d gold, Damage: %d", 
              i+1, weapons[i].name, weapons[i].price, weapons[i].damage);
        if (weapons[i].has_passive) {
            printf(" (Passive: %s)", weapons[i].passive);
        }
        printf("\n");
    }
    printf("[@] Cancel\n");
}

Weapon* get_weapon(int index) {
    if (index >= 1 && index <= NUM_WEAPONS) {
        return &weapons[index-1];
    }
    return NULL;
}

void display_inventory(Player* player) {
    printf("\n=== YOUR INVENTORY ===\n");
    for (int i = 0; i < player->inventory_count; i++) {
        printf("[%d] %s", i, player->inventory[i].name);
        if (player->inventory[i].has_passive) {
            printf(" (Passive: %s)", player->inventory[i].passive);
        }
        if (strcmp(player->inventory[i].name, player->equipped.name) == 0) {
            printf(" (EQUIPPED)");
        }
        printf("\n");
    }
}

int equip_weapon(Player* player, int index) {
    if (index >= 0 && index < player->inventory_count) {
        player->equipped = player->inventory[index];
        player->base_damage = player->inventory[index].damage;
        return 1;
    }
    return 0;
}

void display_player_stats(Player* player) {
    printf("\n=== PLAYER STATS ===\n");
    printf("Gold: %d | Equipped Weapon: %s | Base Damage: %d | Kills: %d", 
          player->gold, player->equipped.name, player->base_damage, player->kills);
    if (player->equipped.has_passive) {
        printf(" | Passive: %s", player->equipped.passive);
    }
    printf("\n\n");
}
