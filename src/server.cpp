#include <iostream>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "data.cpp"

#define PORT 8080

using namespace std;

string client_username = "user1";
string client_password = "pass1";

void deserialize_login_request(char *data, Login_request* login_request);
void serialize_login_status(Login_status* login_status, char* data);
char check_login(string username, string password);

int main(int argc, char **argv) {
    int server_fd;
    sockaddr_in server_address;
    sockaddr_in client;
    socklen_t client_size = sizeof(client);
    int opt = -1;
    char buf[INET_ADDRSTRLEN];

    // create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "[ERROR] Can't create a socket" << endl;
        return -1;
    }

    cout << "[INFO]  Socket has been created" << endl;

    // configure the socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        cerr << "[ERROR] Cant' configure the socket" << endl;
        return -2;
    }

    // bind the socket to a IP/port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY; // localhost
    server_address.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr *)&server_address, sizeof(server_address)) < 0) {
        cerr << "[ERROR] Created socket cannot be binded to ("
             << inet_ntop(AF_INET, &server_address.sin_addr, buf, INET_ADDRSTRLEN)
             << ":" << ntohs(server_address.sin_port) << ")" << endl;
        return -3;
    }

    cout << "[INFO]  Socket is binded to ("
         << inet_ntop(AF_INET, &server_address.sin_addr, buf, INET_ADDRSTRLEN)
         << ":" << ntohs(server_address.sin_port) << ")" << endl;

    // listen
    if (listen(server_fd, SOMAXCONN) < 0) {
        cerr << "[ERROR] Socket cannot be switched to listen mode" << endl;
        return -4;
    } 

    cout << "[INFO]  Socket is listening now" << endl;

    // accept a call
    int client_socket = accept(server_fd, (sockaddr *)&client, &client_size);

    if (client_socket < 0) {
        cerr << "[ERROR] Connections cannot be accepted for a reason" << endl;
        return -5;
    }

    string client_ip = inet_ntoa(client.sin_addr);
    int client_port = ntohs(client.sin_port);

    cout << "[INFO]  Accepted new client @ " << client_ip << ":" << client_port << endl;

    // receive from the client
    int MESSAGELEN = 1024;
    char message[MESSAGELEN];
    memset(message, 0, MESSAGELEN);

    int bytes_received = recv(client_socket, message, MESSAGELEN - 1, 0);

    if (bytes_received < 0) {
        cerr << "[ERROR] Something went wrong while receiving data" << endl;
    }
    if (bytes_received == 0) {
        cout << "[INFO]  Client is disconnected" << endl;
    }
    if (bytes_received > 0) {
        Login_request* login_request = new Login_request;
        deserialize_login_request(message, login_request);

        cout << "Client> " << login_request->header.message_size << endl;
        cout << "Client> " << login_request->header.message_type << endl;
        cout << "Client> " << login_request->header.message_sequence << endl;
        cout << "Client> " << login_request->username << endl;
        cout << "Client> " << login_request->password << endl;

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
        
        printf("%s\n", data);

        if (send(client_socket, data, 1024, 0) < 0) {
            cerr << "[ERROR] Message can't be send" << endl;
        } else {
            cout << "[INFO]  Login status sent to client" << endl;
        }

        if (status) {
            cout << "[INFO]  Login is correct" << endl;
        } else {
            close(client_socket);
            cout << "[ERROR] Login is incorrect" << endl;
            cout << "[INFO]  Client socket is closed" << endl;
        }
    }

    close(server_fd);
    cout << "[INFO]  Listener socket is closed" << endl;

    close(client_socket);
    cout << "[INFO]  Client socket is closed" << endl;

    return 0;
}

void deserialize_login_request(char *data, Login_request* login_request){
    int *q = (int*)data;

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

    while (i < 32) {
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
    client_username.resize(32, '\0');
    client_password.resize(32, '\0');
    
    if (client_username.compare(username) == 0 && 
        client_password.compare(password) == 0) {
            return 1; // OK
    }
    return 0; // FAILED
}

void serialize_login_status(Login_status* login_status, char* data) {
    int *q = (int*)data;

    *q = login_status->header.message_size;
    q++;

    *q = login_status->header.message_type;
    q++;

    *q = login_status->header.message_sequence;
    q++;

    char *p = (char*)q;

    *p = login_status->status;
}
