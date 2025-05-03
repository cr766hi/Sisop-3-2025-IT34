#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define LOG_FILE "server/server.log"

void log_action(const char *source, const char *action, const char *info) {
    time_t now;
    time(&now);
    struct tm *tm_info = localtime(&now);
    
    char timestamp[20];
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    
    FILE *log = fopen(LOG_FILE, "a");
    if (log == NULL) {
        perror("Failed to open log file");
        return;
    }
    
    fprintf(log, "[%s][%s]: [%s] [%s]\n", source, timestamp, action, info);
    fclose(log);
}

void handle_error(int client_socket, const char *error_msg) {
    send(client_socket, error_msg, strlen(error_msg), 0);
}

void decrypt_text(const char *text, char *output) {
 
    int len = strlen(text);
    for (int i = 0; i < len; i++) {
        output[i] = text[len - 1 - i];
    }
    output[len] = '\0';
    
}

void save_to_jpeg(const char *data, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Failed to open file for writing");
        return;
    }
    
    fwrite(data, 1, strlen(data), file);
    fclose(file);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    char client_ip[INET_ADDRSTRLEN];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    getpeername(client_socket, (struct sockaddr*)&client_addr, &addr_len);
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    
    int read_size = read(client_socket, buffer, BUFFER_SIZE);
    if (read_size <= 0) {
        perror("Read failed");
        close(client_socket);
        return;
    }
    
    if (strncmp(buffer, "DECRYPT", 7) == 0) {
        char *filename = buffer + 8;
        FILE *file = fopen(filename, "r");
        if (file == NULL) {
            handle_error(client_socket, "Error: File not found");
            log_action("Server", "ERROR", "File not found");
            close(client_socket);
            return;
        }
        
        fseek(file, 0, SEEK_END);
        long fsize = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        char *text = malloc(fsize + 1);
        fread(text, 1, fsize, file);
        fclose(file);
        
        char decrypted[fsize * 2];
        decrypt_text(text, decrypted);
        free(text);
        
        time_t timestamp = time(NULL);
        char jpeg_filename[50];
        snprintf(jpeg_filename, sizeof(jpeg_filename), "server/database/%ld.jpeg", timestamp);
        
        save_to_jpeg(decrypted, jpeg_filename);
        
        char response[100];
        snprintf(response, sizeof(response), "Server: Text decrypted and saved as %ld.jpeg", timestamp);
        send(client_socket, response, strlen(response), 0);
        
        log_action("Client", "DECRYPT", "Text data");
        log_action("Server", "SAVE", jpeg_filename);
    }
    else if (strncmp(buffer, "DOWNLOAD", 8) == 0) {
        char *filename = buffer + 9;
        char full_path[100];
        snprintf(full_path, sizeof(full_path), "server/database/%s", filename);
        
        FILE *file = fopen(full_path, "rb");
        if (file == NULL) {
            handle_error(client_socket, "Error: File not found for download");
            log_action("Server", "ERROR", "File not found for download");
            close(client_socket);
            return;
        }
        
        fseek(file, 0, SEEK_END);
        long fsize = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        char *file_data = malloc(fsize);
        fread(file_data, 1, fsize, file);
        fclose(file);
        
        send(client_socket, &fsize, sizeof(fsize), 0);
        
        send(client_socket, file_data, fsize, 0);
        free(file_data);
        
        log_action("Client", "DOWNLOAD", filename);
        log_action("Server", "UPLOAD", filename);
    }
    else if (strncmp(buffer, "EXIT", 4) == 0) {
        log_action("Client", "EXIT", "Client requested to exit");
    }
    
    close(client_socket);
}

int main() {
  
    mkdir("server", 0755);
    mkdir("server/database", 0755);

    pid_t pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    umask(0);
    
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
    
    log_action("Server", "START", "Server started");
    
    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        
        handle_client(client_socket);
    }
    
    return 0;
}
