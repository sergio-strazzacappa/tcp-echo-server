#include <iostream>
#include <string>

#define PORT 8080

using namespace std;

typedef struct Header {
    uint16_t message_size;
    uint16_t message_type;
    uint16_t message_sequence;
} Header;

typedef struct Login_request {
        Header header;
        string username;
        string password;
} Login_request;

typedef struct Login_status {
    Header header;
    uint16_t status;
} Login_status;

typedef struct Cipher_message {
    Header header;
    uint16_t message_size;
    string message;
} Cipher_message;

void info_message(string message) {
    cout << "[INFO]  " << message << endl;
}

void error_message(string message) {
    cerr << "[ERROR] " << message << endl;
}
