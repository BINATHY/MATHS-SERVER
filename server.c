#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // for daemon function
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <getopt.h> 
#include <assert.h>
#include <pthread.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/wait.h>

#define MAX_POINTS 4096
#define MAX_CLUSTERS 32
#define NUM_THREADSS 4
typedef struct point {
    float x; // The x-coordinate of the point
    float y; // The y-coordinate of the point
    int cluster; // The cluster that the point belongs to
} point;

int N; // number of entries in the data
int k; // number of centroids
point data[MAX_POINTS]; // Data coordinates
point cluster[MAX_CLUSTERS]; // The coordinates of each cluster center (also called centroid)

pthread_t threads[NUM_THREADSS];
pthread_mutex_t mutex;
pthread_barrier_t barrier;

#define MAXLINE 4098
#define MAX_SIZE 6000
#define NUM_THREADS 8// for command line argument parsing
typedef double matrix[MAX_SIZE][MAX_SIZE];

int N;              /* matrix size     */
int maxnum;         /* max number of element*/
char* Init;         /* matrix init type  */
int PRINT;          /* print switch     */
matrix A;           /* matrix A        */
matrix I = {{0.0}};  /* The A inverse matrix, which will be initialized to the identity matrix */

pthread_mutex_t lock;
pthread_barrier_t barrier;


void read_data() {
    N = 1797;
    k = 9;

    // Initialize points from the data file
    FILE* fp = fopen("kmeans-data.txt", "r");
    if (fp == NULL) {
        perror("Cannot open the file");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < N; i++) {
        fscanf(fp, "%f %f", &data[i].x, &data[i].y);
        data[i].cluster = -1; // Initialize the cluster number to -1
    }
    fclose(fp);
    printf("read data");

    // Initialize centroids randomly
    srand(0); // Setting 0 as the random number generation seed
    for (int i = 0; i < k; i++) {
        int r = rand() % N;
        cluster[i].x = data[r].x;
        cluster[i].y = data[r].y;
    }
    printf("Read the problem data!\n");
}


int get_closest_centroid(int i, int k)
{
    /* find the nearest centroid */
    int nearest_cluster = -1;
    double xdist, ydist, dist, min_dist;
    min_dist = dist = INT_MAX;
    for (int c = 0; c < k; c++)
    { // For each centroid
        // Calculate the square of the Euclidean distance between that centroid and the point
        xdist = data[i].x - cluster[c].x;
        ydist = data[i].y - cluster[c].y;
        dist = xdist * xdist + ydist * ydist; // The square of Euclidean distance
        //printf("%.2lf \n", dist);
        if (dist <= min_dist)
        {
            min_dist = dist;
            nearest_cluster = c;
        }
    }
    //printf("-----------\n");
    return nearest_cluster;
}

bool assign_clusters_to_points()
{
    bool something_changed = false;
    int old_cluster = -1, new_cluster = -1;
    for (int i = 0; i < N; i++)
    { // For each data point
        old_cluster = data[i].cluster;
        new_cluster = get_closest_centroid(i, k);
        data[i].cluster = new_cluster; // Assign a cluster to the point i
        if (old_cluster != new_cluster)
        {
            something_changed = true;
        }
    }
    return something_changed;
}

void* update_cluster_centers(void* thread_id) {
    int tid = *(int*)thread_id;
    int start = tid * (k / NUM_THREADSS);
    int end = (tid == NUM_THREADSS - 1) ? k : start + (k / NUM_THREADSS);

    int count[MAX_CLUSTERS] = {0};
    point temp[MAX_CLUSTERS] = {{0.0}};

    for (int i = 0; i < N; i++) {
        int c = data[i].cluster;
        count[c]++;
        temp[c].x += data[i].x;
        temp[c].y += data[i].y;
    }

    for (int i = start; i < end; i++) {
        if (count[i] != 0) {
            cluster[i].x = temp[i].x / count[i];
            cluster[i].y = temp[i].y / count[i];
        }
    }

    return NULL;
}

