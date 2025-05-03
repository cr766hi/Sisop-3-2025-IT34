#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "shop.c"

#define PORT 8080

void handle_client(int client_socket) {
    Player player = create_new_player();
    int running = 1;
    
    while (running) {
        int choice;
        read(client_socket, &choice, sizeof(int));
        
        switch (choice) {
            case 1:
                write(client_socket, &player, sizeof(Player));
                break;
                
            case 2: {
                write(client_socket, weapons, sizeof(weapons));
                
                char weapon_choice;
                read(client_socket, &weapon_choice, sizeof(char));
                
                if (weapon_choice == '@') {
                    char msg[] = "Purchase cancelled.";
                    write(client_socket, msg, sizeof(msg));
                } else {
                    int weapon_idx = weapon_choice - '0';
                    Weapon* selected = get_weapon(weapon_idx);
                    
                    if (selected && player.gold >= selected->price && player.inventory_count < MAX_INVENTORY) {
                        player.gold -= selected->price;
                        strcpy(player.inventory[player.inventory_count].name, selected->name);
                        player.inventory[player.inventory_count].damage = selected->damage;
                        strcpy(player.inventory[player.inventory_count].passive, selected->passive);
                        player.inventory[player.inventory_count].has_passive = selected->has_passive;
                        player.inventory_count++;
                        
                        char msg[100];
                        snprintf(msg, sizeof(msg), "Purchased %s! Remaining gold: %d", 
                                selected->name, player.gold);
                        write(client_socket, msg, sizeof(msg));
                    } else {
                        char msg[] = "Cannot purchase weapon!";
                        write(client_socket, msg, sizeof(msg));
                    }
                }
                write(client_socket, &player, sizeof(Player));
                break;
            }
                
            case 3: {
                write(client_socket, &player, sizeof(Player));
                
                int equip_choice;
                read(client_socket, &equip_choice, sizeof(int));
                
                if (equip_choice >= 0 && equip_choice < player.inventory_count) {
                    equip_weapon(&player, equip_choice);
                    char msg[] = "Weapon equipped!";
                    write(client_socket, msg, sizeof(msg));
                } else {
                    char msg[] = "Invalid selection!";
                    write(client_socket, msg, sizeof(msg));
                }
                write(client_socket, &player, sizeof(Player));
                break;
            }
                
            case 4:
                write(client_socket, "Battle mode not implemented yet", 32);
                break;
                
            case 5:
                running = 0;
                close(client_socket);
                printf("Player disconnected\n");
                break;
                
            default:
                write(client_socket, "Invalid option", 15);
        }
    }
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Dungeon server running on port %d...\n", PORT);
    
    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        
        printf("New player connected\n");
        handle_client(client_socket);
    }
    
    return 0;
}
