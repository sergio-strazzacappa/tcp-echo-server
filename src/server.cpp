#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "data.cpp"

using namespace std;

string client_username = "user";
string client_password = "pass";
string username;
string password;
uint16_t message_sequence; 

void receive_login_request(int client_socket);
void deserialize_login_request(char *data, Login_request* login_request);
void serialize_login_status(Login_status* login_status, char* data);
char check_login(string username, string password);
void receive_message(int client_socket);
void deserialize_ciphered_message(char *data, Cipher_message* cipher_message);
string decode(string message);
uint32_t next_key(uint32_t key);

int main(int argc, char **argv) {
    int server_fd;
    sockaddr_in server_address;
    sockaddr_in client;
    socklen_t client_size = sizeof(client);
    int opt = -1;
    char buf[INET_ADDRSTRLEN];
    string msg;

    // create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        msg = "Can't create a socket";
        error_message(msg);
        return -1;
    }

    msg = "Socket has been created";
    info_message(msg);

    // configure the socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        msg = "Can't configure the socket";
        error_message(msg);
        return -2;
    }

    // bind the socket to a IP/port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY; // localhost
    server_address.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr *)&server_address, sizeof(server_address)) < 0) {
        msg = "Created socket cannot be binded";
        error_message(msg);
        return -3;
    }

    msg = "Socket is binded";
    info_message(msg);

    // listen
    if (listen(server_fd, SOMAXCONN) < 0) {
        msg = "Socket cannot be switched to listen mode";
        error_message(msg);
        return -4;
    } 

    msg = "Socket is listening now";
    info_message(msg);

    // accept a call
    int client_socket = accept(server_fd, (sockaddr *)&client, &client_size);

    if (client_socket < 0) {
        msg = "Connections cannot be accepted for a reason";
        error_message(msg);
        return -5;
    }

    string client_ip = inet_ntoa(client.sin_addr);
    int client_port = ntohs(client.sin_port);

    msg = "Accepted new client @ " + client_ip + ":" + to_string(client_port);
    info_message(msg);

    // receive login request
    receive_login_request(client_socket);

    client_socket = accept(server_fd, (sockaddr *)&client, &client_size);

    // receive ciphered message
    receive_message(client_socket);

    close(server_fd);
    msg = "Listener socket is closed";
    info_message(msg);

    close(client_socket);
    msg = "Client socket is closed";
    info_message(msg);

    return 0;
}

void receive_login_request(int client_socket) {
    int MESSAGELEN = 1024;
    char message[MESSAGELEN];
    memset(message, 0, MESSAGELEN);
    int bytes_received = recv(client_socket, message, MESSAGELEN - 1, 0);
    string msg;

    if (bytes_received < 0) {
        msg = "Something went wrong while receiving data";
        error_message(msg);
    }
    if (bytes_received == 0) {
        msg = "Client is disconnected";
        info_message(msg);
    }
    if (bytes_received > 0) {
        Login_request* login_request = new Login_request;
        deserialize_login_request(message, login_request);
        
        cout << "Client> Header msg size: " << login_request->header.message_size << endl;
        cout << "Client> Header msg type: " << login_request->header.message_type << endl;
        cout << "Client> Header msg sequence: " << login_request->header.message_sequence << endl;
        cout << "Client> Username: " << login_request->username << endl;
        cout << "Client> Password: " << login_request->password << endl;

        username = login_request->username;
        password = login_request->password;
        message_sequence = login_request->header.message_sequence;

        char status = check_login(login_request->username, login_request->password);
        char data[1024];

        Header login_status_header {
            .message_size = 6,
            .message_type = 1,
            .message_sequence = 1
        };

        Login_status* login_status = new Login_status;
        login_status->header = login_status_header;
        login_status->status = status;

        serialize_login_status(login_status, data);
        
        if (send(client_socket, data, 1024, 0) < 0) {
            msg = "Message can't be send";
            error_message(msg);
        } else {
            msg = "Login status sent to the client";
            info_message(msg);
        }

        if (status) {
            msg = "Login is correct";
            info_message(msg);
        } else {
            close(client_socket);
            msg = "Login is incorrect";
            error_message(msg);
            msg = "Client socket is closed";
            info_message(msg);
        }
    }
}