int kmeans(int k){
    pthread_mutex_init(&mutex, NULL);
    int thread_args[NUM_THREADSS];
    int iter = 0;
    bool somechange;
   
    do {
        iter++;
        somechange = assign_clusters_to_points();
        for (int i = 0; i < NUM_THREADSS; i++) {
            thread_args[i] = i;
            pthread_create(&threads[i], NULL, update_cluster_centers, &thread_args[i]);
        }

        for (int i = 0; i < NUM_THREADSS; i++) {
            pthread_join(threads[i], NULL);
        }
    } while (somechange);

    pthread_mutex_destroy(&mutex);

    printf("Number of iterations taken = %d\n", iter);
    printf("Computed cluster numbers successfully!\n");

    return 0;
}

void write_results() {
    FILE* fp = fopen("kmeans-results-par.txt", "w");
    if (fp == NULL) {
        perror("Cannot open the file");
        exit(EXIT_FAILURE);
    }
    else {
        for (int i = 0; i < N; i++) {
            fprintf(fp, "%.2f %.2f %d\n", data[i].x, data[i].y, data[i].cluster);
        }
    }
    printf("Wrote the results to a file!\n");
}

void kmeans_parallel_main(){
pthread_mutex_init(&mutex, NULL);
    pthread_barrier_init(&barrier, NULL, NUM_THREADSS);
    read_data();
    kmeans(k);
    write_results();
    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);
    


}






void *find_inverse(void *thread_id) {
    int p, col, row;
    double pivalue;

    int tid = *(int*)thread_id;

    int chunk_size = N / NUM_THREADS;
    int start = tid * chunk_size;
    int end = (tid == NUM_THREADS - 1) ? N : start + chunk_size;

    for (p = 0; p < N; p++) {
        pthread_mutex_lock(&lock);
        pivalue = A[p][p];
        for (col = 0; col < N; col++) {
            A[p][col] = A[p][col] / pivalue; /* Division step on A */
            I[p][col] = I[p][col] / pivalue; /* Division step on I */
        }
        assert(A[p][p] == 1.0);
        pthread_mutex_unlock(&lock);

      
        pthread_barrier_wait(&barrier);

        double multiplier;

        for (row = start; row < end; row++) {
            multiplier = A[row][p];
            if (row != p) { // Perform elimination on all except the current pivot row
                for (col = 0; col < N; col++) {
                    A[row][col] = A[row][col] - A[p][col] * multiplier; /* Elimination step on A */
                    I[row][col] = I[row][col] - I[p][col] * multiplier; /* Elimination step on I */
                }
                assert(A[row][p] == 0.0);
            }
        }

       
        pthread_barrier_wait(&barrier);
    }

    pthread_exit(NULL);
}

void Init_Matrix()
{
   int row, col;

    // Set the diagonal elements of the inverse matrix to 1.0
    // So that you get an identity matrix to begin with
    for (row = 0; row < N; row++) {
        for (col = 0; col < N; col++) {
            if (row == col)
                I[row][col] = 1.0;
        }
    }

    //printf("\nsize      = %dx%d ", N, N);
    //printf("\nmaxnum    = %d \n", maxnum);
    //printf("Init	  = %s \n", Init);
    

    if (strcmp(Init, "rand") == 0) {
        for (row = 0; row < N; row++) {
            for (col = 0; col < N; col++) {
                if (row == col) /* diagonal dominance */
                    A[row][col] = (double)(rand() % maxnum) + 5.0;
                else
                    A[row][col] = (double)(rand() % maxnum) + 1.0;
            }
        }
    }
    if (strcmp(Init, "fast") == 0) {
        for (row = 0; row < N; row++) {
            for (col = 0; col < N; col++) {
                if (row == col) /* diagonal dominance */
                    A[row][col] = 5.0;
                else
                    A[row][col] = 2.0;
            }
        }
    }

    printf("done \n\n");
    if (PRINT == 1)
    {
        //Print_Matrix(A, "Begin: Input");
        //Print_Matrix(I, "Begin: Inverse");
    }
}

void Print_Matrix(matrix M, char name[])
{
    int row, col;

    printf("%s Matrix:\n", name);
    for (row = 0; row < N; row++) {
        for (col = 0; col < N; col++)
            printf(" %5.2f", M[row][col]);
        printf("\n");
    }
    printf("\n\n");
}

