#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "shop.c"

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address\n");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }
    
    printf("Connected to dungeon server\n");
    Player player;
    int running = 1;
    
    while (running) {
        printf("\n==== MAIN MENU ====\n");
        printf("1. Show Player Stats\n");
        printf("2. Shop (Buy Weapons)\n");
        printf("3. View Inventory & Equip Weapons\n");
        printf("4. Battle Mode\n");
        printf("5. Exit Game\n");
        printf("Choose an option: ");
        
        int choice;
        scanf("%d", &choice);
        write(sock, &choice, sizeof(int));
        
        switch (choice) {
            case 1:
                read(sock, &player, sizeof(Player));
                display_player_stats(&player);
                break;
                
            case 2: {
                Weapon shop_weapons[NUM_WEAPONS];
                read(sock, shop_weapons, sizeof(shop_weapons));
                
                printf("\n=== WEAPON SHOP ===\n");
                for (int i = 0; i < NUM_WEAPONS; i++) {
                    printf("[%d] %s - Price: %d gold, Damage: %d", 
                          i+1, shop_weapons[i].name, shop_weapons[i].price, shop_weapons[i].damage);
                    if (shop_weapons[i].has_passive) {
                        printf(" (Passive: %s)", shop_weapons[i].passive);
                    }
                    printf("\n");
                }
                printf("[@] Cancel\n");
                printf("Enter weapon number to buy (@ to cancel): ");
                
                char weapon_choice;
                scanf(" %c", &weapon_choice);
                write(sock, &weapon_choice, sizeof(char));
                
                char result[100];
                read(sock, result, sizeof(result));
                printf("%s\n", result);
                
                read(sock, &player, sizeof(Player));
                break;
            }
                
            case 3: {
                read(sock, &player, sizeof(Player));
                display_inventory(&player);
                
                printf("Enter weapon number to equip (-1 to cancel): ");
                int equip_choice;
                scanf("%d", &equip_choice);
                write(sock, &equip_choice, sizeof(int));
                
                char result[100];
                read(sock, result, sizeof(result));
                printf("%s\n", result);
                
                read(sock, &player, sizeof(Player));
                break;
            }
                
            case 4:
                char msg[32];
                read(sock, msg, sizeof(msg));
                printf("%s\n", msg);
                break;
                
            case 5:
                running = 0;
                printf("Exiting game...\n");
                break;
                
            default:
                printf("Invalid option\n");
        }
    }
    
    close(sock);
    return 0;
}
