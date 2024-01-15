#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT 8080

using namespace std;

int send_messages(int client_fd);

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

    // send messages
    send_messages(client_fd);

    close(client_fd);
    cout << "[INFO]  Client socket is closed" << endl;

    return 0;
}

int send_messages(int client_fd) {
    send(client_fd, "Hello world!", strlen("Hello world!"), 0);
    return 0;
}
