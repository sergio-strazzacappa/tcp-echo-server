#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

using namespace std;

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
    int USERNAMELEN = 1024;
    int PASSWORDLEN = 1024;
    int MESSAGELEN = 1024;

    char username[USERNAMELEN];
    char password[PASSWORDLEN];
    char message[MESSAGELEN];

    while (1) {
        memset(username, 0, USERNAMELEN);
        memset(password, 0, PASSWORDLEN);
        memset(message, 0, MESSAGELEN);

        int bytes_received = recv(client_socket, message, MESSAGELEN - 1, 0);

        if (bytes_received < 0) {
            cerr << "[ERROR] Something went wrong while receiving data" << endl;
            break;
        }
        if (bytes_received == 0) {
            cout << "[INFO]  Client is disconnected" << endl;
            break;
        }
        if (bytes_received > 0) {
            // TODO: process the answer to the client
            cout << "Client> " << string(message, 0, bytes_received) << endl;

            if (send(client_socket, message, bytes_received, 0) < 0) {
                cerr << "[ERROR] Message can't be send" << endl;
                break;
            }
        }
    }

    close(server_fd);
    cout << "[INFO]  Listener socket is closed" << endl;

    close(client_socket);
    cout << "[INFO]  Client socket is closed" << endl;

    return 0;
}
