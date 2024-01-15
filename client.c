#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>



#define MAXLINE 1024

//#define PORT 8080
//#define SERVER_IP "127.0.0.1" 



int main(int argc, char const* argv[]) {
    int client_socket;
    struct sockaddr_in server_address;
    //char buffer[1024] = {0};
    const char *ip_address = NULL;
    int port = -1;
    char message[1024];

    
    if (argc != 5) {
        fprintf(stderr, "Usage: %s -ip <IP_ADDRESS> -p <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-ip") == 0) {
            ip_address = argv[i + 1];
        } else if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[i + 1]);
        } else {
            fprintf(stderr, "Invalid option: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
    
    //const char *message = "Hello from the client!";

    
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    

    
    if (inet_pton(AF_INET, ip_address, &server_address.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

     while (1) {
        printf("Enter a Command (or type 'exit' to quit): ");

        if (fgets(message, sizeof(message), stdin) == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        if (strcmp(message, "exit\n") == 0) {
            printf("Exiting...\n");
            break;
        }
        if (strstr(message, "matinvpar") != NULL) {
        
            if (send(client_socket, message, strlen(message), 0) == -1) {
            perror("Error sending data");
            exit(1);
                }       
            printf("Data sent to server\n");
            
            
       } else if (strncmp(message, "kmeanspar",9)==0) {
           if (send(client_socket, message, strlen(message), 0) == -1) {
            perror("Error sending data");
            exit(1);
                }       
            printf(" Command sent to server\n");
            
           char *file_name = "kmeans-data.txt";
           int fd = open(file_name, O_RDONLY);
           if (fd == -1) {
               perror("open");
               exit(1);
          }
          
          struct stat stat_buf;
          fstat(fd, &stat_buf);

          off_t offset = 0;
          int remaining_data = stat_buf.st_size;
          ssize_t sent_bytes = 0;

          // Send file using sendfile
          while (((sent_bytes = sendfile(client_socket, fd, &offset, MAXLINE)) > 0) && (remaining_data > 0)) {
              remaining_data -= sent_bytes;
             // printf("test case 1\n");
          }
                   
      } else {
      
         printf("invalid option");
      } 

       
        
    }

    close(client_socket);

    return 0;
}

