#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 4096

void display_menu() {
    printf("\nImage Decoder Client |\n");
    printf("----------------------\n");
    printf("1. Send input file to server\n");
    printf("2. Download file from server\n");
    printf("3. Exit\n");
    printf(">> ");
}

int connect_to_server() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }
    
    return sock;
}

void send_file_to_server(int sock, const char *filename) {
    char full_path[100];
    snprintf(full_path, sizeof(full_path), "client/secrets/%s", filename);
    
    FILE *file = fopen(full_path, "r");
    if (file == NULL) {
        printf("Error: File not found\n");
        return;
    }
    
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "DECRYPT %s", full_path);
    send(sock, command, strlen(command), 0);
    
    char buffer[BUFFER_SIZE] = {0};
    read(sock, buffer, BUFFER_SIZE);
    printf("%s\n", buffer);
}

void download_file_from_server(int sock, const char *filename) {
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "DOWNLOAD %s", filename);
    send(sock, command, strlen(command), 0);
    
    long fsize;
    read(sock, &fsize, sizeof(fsize));
    
    char *buffer = malloc(fsize);
    read(sock, buffer, fsize);
    
    char output_path[100];
    snprintf(output_path, sizeof(output_path), "client/%s", filename);
    
    FILE *file = fopen(output_path, "wb");
    if (file == NULL) {
        printf("Error: Failed to create output file\n");
        free(buffer);
        return;
    }
    
    fwrite(buffer, 1, fsize, file);
    fclose(file);
    free(buffer);
    
    printf("Success! Image saved as %s\n", filename);
}

int main() {
    int choice;
    int sock;
    char filename[100];
    
    while (1) {
        display_menu();
        scanf("%d", &choice);
        getchar(); // Consume newline
        
        switch (choice) {
            case 1:
                printf("Enter the file name: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\n")] = 0; // Remove newline
                
                sock = connect_to_server();
                if (sock < 0) {
                    printf("Failed to connect to server\n");
                    break;
                }
                
                send_file_to_server(sock, filename);
                close(sock);
                break;
                
            case 2:
                printf("Enter the file name: ");
                fgets(filename, sizeof(filename), stdin);
                filename[strcspn(filename, "\n")] = 0; // Remove newline
                
                sock = connect_to_server();
                if (sock < 0) {
                    printf("Failed to connect to server\n");
                    break;
                }
                
                download_file_from_server(sock, filename);
                close(sock);
                break;
                
            case 3:
                sock = connect_to_server();
                if (sock >= 0) {
                    send(sock, "EXIT", 4, 0);
                    close(sock);
                }
                printf("Exiting...\n");
                exit(0);
                
            default:
                printf("Invalid choice\n");
        }
    }
    
    return 0;
}