void deserialize_login_request(char *data, Login_request* login_request){
    uint16_t *q = (uint16_t*)data;

    login_request->header.message_size = *q;
    q++;

    login_request->header.message_type = *q;
    q++;

    login_request->header.message_sequence = *q;
    q++;

    char *p = (char*)q;
    int i = 0;
    
    string tmp_user = "";
    string tmp_pass = "";
    char c;

    while (i < 4) {
        c = *p;
        tmp_user.push_back(c);
        char *r = (char*)p + 32;
        c = *r;
        tmp_pass.push_back(c);
        p++;
        i++;
    }
    login_request->username = tmp_user;
    login_request->password = tmp_pass;
}

char check_login(string username, string password) {
    client_username.resize(4, '\0');
    client_password.resize(4, '\0');
    
    if (client_username.compare(username) == 0 && 
        client_password.compare(password) == 0) {
            return 1; // OK
    }
    return 0; // FAILED
}

void serialize_login_status(Login_status* login_status, char* data) {
    uint16_t *q = (uint16_t*)data;

    *q = login_status->header.message_size;
    q++;

    uint8_t *s = (uint8_t*)q;

    *s = login_status->header.message_type;
    s++;

    *s = login_status->header.message_sequence;
    s++;

    char *p = (char*)s;

    *p = login_status->status;
}

void receive_message(int client_socket) {
    int MESSAGELEN = 1024;
    char message[MESSAGELEN];
    memset(message, 0, MESSAGELEN);
    int bytes_received = recv(client_socket, message, MESSAGELEN - 1, 0);
    string msg;

    if (bytes_received < 0) {
        msg = "Something went wrong while receiving data";
        error_message(msg);
    }
    if (bytes_received == 0) {
        msg = "Client is disconnected";
        info_message(msg);
    }
    if (bytes_received > 0) {
        Cipher_message* cipher_message = new Cipher_message;
        deserialize_ciphered_message(message, cipher_message);
        
        cout << "Client> Header msg size: " << cipher_message->header.message_size << endl;
        cout << "Client> Header msg type: " << cipher_message->header.message_type << endl;
        cout << "Client> Header msg sequence: " << cipher_message->header.message_sequence << endl;
        cout << "Client> Message size: " << cipher_message->message_size << endl;
        cout << "Client> Message: " << cipher_message->message << endl;

        string plain_text = decode(cipher_message->message);

        char data[1024];

        Header login_status_header {
            .message_size = 6,
            .message_type = 3,
            .message_sequence = 1
        };
    }
}

void deserialize_ciphered_message(char *data, Cipher_message* cipher_message){
    uint16_t *q = (uint16_t*)data;

    cipher_message->header.message_size = *q;
    q++;

    cipher_message->header.message_type = *q;
    q++;

    cipher_message->header.message_sequence = *q;
    q++;

    cipher_message->message_size = *q;
    q++;

    char *p = (char*)q;
    int i = 0;
    
    string msg = "";
    char c;

    while (i < 4) {
        c = *p;
        msg.push_back(c);
        p++;
        i++;
    }
    cipher_message->message = msg;
}

string decode(string message) {
    uint8_t username1 = username[0];
    uint8_t username2 = username[1];
    uint8_t username3 = username[2];
    uint8_t username4 = username[3];
    uint32_t username_checksum = ~(username1 + username2 + username3 + username4);

    uint8_t password1 = password[0];
    uint8_t password2 = password[1];
    uint8_t password3 = password[2];
    uint8_t password4 = password[3];
    uint32_t password_checksum = ~(password1 + password2 + password3 + password4);

    uint32_t initial_key = (message_sequence << 16) | (username_checksum) << 8 | password_checksum;

    char m[32];
    for (int i = 0; i < message.size(); i++) {
        char c = message[i] ^ next_key(initial_key);
        m[i] = ~c;
    }
    info_message("Server has decoded the message");
    return m;
}

uint32_t next_key(uint32_t key) {
    return (key * 1103515245 + 12345) % 0X7FFFFFFF;
}
