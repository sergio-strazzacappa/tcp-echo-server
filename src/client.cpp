#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include "data.cpp"

using namespace std;

void send_login_request(int client_fd, string user, string password);
int receive_login_status(int client_fd);
void serialize_login_request(Login_request* login_request, char *data);
void deserialize_login_status(char *data, Login_status* login_status);
void send_message(int client_fd);
void serialize_message(Cipher_message* cipher_message, char *data);

int main(int argc, char **argv) {
    int client_fd;
    sockaddr_in server_address;
    string msg;

    msg = "Client is starting"; 
    info_message(msg);

    if (argc != 3) {
        msg = "Usage: " + string(argv[0]) + " <user><pass>";
        error_message(msg);
        return -1;
    }

    // create a socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        msg = "Can't create a socket";
        error_message(msg);
        return -2;
    }

    msg = "Socket has been created";
    info_message(msg);

    // bind the socket to a IP/port
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        msg = "Invalid address";
        error_message(msg);
        return -3;
    }

    // connect to the server
    if (connect(client_fd, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        msg = "Connection cannot be established";
        error_message(msg);
        return -4;
    }

    msg = "Connection established";
    info_message(msg);

    // send login request
    send_login_request(client_fd, argv[1], argv[2]);

    // receive login status
    receive_login_status(client_fd);

    msg = "CLient socket is closed";
    info_message(msg);
    
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    // connect to the server
    if (connect(client_fd, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        msg = "Connection cannot be established";
        error_message(msg);
        return -4;
    }

    // send cipher message
    send_message(client_fd);

    close(client_fd);
    return 0;
}

void send_login_request(int client_fd, string user, string password) {
    Header login_request_header {
        .message_size = 68,
        .message_type = 0,
        .message_sequence = 1
    };

    Login_request* login_request = new Login_request;
    login_request->header = login_request_header;
    login_request->username = user;
    login_request->username.resize(4, '\0');
    login_request->password = password;
    login_request->password.resize(4, '\0');
    
    char data[1024];
    serialize_login_request(login_request, data);

    send(client_fd, data, 1024, 0);

    string msg = "Client has sent login request";
    info_message(msg);
}

int receive_login_status(int client_fd) {
    int MESSAGELEN = 1024;
    char message[MESSAGELEN];
    memset(message, 0, MESSAGELEN);

    int bytes_received = recv(client_fd, message, MESSAGELEN, 0);

    Login_status* login_status = new Login_status;

    deserialize_login_status(message, login_status);

    cout << "Server> Header msg size: " << login_status->header.message_size << endl;
    cout << "Server> Header msg type: " << login_status->header.message_type << endl;
    cout << "Server> Header msg sequence: " << login_status->header.message_sequence << endl;
    cout << "Server> Login status: " << (int)login_status->status << endl;

    if (bytes_received < 0) {
        string msg = "Message cannot be received";
        error_message(msg);
        return -5; 
    }
    return 0;
}

void serialize_login_request(Login_request* login_request, char *data) {
    uint16_t *q = (uint16_t*)data;

    *q = login_request->header.message_size;
    q++;

    *q = login_request->header.message_type >> 8;
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
    uint16_t *q = (uint16_t*)data;

    login_status->header.message_size = *q;
    q++;

    uint8_t *s = (uint8_t*)q;

    login_status->header.message_type = *s;
    s++;

    login_status->header.message_sequence = *s;
    s++;

    login_status->status = *s;
}

void send_message(int client_fd) {
    Header message_header {
        .message_size = 4,
        .message_type = 2,
        .message_sequence = 15
    };

    Cipher_message* cipher_message = new Cipher_message;
    cipher_message->header = message_header;
    cipher_message->message = "1239";
    cipher_message->message_size = sizeof(cipher_message->message);
    message_header.message_size += cipher_message->message_size;

    char data[1024];
    serialize_message(cipher_message, data);

    send(client_fd, data, 1024, 0);

    string msg = "Client has sent cipher message";
    info_message(msg);
}

void serialize_message(Cipher_message* cipher_message, char *data) {
    uint16_t *q = (uint16_t*)data;

    *q = cipher_message->header.message_size;
    q++;

    *q = cipher_message->header.message_type;
    q++;

    *q = cipher_message->header.message_sequence;
    q++;
    
    *q = cipher_message->message_size;
    q++;

    char *p = (char*)q;
    int i = 0;

    while (i < 32) {
        *p = cipher_message->message[i];
        p++;
        i++;
    }
}

