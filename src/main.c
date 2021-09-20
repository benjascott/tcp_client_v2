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
    char buf[1024];
    int buf_size = 1023;

    log_set_level(LOG_ERROR);

    tcp_client_allocate_config(&conf);

    if(argc < 3) {
        printHelpMessage();
        log_warn("Incorrect number of arguments");
        exit(EXIT_FAILURE);
    }
    int result = tcp_client_parse_arguments(argc, argv, &conf);
    if(result != 0) {
        log_warn("Incorrect number of arguments");
        exit(EXIT_FAILURE);
    }

    // printf("host: %s, port: %s\n", conf.host, conf.port);
    socket = tcp_client_connect(conf);
    if(socket == -1) {
        log_warn("Unable to connect to a socket, exiting program");
        exit(EXIT_FAILURE);
    }
  
    if(tcp_client_send_request(socket, conf)) {
        log_warn("Message was not sent successfully to the server");
        exit(EXIT_FAILURE);
    }

    // //init buf and buf_size
    if(tcp_client_receive_response(socket, buf, buf_size)) {
        log_warn("Message expected from the server, not received");
        exit(EXIT_FAILURE);
    }
    printf("%s\n", buf);

    if(tcp_client_close(socket)) {
        log_warn("Unable to disconnect from the server");
        exit(EXIT_FAILURE);
    }

    log_info("Program executed successfully");
    exit(EXIT_SUCCESS);
}
