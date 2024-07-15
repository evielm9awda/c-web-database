#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 16384
#define MAX_RECORDS 100

typedef struct {
    int id;
    char name[50];
    int age;
} Record;

Record database[MAX_RECORDS];
int record_count = 0;

void handle_request(int client_socket, char* method, char* path, char* body);
void send_response(int client_socket, const char* content, const char* content_type);
void generate_html(char* html, size_t size);
void insert_record(int id, const char* name, int age);
void update_record(int id, const char* name, int age);
void delete_record(int id);
void save_database(const char* filename);
void load_database(const char* filename);

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server running on http://localhost:%d\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        char buffer[BUFFER_SIZE] = {0};
        read(new_socket, buffer, BUFFER_SIZE);

        char method[10], path[255], http_version[20];
        sscanf(buffer, "%s %s %s", method, path, http_version);

        char* body = strstr(buffer, "\r\n\r\n");
        if (body) body += 4;

        handle_request(new_socket, method, path, body);
        close(new_socket);
    }

    return 0;
}

void handle_request(int client_socket, char* method, char* path, char* body) {
    if (strcmp(method, "GET") == 0 && strcmp(path, "/") == 0) {
        char html[BUFFER_SIZE];
        generate_html(html, sizeof(html));
        send_response(client_socket, html, "text/html");
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/save") == 0) {
        save_database("database.txt");
        send_response(client_socket, "Database saved", "text/plain");
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/load") == 0) {
        load_database("database.txt");
        send_response(client_socket, "Database loaded", "text/plain");
    } else {
        send_response(client_socket, "404 Not Found", "text/plain");
    }
}

void send_response(int client_socket, const char* content, const char* content_type) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %lu\r\n"
             "\r\n"
             "%s",
             content_type, strlen(content), content);
    write(client_socket, response, strlen(response));
}

void generate_html(char* html, size_t size) {
    char* p = html;
    p += snprintf(p, size,
        "<!DOCTYPE html>"
        "<html lang='en'>"
        "<head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<title>Database Management System</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; line-height: 1.6; color: #333; margin: 0; padding: 0; display: flex; }"
        "#sidebar { width: 300px; background-color: #f4f4f4; padding: 20px; height: 100vh; overflow-y: auto; }"
        "#main { flex-grow: 1; padding: 20px; }"
        "h1, h2 { color: #2c3e50; }"
        "</style>"
        "</head>"
        "<body>"
        "<div id='sidebar'>"
        "<h2>Records</h2>"
        "<div id='recordList'></div>"
        "</div>"
        "<div id='main'>"
        "<h1>Database Management System</h1>"
        "<h2>Add/Edit Record</h2>"
        "<form id='recordForm'>"
        "<div class='form-group'>"
        "<label for='id'>ID:</label>"
        "<input type='number' id='id' required>"
        "</div>"
        "<div class='form-group'>"
        "<label for='name'>Name:</label>"
        "<input type='text' id='name' required>"
        "</div>"
        "<div class='form-group'>"
        "<label for='age'>Age:</label>"
        "<input type='number' id='age' required>"
        "</div>"
        "<button type='submit'>Add/Update Record</button>"
        "</form>"
        "</div>"
        "<script>"
        "document.getElementById('recordForm').addEventListener('submit', function(e) {"
        "  e.preventDefault();"
        "  // Add JavaScript to handle form submission"
        "});"
        "</script>"
        "</body>"
        "</html>");
}

void save_database(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Failed to save database");
        return;
    }

    for (int i = 0; i < record_count; i++) {
        fprintf(file, "%d,%s,%d\n", database[i].id, database[i].name, database[i].age);
    }

    fclose(file);
}

void load_database(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to load database");
        return;
    }

    record_count = 0;
    while (fscanf(file, "%d,%49[^,],%d\n", &database[record_count].id, database[record_count].name, &database[record_count].age) == 3) {
        record_count++;
        if (record_count >= MAX_RECORDS) {
            break;
        }
    }

    fclose(file);
}
