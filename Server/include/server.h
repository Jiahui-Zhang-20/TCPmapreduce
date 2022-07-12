/* test machine: csel-kh1250-10.cselabs.umn.edu
 * date: 4/21/2021
 * name: Daniel Black, Tam Nguyen, Jiahui Zhang
 * x500: blac0352, nguy3492, zhan7554
*/

#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <zconf.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include "../include/protocol.h"

typedef struct shared_data{
    int *result_histogram;   // histogram of word lengths (1-20)
    int *client_status;  // list of counts from clients
    pthread_mutex_t *res_mutex;  // mutex for result histogram
    pthread_mutex_t *cnt_mutex;  // mutex for client status list
} shared_data_t;

typedef struct thread_args{    // argument passed to server threads
    int client_id;   // server thread gives this to corresponding client process
    int client_fd;   // socket for corresponding client
    struct sockaddr_in client_add;  // client address
    shared_data_t *thread_data; // pointer to shared data
} thread_args_t;

/**
 * handles connection from client (handles requests and sends responses)
 * @param args the thread_args_t struct
 */
void *clientHandler(void *args);

/**
 * updates histogram of word length counts (synchronized)
 * @param thread_data the data shared among threads
 * @param stat_update status update array from the client
 * @return error if any
 */
int updateHistogram(shared_data_t *thread_data, int *stat_update);

/**
 * updates client status (how many update requests so far) (synchronized) 
 * @param thread_data the data shared among threads
 * @param client_id the ID of the client
 * @return error if any
 */
int updateClientStatus(shared_data_t *thread_data, int client_id);

/**
 * get client status (how many update requests so far) (synchronized) 
 * @param thread_data the data shared among threads
 * @param client_id the ID of the client
 * @return error if any
 */
int getClientStatus(shared_data_t *thread_data, int client_id);

/**
 * sum updates from all clients thus far (synchronized) 
 * @param thread_data the data shared among threads
 * @return error if any
 */
int sumUpdates(shared_data_t *thread_data);

#endif // SERVER_H
