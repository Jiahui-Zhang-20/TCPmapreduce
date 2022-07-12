/* test machine: csel-kh1250-10.cselabs.umn.edu
 * date: 4/23/2021
 * name: Daniel Black, Tam Nguyen, Jiahui Zhang
 * x500: blac0352, nguy3492, zhan7554
*/

#include "../include/server.h"

int main(int argc, char *argv[]) {

    // process input arguments
    if(argc<2){ // requires Server Port argument
        fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (char *sp = argv[1]; *sp != '\0'; sp++){ // check each character in argument is a digit
        if(isdigit(*sp)==0){ // iterate through each character to make sure it is within 0-9 (inclusive
        fprintf(stderr, "Usage: %s <Server Port>", argv[0]);
        return EXIT_FAILURE;
        }
    }

    unsigned int server_port = strtoul(argv[1], NULL, 10); // get server port in unsigned integer

    // socket create and verification 
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd<0){
        fprintf(stderr, "Server failed to create socket\n");
        return EXIT_FAILURE;
    }
    
    // Bind socket to a local address
    // assign IP, PORT 
    struct sockaddr_in servAddress;
    servAddress.sin_family = AF_INET;
    servAddress.sin_port = htons(server_port);
    servAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    // Binding newly created socket to given IP
    if((bind(sock_fd, (struct sockaddr *) &servAddress, sizeof(servAddress)))==-1){
        fprintf(stderr, "Server failed to bind socket\n");
        return EXIT_FAILURE;
    }
    
    // Now server is ready to listen
    if(listen(sock_fd, MAX_CONCURRENT_CLIENTS)==-1){
        fprintf(stderr, "Server failed to listen through socket\n");
        return EXIT_FAILURE;
    }
    printf("server is listening\n");

    shared_data_t *thread_data = (shared_data_t*)malloc(sizeof(shared_data_t));
    if(thread_data==NULL){
        fprintf(stderr, "Server failed to allocate memory for thread data\n");
        return EXIT_FAILURE;
    }
    thread_data->result_histogram = (int*)malloc(sizeof(int)*WORD_LENGTH_RANGE);
    if(thread_data==NULL){
        fprintf(stderr, "Server failed to allocate memory for result histogram\n");
        return EXIT_FAILURE;
    }
    thread_data->client_status = (int*)malloc(sizeof(int)*MAX_CONCURRENT_CLIENTS);
    if(thread_data==NULL){
        fprintf(stderr, "Server failed to allocate memory for client status\n");
        return EXIT_FAILURE;
    }
    thread_data->res_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if(thread_data==NULL){
        fprintf(stderr, "Server failed to allocate memory for result histogram mutex lock\n");
        return EXIT_FAILURE;
    }
    thread_data->cnt_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if(thread_data==NULL){
        fprintf(stderr, "Server failed to allocate memory for client status mutex lock\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i<WORD_LENGTH_RANGE; i++){ // initialize histogram to all zeros
        thread_data->result_histogram[i]=0;
    }

    for (int i = 0; i<MAX_NUM_CLIENTS; i++){  // initialize client status to all zeros
        thread_data->client_status[i] = 0;
    }

    // initialize mutex locks for result histogram and client status
    if((pthread_mutex_init(thread_data->res_mutex, NULL))!=0){
        fprintf(stderr, "Server failed to initialize result histogram mutex\n");
        return EXIT_FAILURE;
    }
    if((pthread_mutex_init(thread_data->cnt_mutex, NULL))!=0){
        fprintf(stderr, "Server failed to initialize client status mutex\n");
        return EXIT_FAILURE;
    }

    pthread_t server_threads[MAX_CONCURRENT_CLIENTS];   // thread IDs of clients
    int client_fd;
    int thread_num = 1;  // initialize thread number
    while (1) {
        // Accept the data packet from client
        struct sockaddr_in clientAddress;   // initialize socket address
        socklen_t add_size = sizeof(struct sockaddr_in);
        client_fd = accept(sock_fd, (struct sockaddr*)&clientAddress, &add_size);
        if(client_fd<0){
            fprintf(stderr, "Server failed to accept connection\n");
            return EXIT_FAILURE;
        }
        printf("open connection from %s:%d\n", inet_ntoa(clientAddress.sin_addr), clientAddress.sin_port);

        // update arguments to pass to server thread
        thread_args_t *curr_args = (thread_args_t*)malloc(sizeof(thread_args_t));
        if(curr_args==NULL){
            fprintf(stderr, "Server failed to allocate memory for current arguments\n");
            return EXIT_FAILURE;
        }
        curr_args->client_fd = client_fd;   // client file descriptor
        curr_args->client_add = clientAddress;  // client address
        curr_args->thread_data = thread_data;   // thread data

        // Spawn the threads
        // printf("create thread for client %d\n", thread_num);
        if((pthread_create(&server_threads[thread_num-1], NULL, clientHandler, curr_args))!=0){ // create thread
            fprintf(stderr, "Server failed to create thread\n");
            return EXIT_FAILURE;
        }
        // detach the client threads (so they release resources when exiting) 
        if((pthread_detach(server_threads[thread_num-1]))!=0){
            fprintf(stderr, "Server failed to detach thread\n");
            return EXIT_FAILURE;
        }
        thread_num++;    // increment thread number
    }

    // destroy all mutex locks
    if(pthread_mutex_destroy(thread_data->res_mutex)!=0){
        fprintf(stderr, "Server failed to destroy result histogram mutex\n");
        return EXIT_FAILURE;
    }
    if(pthread_mutex_destroy(thread_data->cnt_mutex)!=0){
        fprintf(stderr, "Server failed to destroy client status mutex\n");
        return EXIT_FAILURE;
    }


    // free the data struct passed to threads as argument
    free(thread_data->result_histogram);
    free(thread_data->client_status);
    free(thread_data->res_mutex);
    free(thread_data->cnt_mutex);
    free(thread_data);

    return EXIT_SUCCESS;
}

/**
 * handles connection from client (handles requests and sends responses)
 * @param args the thread_args_t struct
 */
void *clientHandler(void *client_args){
    thread_args_t *args = (thread_args_t *) client_args; // cast argument as data struct
    args->client_id = -1;   // initialize ID as -1
    int persistent_flag = 1;  // the persist flag should not exit until changed
    int received_request[REQUEST_MSG_SIZE]; // initialize request string
    int send_response[LONG_RESPONSE_MSG_SIZE];  // initialize response string
    ssize_t size_received;  // initialize size received from client
    ssize_t size_sent;  // initialize size sent to client
    int rsp_msg_size;   // set response message size (short/long) based on request code
    
    while(persistent_flag==PERSIST){
        // read the request from client
        size_received = recv(args->client_fd, received_request, sizeof(int)*REQUEST_MSG_SIZE, 0);    // receive request
        if(size_received!=sizeof(int)*REQUEST_MSG_SIZE){    // check correct number of bytes received
            // fprintf(stderr, "[%d] server only received %ld bytes out of %ld bytes expected\n", args->client_id, size_received, sizeof(int)*REQUEST_MSG_SIZE);
            // send_response[RSP_RSP_CODE_NUM] = RSP_NOK;
            // send(args->client_id, send_response, sizeof(int)*RESPONSE_MSG_SIZE, 0);  // send response
            break;
        }

        send_response[RSP_RQS_CODE_NUM] = received_request[RQS_RQS_CODE_NUM];   // get request code number

        if(args->client_id==-1){    // initialize client id (from now on, client ID must be the same)
            args->client_id=received_request[RQS_CLIENT_ID];
        }

        if(received_request[RQS_CLIENT_ID]<1){ // incorrect client ID
            fprintf(stderr, "[%d] invalid client ID: %d\n", args->client_id, received_request[RQS_CLIENT_ID]);
            send_response[RSP_RSP_CODE_NUM] = RSP_NOK;
            send(args->client_id, send_response, sizeof(int)*RESPONSE_MSG_SIZE, 0);  // send response
            fprintf(stderr, "[%d] ending server thread due to error\n", args->client_id);
            break;
        }

        // handle the request based on the request code received
        switch(send_response[RSP_RQS_CODE_NUM]) {  // process request from client 
            case UPDATE_WSTAT : // update the result histogram and client status
                if(updateHistogram(args->thread_data, &received_request[RQS_DATA])!=EXIT_SUCCESS){
                    fprintf(stderr, "[%d] server failed to update result histogram\n", args->client_id);
                    send_response[RSP_RSP_CODE_NUM] = RSP_NOK;
                    break;
                }
                if(updateClientStatus(args->thread_data, received_request[RQS_CLIENT_ID])!=EXIT_SUCCESS){
                    fprintf(stderr, "[%d] server failed to update client status\n", args->client_id);
                    send_response[RSP_RSP_CODE_NUM] = RSP_NOK;
                    break;
                }
                send_response[RSP_RSP_CODE_NUM] = RSP_OK;
                send_response[RSP_DATA] = received_request[RQS_CLIENT_ID];
                printf("[%d] UPDATE_WSTAT\n", received_request[RQS_CLIENT_ID]);
                rsp_msg_size = RESPONSE_MSG_SIZE;
                break;
            case GET_MY_UPDATES :   // get value of number of updates for current client
                send_response[RSP_RSP_CODE_NUM] = RSP_OK;
                send_response[RSP_DATA] = getClientStatus(args->thread_data, received_request[RQS_CLIENT_ID]);
                if(send_response[RSP_DATA]==-1){
                    fprintf(stderr, "[%d] failed to get client status\n", args->client_id);
                    send_response[RSP_RSP_CODE_NUM] = RSP_NOK;
                    break;
                }
                printf("[%d] GET_MY_UPDATES\n", received_request[RQS_CLIENT_ID]);
                rsp_msg_size = RESPONSE_MSG_SIZE;
                break;
            case GET_ALL_UPDATES : // get sum of number of updates of all clients
                send_response[RSP_RSP_CODE_NUM] = RSP_OK;
                send_response[RSP_DATA] = sumUpdates(args->thread_data);
                if(send_response[RSP_DATA]==-1){
                    fprintf(stderr, "[%d] server failed to get sum of updates", args->client_id);
                    send_response[RSP_RSP_CODE_NUM] = RSP_NOK;
                    break;
                }
                printf("[%d] GET_ALL_UPDATES\n", received_request[RQS_CLIENT_ID]);
                rsp_msg_size = RESPONSE_MSG_SIZE;
                break;
            case GET_WSTAT :    // get word length stat at moment of request
                send_response[RSP_RSP_CODE_NUM] = RSP_OK;
                for(int i = 0; i<WORD_LENGTH_RANGE; i++){
                    send_response[RSP_DATA+i] = args->thread_data->result_histogram[i];
                }
                printf("[%d] GET_WSTAT\n", received_request[RQS_CLIENT_ID]);
                rsp_msg_size = LONG_RESPONSE_MSG_SIZE;
                break;
            default :   // RESPOND IN ERROR
                fprintf(stderr, "[%d] wrong/undefined request code: %d\n", received_request[RQS_CLIENT_ID], send_response[RSP_RQS_CODE_NUM]);
                send_response[RSP_RSP_CODE_NUM] = RSP_NOK;
                rsp_msg_size = RESPONSE_MSG_SIZE;
                break;
        }

        // //print message to be send////////////////////////////////////////////////////////////
        // fprintf(stderr, "[%d] Server sending message: ", args->client_id);
        // for(int idx = 0; idx<rsp_msg_size; idx++){
        //     fprintf(stderr, "%d ", send_response[idx]);
        // }
        // fprintf(stderr, "\n");
        // //////////////////////////////////////////////////////////////////////////////////////

        size_sent = send(args->client_fd, send_response, sizeof(int)*rsp_msg_size, 0);  // send response
        if(size_sent!=sizeof(int)*rsp_msg_size){    // check correct number of bytes received
            fprintf(stderr, "[%d] server only sent %ld bytes out of %ld bytes expected\n", args->client_id, size_sent, sizeof(int)*rsp_msg_size);
            break;
        }

        persistent_flag = received_request[RQS_PERSISTENT_FLAG]; // update the persist flag

        if(send_response[RSP_RSP_CODE_NUM]==RSP_NOK){
            fprintf(stderr, "[%d] ending server thread due to error\n",args->client_id);
            break;
        }
   }

    close(args->client_fd); // close connection
    printf("[%d] close connection from %s:%d\n", args->client_id, inet_ntoa(args->client_add.sin_addr), args->client_add.sin_port);

    free((thread_args_t*)args);
    pthread_exit(NULL);
}

/**
 * updates histogram of word length counts (synchronized)
 * @param thread_data the data shared among threads
 * @param stat_update status update array from the client
 * @return error if any
 */
int updateHistogram(shared_data_t *thread_data, int *stat_update){
    if(pthread_mutex_lock(thread_data->res_mutex)!=0){
        pthread_mutex_unlock(thread_data->res_mutex);
        return EXIT_FAILURE;
    }
    for(int i = 0; i < WORD_LENGTH_RANGE; i++){
        thread_data->result_histogram[i] = thread_data->result_histogram[i] + stat_update[i];
    }
    if(pthread_mutex_unlock(thread_data->res_mutex)!=0){
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * update client status (how many update requests so far) (synchronized) 
 * @param thread_data the data shared among threads
 * @param client_id the ID of the client
 * @return error if any
 */
int updateClientStatus(shared_data_t *thread_data, int client_id){
    if(pthread_mutex_lock(thread_data->cnt_mutex)!=0){
        pthread_mutex_unlock(thread_data->cnt_mutex);
        return EXIT_FAILURE;
    }
    (thread_data->client_status)[client_id-1]++;
    if(pthread_mutex_unlock(thread_data->cnt_mutex)!=0){
        return EXIT_FAILURE;
    }  
    return EXIT_SUCCESS;
}

/**
 * get client status (how many update requests so far) (synchronized) 
 * @param thread_data the data shared among threads
 * @param client_id the ID of the client
 * @return error if any
 */
int getClientStatus(shared_data_t *thread_data, int client_id){
    int stat;
    if(pthread_mutex_lock(thread_data->cnt_mutex)!=0){
        pthread_mutex_unlock(thread_data->cnt_mutex);
        return -1;
    }
    stat = (thread_data->client_status)[client_id-1];
    if(pthread_mutex_unlock(thread_data->cnt_mutex)!=0){
        return -1;
    }
    return stat;
}

/**
 * sum updates from all clients thus far (synchronized) 
 * @param thread_data the data shared among threads
 * @return error if any
 */
int sumUpdates(shared_data_t *thread_data){
    if(pthread_mutex_lock(thread_data->cnt_mutex)!=0){
        pthread_mutex_unlock(thread_data->cnt_mutex);
        return -1;
    }
    int total = 0;
    for(int i = 0; i<MAX_NUM_CLIENTS; i++){
        total = total + (thread_data->client_status)[i];
    }
    if(pthread_mutex_unlock(thread_data->cnt_mutex)!=0){
        return -1;
    }
    return total;
}
