#include <string>

using namespace std;

typedef struct Header {
    unsigned int message_size;
    unsigned int message_type;
    unsigned int message_sequence;
} Header;

typedef struct Login_request {
        Header header;
        string username;
        string password;
} Login_request;

typedef struct Login_status {
    Header header;
    char status;
} Login_status;
