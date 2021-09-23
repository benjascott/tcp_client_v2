#include <stdio.h>

#include "tcp_client.h"
#include "log.h"

void printHelpMessage() {
    printf("\nUsage: tcp_client [--help] [-v] [-h HOST] [-p PORT] ACTION MESSAGE\n\n");
    printf("Arguments:\n  ACTION\tMust be uppercase, lowercase, title-case,\n\t\treverse, or shuffle.\n  MESSAGE\tMessage to send to the server\n");
    printf("Options:\n  --help\n  -v, --verbose\n  --host HOSTNAME, -h HOSTNAME\n  --port PORT, -p PORT\n");
}

int main(int argc, char *argv[]) {

    Config conf;
    int socket;

    log_set_level(LOG_TRACE);

    tcp_client_allocate_config(&conf);

    if(argc < 2) {
        printHelpMessage();
        log_warn("Incorrect number of arguments");
        exit(EXIT_FAILURE);
    }
    int result = tcp_client_parse_arguments(argc, argv, &conf);
    if(result != 0) {
        log_warn("Incorrect arguments provided");
        printHelpMessage();
        exit(EXIT_FAILURE);
    }


    log_info("host: %s, port: %s", conf.host, conf.port);
    socket = tcp_client_connect(conf);
    if(socket == -1) {
        log_warn("Unable to connect to a socket, exiting program");
        exit(EXIT_FAILURE);
    }
    else {
        log_trace("Connection was established to the socket.");
    }

    // open the file that will be read from
    FILE *f = tcp_client_open_file(conf.file);
    if(f != NULL) {
        log_trace("File was successfully opened.");
        fclose(f);
    }
    else {
        log_error("There was an error trying to open the file.");
    }


    char *action;
    char *message;

    //while there is data in the file to be sent, 
    while(tcp_client_get_line(f, &action, &message) != -1) {
        
        log_trace("Attempting to a new send message.");
        if(tcp_client_send_request(socket, action, message)) {
            log_warn("Message was not sent successfully to the server");
            exit(EXIT_FAILURE);
        }
    }
  
    
    if(tcp_client_close_file(f)) {
        log_error("There was a problem trying to close the file.");
    }
    else {
        log_trace("File was closed successfully.");
    }

    // // //init buf and buf_size
    // if(tcp_client_receive_response(socket, buf, buf_size)) {
    //     log_warn("Message expected from the server, not received");
    //     exit(EXIT_FAILURE);
    // }
    // printf("%s\n", buf);


    if(tcp_client_close(socket)) {
        log_warn("Unable to disconnect from the server");
        exit(EXIT_FAILURE);
    }

    log_info("Program executed successfully");
    exit(EXIT_SUCCESS);
}
