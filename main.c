#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

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
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/records") == 0) {
        char json[BUFFER_SIZE] = "[";
        for (int i = 0; i < record_count; i++) {
            char record[256];
            snprintf(record, sizeof(record), 
                     "{\"id\":%d,\"name\":\"%s\",\"age\":%d}%s", 
                     database[i].id, database[i].name, database[i].age,
                     i < record_count - 1 ? "," : "");
            strcat(json, record);
        }
        strcat(json, "]");
        send_response(client_socket, json, "application/json");
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/add") == 0) {
        int id, age;
        char name[50];
        sscanf(body, "{\"id\":%d,\"name\":\"%49[^\"]\",\"age\":%d}", &id, name, &age);
        insert_record(id, name, age);
        send_response(client_socket, "{\"status\":\"success\"}", "application/json");
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/update") == 0) {
        int id, age;
        char name[50];
        sscanf(body, "{\"id\":%d,\"name\":\"%49[^\"]\",\"age\":%d}", &id, name, &age);
        update_record(id, name, age);
        send_response(client_socket, "{\"status\":\"success\"}", "application/json");
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/delete") == 0) {
        int id;
        sscanf(body, "{\"id\":%d}", &id);
        delete_record(id);
        send_response(client_socket, "{\"status\":\"success\"}", "application/json");
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/save") == 0) {
        char filename[255];
        sscanf(body, "{\"filename\":\"%254[^\"]\"}", filename);
        save_database(filename);
        send_response(client_socket, "{\"status\":\"success\"}", "application/json");
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/load") == 0) {
        char filename[255];
        sscanf(body, "{\"filename\":\"%254[^\"]\"}", filename);
        load_database(filename);
        send_response(client_socket, "{\"status\":\"success\"}", "application/json");
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
        "table { width: 100%%; border-collapse: collapse; margin-bottom: 20px; }"
        "th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }"
        "th { background-color: #3498db; color: white; }"
        "tr:hover { background-color: #f5f5f5; }"
        ".form-group { margin-bottom: 15px; }"
        "label { display: block; margin-bottom: 5px; }"
        "input[type='text'], input[type='number'] { width: 100%%; padding: 8px; margin-top: 5px; display: inline-block; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }"
        "button { background-color: #3498db; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; }"
        "button:hover { background-color: #2980b9; }"
        ".actions { display: flex; justify-content: space-between; margin-top: 20px; }"
        "#overlay { position: fixed; top: 0; left: 0; width: 100%%; height: 100%%; background-color: rgba(0,0,0,0.5); display: none; justify-content: center; align-items: center; }"
        "#popup { background-color: white; padding: 20px; border-radius: 5px; }"
        ".record-item { background-color: white; padding: 10px; margin-bottom: 10px; border-radius: 4px; box-shadow: 0 1px 3px rgba(0,0,0,0.12); }"
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
        "<div class='actions'>"
        "<div>"
        "<button onclick='showSavePopup()'>Save Database</button>"
        "<button onclick='showLoadPopup()'>Load Database</button>"
        "</div>"
        "</div>"
        "</div>"
        "<div id='overlay'>"
        "<div id='popup'>"
        "<h2 id='popupTitle'></h2>"
        "<input type='text' id='filename' placeholder='Enter filename'>"
        "<button onclick='confirmAction()'>Confirm</button>"
        "<button onclick='closePopup()'>Cancel</button>"
        "</div>"
        "</div>"
        "<script>"
        "let records = [];"
        "let currentAction = '';"
        "function fetchRecords() {"
        "  fetch('/api/records')"
        "    .then(response => response.json())"
        "    .then(data => {"
        "      records = data;"
        "      updateSidebar();"
        "    });"
        "}"
        "function updateSidebar() {"
        "  const recordList = document.getElementById('recordList');"
        "  recordList.innerHTML = '';"
        "  records.forEach(record => {"
        "    const div = document.createElement('div');"
        "    div.className = 'record-item';"
        "    div.innerHTML = "
        "      <strong>${record.name}</strong> (ID: ${record.id})<br>"
        "      Age: ${record.age}<br>"
        "      <button onclick='editRecord(${record.id})'>Edit</button>"
        "      <button onclick='deleteRecord(${record.id})'>Delete</button>"
        "    ;"
        "    recordList.appendChild(div);"
        "  });"
        "}"
        "document.getElementById('recordForm').addEventListener('submit', function(e) {"
        "  e.preventDefault();"
        "  const id = document.getElementById('id').value;"
        "  const name = document.getElementById('name').value;"
        "  const age = document.getElementById('age').value;"
        "  const record = { id: parseInt(id), name, age: parseInt(age) };"
        "  const url = records.some(r => r.id === record.id) ? '/api/update' : '/api/add';"
        "  fetch(url, {"
        "    method: 'POST',"
        "    headers: { 'Content-Type': 'application/json' },"
        "    body: JSON.stringify(record)"
        "  }).then(() => {"
        "    fetchRecords();"
        "    document.getElementById('recordForm').reset();"
        "  });"
        "});"
        "function editRecord(id) {"
        "  const record = records.find(r => r.id === id);"
        "  document.getElementById('id').value = record.id;"
        "  document.getElementById('name').value = record.name;"
        "  document.getElementById('age').value = record.age;"
        "}"
        "function deleteRecord(id) {"
        "  fetch('/api/delete', {"
        "    method: 'POST',"
        "    headers: { 'Content-Type': 'application/json' },"
        "    body: JSON.stringify({ id })"
        "  }).then(() => fetchRecords());"
        "}"
        "function showSavePopup() {"
        "  currentAction = 'save';"
        "  document.getElementById('popupTitle').textContent = 'Save Database';"
        "  document.getElementById('overlay').style.display = 'flex';"
        "}"
        "function showLoadPopup() {"
        "  currentAction = 'load';"
        "  document.getElementById('popupTitle').textContent = 'Load Database';"
        "  document.getElementById('overlay').style.display = 'flex';"
        "}"
        "function closePopup() {"
        "  document.getElementById('overlay').style.display = 'none';"
        "  document.getElementById('filename').value = '';"
        "}"
        "function confirmAction() {"
        "  const filename = document.getElementById('filename').value;"
        "  if (!filename) {"
        "    alert('Please enter a filename');"
        "    return;"
        "  }"
        "  fetch(/api/${currentAction}, {"
        "    method: 'POST',"
        "    headers: { 'Content-Type': 'application/json' },"
        "    body: JSON.stringify({ filename })"
        "  }).then(() => {"
        "    closePopup();"
        "    if (currentAction === 'load') fetchRecords();"
        "    alert(Database ${currentAction}d successfully);"
        "  });"
        "}"
        "fetchRecords();"
        "</script>"
        "</body>"
        "</html>");
}

