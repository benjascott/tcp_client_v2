#include "log.h"

#include "tcp_client.h"

#include <sys/types.h>
#include <sys/socket.h>

/*
print message to clarify usage of the commandline tool
*/
void tcp_client_printHelpMessage() {
    printf("\nUsage: tcp_client [--help] [-v] [-h HOST] [-p PORT] ACTION MESSAGE\n\n");
    printf("Arguments:\n  ACTION\tMust be uppercase, lowercase, title-case,\n\t\treverse, or shuffle.\n  MESSAGE\tMessage to send to the server\n");
    printf("Options:\n  --help\n  -v, --verbose\n  --host HOSTNAME, -h HOSTNAME\n  --port PORT, -p PORT\n");
}

// allocate memory for the configuration details
void tcp_client_allocate_config(Config *config) {
    config->host = malloc(sizeof(char)*(100+1));
    config->port = malloc(sizeof(char)*(5+1));
    config->file = malloc(sizeof(char)*(100+1));
    log_trace("memory allocated");
}

/*
Description:
    Parses the commandline arguments and options given to the program.
Arguments:
    int argc: the amount of arguments provided to the program (provided by the main function)
    char *argv[]: the array of arguments provided to the program (provided by the main function)
    Config *config: An empty Config struct that will be filled in by this function.
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_parse_arguments(int argc, char *argv[], Config *config) {
    int c;

    strcpy(config->port, TCP_CLIENT_DEFAULT_PORT);
    strcpy(config->host, TCP_CLIENT_DEFAULT_HOST);
    log_trace("enter parse args");
    while(1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, NULL, 0},
            {"verbose", no_argument, NULL, 'v'},
            {"host", required_argument, NULL, 'h'},
            {"port", required_argument, NULL, 'p'},
            {NULL, 0, NULL, 0}
        };
        c = getopt_long(argc, argv, "vh:p:", long_options, &option_index);
        if(c == -1) {
            break;
        }

        switch (c) {
            case 0:
                tcp_client_printHelpMessage();
                break;
            case 'v':
                log_set_level(LOG_TRACE);
                break;
            case 'h':
                log_info("host: %s", optarg);
                strcpy(config->host, optarg);
                break;
            case 'p':
                log_info("port: %s", optarg);
                strcpy(config->port, optarg);
                break;
            case '?':
                log_info("unrecognized option, exiting program");
                tcp_client_printHelpMessage();
                return 1;
        }
    }
    if (optind < argc) {
        strcpy(config->file, argv[optind++]);
        log_info("File: %s", config->file);    
    }

    else {
        log_debug("Incorrect number of arguments");
        tcp_client_printHelpMessage();
        return 1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////
/////////////////////// SOCKET RELATED FUNCTIONS //////////////////////
///////////////////////////////////////////////////////////////////////

/*
Description:
    Creates a TCP socket and connects it to the specified host and port.
Arguments:
    Config config: A config struct with the necessary information.
Return value:
    Returns the socket file descriptor or -1 if an error occurs.
*/
int tcp_client_connect(Config config) {
    struct addrinfo hints, *servinfo, *p;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(config.host, config.port, &hints, &servinfo);

    log_info("Searching for a socket to connect to");
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
                continue;
        }
        log_info("Attempting to connect to a socket");
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo);
    if (p == NULL) {
        log_debug("No available sockets to connect to");
        return -1;
    }
    log_info("Connected to a socket");
    return sockfd;
}

/*
Helper function that assures that all data is sent before returning
*/
int sendAll(int sockfd, char *action, char *message) {
    
    int len, sent;
    char *msg = malloc(strlen(action)+4+strlen(message));
    char str[5];
    sprintf(str, "%lu", strlen(message));
    strcpy(msg, action);
    strcat(msg, " ");
    strcat(msg, str);
    strcat(msg, " ");
    strcat(msg, message);
    len = strlen(msg);

    log_info("Message being sent to the server: %s", msg);
    sent = send(sockfd, msg, len, 0);
    if(sent == -1) {
        log_warn("Data was not successfully sent to the server");
        return -1;
    }
    return 0;
}

/*
Description:
    Creates and sends request to server using the socket and configuration.
Arguments:
    int sockfd: Socket file descriptor
    Config config: A config struct with the necessary information.
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_send_request(int sockfd, char *action, char *message) {
    log_info("Sending data to the server");
    return sendAll(sockfd, action, message);
}

/*
Description:
    Receives the response from the server. The caller must provide an already allocated buffer.
Arguments:
    int sockfd: Socket file descriptor
    char *buf: An already allocated buffer to receive the response in
    int buf_size: The size of the allocated buffer
Return value:
    Returns a 1 on failure, 0 on success
*/
// int tcp_client_receive_response(int sockfd, int (*handle_response)(char *)) {
//     // log_info("Receiving data from the server");
//     // int numbytes = recv(sockfd, buf, buf_size, 0);
//     // if(numbytes == -1) {
//     //     return 1;
//     // }
//     // return 0;
// }

/*
Description:
    Closes the given socket.
Arguments:
    int sockfd: Socket file descriptor
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_close(int sockfd) {
    log_info("Closing the socket connection");
    if(close(sockfd) == -1) {
        log_warn("Failed to close connection");
        return 1;
    }
    log_info("Successfully closed socket");
    return 0;
}

/*
Description:
    Opens a file.
Arguments:
    char *file_name: The name of the file to open
Return value:
    Returns NULL on failure, a FILE pointer on success
*/
FILE *tcp_client_open_file(char *file_name) {
    FILE *f = fopen(file_name, "r");
    return f;
}

/*
Description:
    Gets the next line of a file, filling in action and message. This function should be similar
    design to getline() (https://linux.die.net/man/3/getline). *action and message must be allocated
    by the function and freed by the caller.* When this function is called, action must point to the
    action string and the message must point to the message string.
Arguments:
    FILE *fd: The file pointer to read from
    char **action: A pointer to the action that was read in
    char **message: A pointer to the message that was read in
Return value:
    Returns -1 on failure, the number of characters read on success
*/
int tcp_client_get_line(FILE *fd, char **action, char **message) {
    char s;
    while((s=fgetc(fd))!=EOF) {
      printf("%c",s);
    }
    action = malloc(sizeof(char)*(13+1));
    message = malloc(sizeof(char)*(1024+1));
    int read = fscanf(fd, "%s %[^\n]", *action, *message);
    log_info("Chars read: %d\n", read);
    log_info("Action: %s, Message: %s\n", action, message);
    return read;
}

/*
Description:
    Closes a file.
Arguments:
    FILE *fd: The file pointer to close
Return value:
    Returns a 1 on failure, 0 on success
*/
int tcp_client_close_file(FILE *fd) {
    return fclose(fd);
}
