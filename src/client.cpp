#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include "data.cpp"

#define PORT 8080

using namespace std;

void serialize_login_request(Login_request* login_request, char *data);
int send_login_request(int client_fd);
void deserialize_login_status(char *data, Login_status* login_status);

int main(int argc, char **argv) {
    int client_fd;
    sockaddr_in server_address;

    cout << "[INFO]  Client is starting" << endl;

    // create a socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "[ERROR] Can't create a socket" << endl;
        return -1;
    }

    cout << "[INFO]  Socket has been created" << endl;

    // bind the socket to a IP/port
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        cerr << "[ERROR] Invalid address" << endl;
        return -1;
    }

    // connect to the server
    if (connect(client_fd, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        cerr << "[ERROR] Connection cannot be established" << endl;
        return -2;
    }

    cout << "[INFO]  Connection established" << endl;

    // send login request
    send_login_request(client_fd);

    // recieve login status
    int MESSAGELEN = 1024;
    char message[MESSAGELEN];
    memset(message, 0, MESSAGELEN);

    int bytes_received = recv(client_fd, message, MESSAGELEN, 0);

    Login_status* login_status = new Login_status;

    deserialize_login_status(message, login_status);

    cout << login_status->header.message_size << endl;
    cout << login_status->header.message_type << endl;
    cout << login_status->header.message_sequence << endl;
    cout << (int)login_status->status << endl;

    if (bytes_received < 0) {
        cerr << "[ERROR] Message cannot be received" << endl;
        return -3; 
    }

    close(client_fd);
    cout << "[INFO]  Client socket is closed" << endl;

    return 0;
}

int send_login_request(int client_fd) {
    Header login_request_header {
        .message_size = 68,
        .message_type = 0,
        .message_sequence = 1
    };

    Login_request* login_request = new Login_request;
    login_request->header = login_request_header;
    login_request->username = "user1";
    login_request->username.resize(32, '\0');
    login_request->password = "pass1";
    login_request->password.resize(32, '\0');
    
    char data[1024];
    serialize_login_request(login_request, data);

    send(client_fd, data, 1024, 0);

    cout  << "[INFO]  Client has sent login request" << endl;

    return 0;
}

void serialize_login_request(Login_request* login_request, char *data) {
    int *q = (int*)data;

    *q = login_request->header.message_size;
    q++;

    *q = login_request->header.message_type;
    q++;

    *q = login_request->header.message_sequence;
    q++;
    
    char *p = (char*)q;
    int i = 0;

    while (i < 32) {
        char *r = (char*)p + 32;
        *p = login_request->username[i];
        *r = login_request->password[i];
        p++;
        i++;
    }
}

void deserialize_login_status(char *data, Login_status* login_status) {
    int *q = (int*)data;

    login_status->header.message_size = *q;
    q++;

    login_status->header.message_type = *q;
    q++;

    login_status->header.message_sequence = *q;
    q++;

    login_status->status = *q;
}