void Print_Matrix_To_File(matrix M, char name[], char* filename) {
    int row, col;
    FILE *fp;
    fp = fopen(filename, "w");
    fprintf(fp, "%s Matrix:\n", name);
    fprintf(fp,"\nsize      = %dx%d ", N, N);
    fprintf(fp,"\nmaxnum    = %d \n", maxnum);
    fprintf(fp,"Init	  = %s \n", Init);
    fprintf(fp,"Initializing matrix...");
    for (row = 0; row < N; row++) {
        for (col = 0; col < N; col++)
            fprintf(fp, " %5.2f", M[row][col]);
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n\n");
    fclose(fp);
}

void
Init_Default()
{
    N = 5;
    Init = "fast";
    maxnum = 15.0;
    PRINT = 1;
}



void
Read_Options(int argc, char** argv)
{
    char* prog;

    prog = *argv;
    while (++argv, --argc > 0)
        if (**argv == '-')
            switch (*++ * argv) {
            case 'n':
                --argc;
                N = atoi(*++argv);
                break;
            case 'h':
                printf("\nHELP: try matinv -u \n\n");
                exit(0);
                break;
            case 'u':
                printf("\nUsage: matinv [-n problemsize]\n");
                printf("           [-D] show default values \n");
                printf("           [-h] help \n");
                printf("           [-I init_type] fast/rand \n");
                printf("           [-m maxnum] max random no \n");
                printf("           [-P print_switch] 0/1 \n");
                exit(0);
                break;
            case 'D':
                printf("\nDefault:  n         = %d ", N);
                printf("\n          Init      = rand");
                printf("\n          maxnum    = 5 ");
                printf("\n          P         = 0 \n\n");
                exit(0);
                break;
            case 'I':
                --argc;
                Init = *++argv;
                break;
            case 'm':
                --argc;
                maxnum = atoi(*++argv);
                break;
            case 'P':
                --argc;
                PRINT = atoi(*++argv);
                break;
            default:
                printf("%s: ignored option: -%s\n", prog, *argv);
                printf("HELP: try %s -u \n\n", prog);
                break;
            }
}


void matrixinv_parallel(char *array[10], int repeat, int client_count, int client_socket){

pthread_mutex_init(&lock, NULL);

    //printf("Matrix Inverse\n");
    printf("matrix_client_%d_soln_%d.txt sent",client_count,repeat);
    //int i, timestart, timeend, iter;

    Init_Default();      /* Init default values    */
    Read_Options(7, array);   /* Read arguments    */
    Init_Matrix();      /* Init the matrix    */

    pthread_t threads[NUM_THREADS]; 
    int thread_args[NUM_THREADS];


    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        pthread_create(&threads[i], NULL, find_inverse, &thread_args[i]);
    }

    
    for ( int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    if (PRINT == 1) {
        //char output_file[] = "matrix_client%n_soln%n.txt";
        char output_file[100];
        const char *relative_path = "mathsserver/computer_results/";
        
        sprintf(output_file, "%smatrix_client_%d_soln_%d.txt",relative_path, client_count, repeat);
        //sprintf(output_file, "matrix_client_%d_soln_%d.txt", repeat, client_count);
        Print_Matrix_To_File(I, "Inversed", output_file);
        
        // Sending the file back to the client
        FILE *fp;
        char buffer[MAX_SIZE];
        fp = fopen(output_file, "r");
        if (fp == NULL) {
            perror("Error opening file");
            exit(1);
        }

        char c;
        int i = 0;
        while ((c = getc(fp)) != EOF) {
            buffer[i++] = c;
        }
        buffer[i] = '\0';

        fclose(fp);

        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            perror("Error sending data");
            exit(1);
        }
        
        
    }
}