void insert_record(int id, const char* name, int age) {
    if (record_count < MAX_RECORDS) {
        database[record_count].id = id;
        strncpy(database[record_count].name, name, sizeof(database[record_count].name) - 1);
        database[record_count].name[sizeof(database[record_count].name) - 1] = '\0';
        database[record_count].age = age;
        record_count++;
    }
}

void update_record(int id, const char* name, int age) {
    for (int i = 0; i < record_count; i++) {
        if (database[i].id == id) {
            strncpy(database[i].name, name, sizeof(database[i].name) - 1);
            database[i].name[sizeof(database[i].name) - 1] = '\0';
            database[i].age = age;
            break;
        }
    }
}

void delete_record(int id) {
    for (int i = 0; i < record_count; i++) {
        if (database[i].id == id) {
            for (int j = i; j < record_count - 1; j++) {
                database[j] = database[j + 1];
            }
            record_count--;
            break;
        }
    }
}

void save_database(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file for writing");
        return;
    }
    for (int i = 0; i < record_count; i++) {
        fprintf(file, "%d,%s,%d\n", database[i].id, database[i].name, database[i].age);
    }
    fclose(file);
}

void load_database(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file for reading");
        return;
    }
    record_count = 0;
    while (fscanf(file, "%d,%49[^,],%d\n", &database[record_count].id, database[record_count].name, &database[record_count].age) == 3) {
        record_count++;
        if (record_count >= MAX_RECORDS) break;
    }
    fclose(file);
}
