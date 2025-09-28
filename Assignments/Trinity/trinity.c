// Created by Shamanthi Rajagopal
// ECE 252 Assignment 2

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>


int main( int argc, char** argv ) {
    if ( argc != 2 ) {
        printf( "Usage: %s hostname\n", argv[0] );
        return 1;
    }
    printf("Connecting to %s on port 80...\n", argv[1]);

    // lookup server address
    struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM };
    struct addrinfo *server;
    int status = getaddrinfo(argv[1], "80", &hints, &server); // returns 0 on success

    if (status != 0) { //error check to see if it failed to create address info struct
        fprintf(stderr,"%s\n", gai_strerror(status));
        return 1;

    }

    // create socket
    int sockfd;
    sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol); //idk where server->ai_protocol is declared

    if (sockfd < 0) { // error check
        
        perror("socket");
        freeaddrinfo(server);
        return 1;

    }

    // connect to server (0 means success)

    if (connect(sockfd, server->ai_addr, server->ai_addrlen) != 0) {      

        perror("connect");
        freeaddrinfo(server);
        close(sockfd);
        return 1;

    }

    // read file contents into buffer
    FILE *file = fopen("request.txt", "rb");

    if (!file) { //error check if file doesn't open
        perror("fopen");
        freeaddrinfo(server);
        close(sockfd);
        return 1;
    }

    if (fseek(file, 0, SEEK_END) != 0) { //error check incase fseek fails 
        
        perror("fseek");
        fclose(file);
        freeaddrinfo(server);
        close(sockfd);
        return 1;

    }

    long file_size_check = ftell(file); // variable to test if correct file size before using

    if (file_size_check < 0) { // error check to see if ftell is negative
        perror("ftell");
        fclose(file);
        freeaddrinfo(server);
        close(sockfd);
    }

    size_t file_size = (size_t)ftell(file); // use file size after confirming no error with file size
    rewind(file);

    char *request = malloc(file_size + 1);

    if (!request) { //request error check
        
        perror("malloc");
        fclose(file);
        freeaddrinfo(server);
        close(sockfd);
        return 1;

    }

    size_t n_read = fread(request, 1, file_size, file); // number of bytes read from the file
    
    if (n_read != file_size && ferror(file)) { // error check
        perror("fread");
        fclose(file);
        free(request);
        freeaddrinfo(server);
        close(sockfd);
        return 1;

    }
    
    request[n_read] = '\0';

    fclose(file);

    // send buffer to server
    size_t to_send = n_read;
    char *point = request;
   
    while (to_send > 0) {
        ssize_t sent = send(sockfd, point, to_send, 0);
        
        if (sent < 0) { // error check
            perror("send"); 
            free(request);
            freeaddrinfo(server);
            close(sockfd); 
            return 1;

        }

        point += sent;
        to_send -= sent;
    }


    // receive response and dump to screen

    char buffer[8192];
    
    for (;;) { // iterate forever
        
        ssize_t got = recv(sockfd, buffer, sizeof buffer, 0); // client reads from socket
        
        if (got < 0) { //error check

            perror("recv"); 
            break; 
        }
        
        if (got == 0) { // server closed
        
            break;
        }             
        
        fwrite(buffer, 1, got, stdout);  // write bytes for buffer
    }


    // Clean up memory and other things
    free(request);
    freeaddrinfo(server);
    close(sockfd);

    return 0; // end of program
}