void handle_client(int client_socket, int client_count) {
int repeat = 1;
while(1){
char message[100];
    
    memset(message, 0, sizeof(message));
    if (recv(client_socket, message, sizeof(message), 0) == -1) {
    perror("error receiving data");
    exit(1);
    };
    
   printf("received from client\n");
   
   if (strstr(message, "matinvpar") != NULL) {
      //printf("hello");   
      char *token = strtok(message, " ");
      char *array[10];
      int j = 0;
      while (token != NULL && j < 10) {
          array[j] = token;
          //printf("array[%d] = %s\n", j, array[j]);
          token = strtok(NULL, " ");
          j++;
          }
          matrixinv_parallel(array, repeat, client_count, client_socket);
          memset(message, 0, sizeof(message));
          repeat++;
        
   } else if (strstr(message, "kmeanspar") != NULL) {
         char buffer[MAXLINE];
           int file_fd = open("received_file.txt", O_CREAT | O_WRONLY, 0666);
           if (file_fd == -1) {
               perror("file open failed");
               exit(1);
           }

           struct stat stat_buf;
           fstat(client_socket, &stat_buf);

           //off_t offset = 0;
           ssize_t bytes_received = 0;
           while ((bytes_received = recv(client_socket, buffer, MAXLINE, 0)) > 0) {
               if (write(file_fd, buffer, bytes_received) != bytes_received) {
                   perror("write to file failed");
                   //exit(1);
                }
          }
          kmeans_parallel_main();

          if (bytes_received == -1) {
              perror("receive file failed");
              exit(1);
          }
          
          
         
   
   } else {
   
          printf("invalid command");   
   }
   }     
}

int main(int argc, char const* argv[])
{
    int port_number = 9001; // Default port
    //int daemonize = 0; // Don't run as a daemon by default
    //char* strategy = "fork"; // Default request handling strategy

    // Parse command line arguments
    int opt;
     while ((opt = getopt(argc, (char* const*)argv, "hp:ds:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                printf("Usage: %s [-h] [-p port] [-d] [-s strategy]\n", argv[0]);
                exit(0);
            case 'p':
                port_number = atoi(optarg);
                break;
            case 'd':
                //daemon == 1;
                break;
            case 's':
                //strategy = optarg;
               
                break;
            default:
                fprintf(stderr, "Usage: %s [-h] [-p port] [-d] [-s strategy]\n", argv[0]);
                exit(3);
        }
    }
    if (optind < argc) {
	    port_number = atoi(argv[optind]);
    }

    /*if (daemonize) {
        // Daemonize the process
        if (daemon(0, 0) == -1) {
            perror("Failed to daemonize");
            exit(3);
        }

        // Print the process ID if running as a daemon
        printf("Server is running as a daemon with PID: %d\n", getpid());
    }
*/

    // Create a socket
    int ser_sock= socket(AF_INET, SOCK_STREAM, 0);

    // Define server address and bind
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port_number);
    servAddr.sin_addr.s_addr = INADDR_ANY;
    bind(ser_sock, (struct sockaddr*)&servAddr, sizeof(servAddr));
    printf("listening.............");
    // Listen for connections
    listen(ser_sock, 10);
    
    int client_socket;
    
    
    socklen_t addr_len = sizeof(servAddr);
//if ((client_socket = accept(ser_sock, (struct sockaddr *)&servAddr, &addr_len)) < 0) {
    // Handle the error
//}

    
    
    
    int client_count = 1;
     while (1) {
        // Accept a new connection
        if ((client_socket = accept(ser_sock, (struct sockaddr *)&servAddr, &addr_len)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        // Create a child process to handle the client
        /*if (fork() == 0) {
            
                    
            // Child process
            close(ser_sock);
            handle_client(client_socket,client_count);
            client_count += 1 ;
             exit(0); 
            //close(ser_sock);
            
        } else {
        close(client_socket); 
        int status;
        pid_t term_child = wait(&status);
        if (term_child == -1 ){
            perror("wait failed");
        } else {
          printf("child has termimated");
        }  
        
        
        
        
        }*/
        
         pid_t pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        close(ser_sock); // Close the server socket in the child process
        handle_client(client_socket, client_count);
        
        close(client_socket);
         // Close the client socket in the child process
        exit(0); // Exit the child process
    } else {
        close(client_socket); // Close the client socket in the parent process
        client_count += 1;
    }
    }
    
     
    
    return 0;
}

