/**
 * \author {AUTHOR}
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "config.h"
#include "lib/tcpsock.h"
#include "pthread.h"


#define PORT 1234 // Define the port number here
#define MAX_CONN 10 // Define the maximum connections here

/**
 * Implements a sequential test server (only one connection at the same time)
 */
 // Mutex for thread-safe manipulation of connection counter
 int conn_counter = 0;

 pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

 // Function to handle client connections in separate threads
 void *handle_client(void *arg){
     tcpsock_t *client = (tcpsock_t *) arg;
     sensor_data_t data;
     int bytes, result;

     // ... read and process data from client
     // This part of the code can remain the same as before
     // ...

     tcp_close(&client); // Close client connection when done
     pthread_mutex_lock(&counter_mutex);
     conn_counter--; // Decrease connection counter in a thread-safe manner
     pthread_mutex_unlock(&counter_mutex);
     return NULL;
 }



int main(int argc, char *argv[]) {
    tcpsock_t *server, *client;
    sensor_data_t data;
    int bytes, result;

    
    if(argc < 3) {
    	printf("Please provide the right arguments: first the port, then the max nb of clients");
    	return -1;
    }
    


    printf("Test server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);

    pthread_t tid; // Thread ID placeholder

    do {
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) exit(EXIT_FAILURE);
        printf("Incoming client connection\n");

        pthread_mutex_lock(&counter_mutex);
        if (conn_counter <MAX_CONN){
            conn_counter++;
            pthread_create(&tid, NULL, handle_client, (void *)client); // Create a thread to handle client

            //HANDLE CLIENT
            do {
                // read sensor ID
                bytes = sizeof(data.id);
                result = tcp_receive(client, (void *) &data.id, &bytes);
                // read temperature
                bytes = sizeof(data.value);
                result = tcp_receive(client, (void *) &data.value, &bytes);
                // read timestamp
                bytes = sizeof(data.ts);
                result = tcp_receive(client, (void *) &data.ts, &bytes);
                if ((result == TCP_NO_ERROR) && bytes) {
                    printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value,
                            (long int) data.ts);
                }

            } while (result == TCP_NO_ERROR);
            if (result == TCP_CONNECTION_CLOSED)
                printf("Peer has closed connection\n");
            else
                printf("Error occured on connection to peer\n");
            tcp_close(&client);


            pthread_detach(tid); // Detach thread to avoid memory leaks
        } else {
            printf("Server at maximum capacity. Rejecting new connection.\n");
            tcp_close(&client); // Close the connection if maximum capacity reached
        }
        pthread_mutex_unlock(&counter_mutex);

    } while (conn_counter < MAX_CONN);
    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    printf("Test server is shutting down\n");
    return 0;
}




