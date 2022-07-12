/* test machine: csel-kh1250-10.cselabs.umn.edu
 * date: 4/23/2021
 * name: Daniel Black, Tam Nguyen, Jiahui Zhang
 * x500: blac0352, nguy3492, zhan7554
*/

#include "../include/client.h"

int main(int argc, char *argv[]) {

    // create a named semaphore for logging
    sem_t *log_sem = sem_open(LOG_SEM_NAME, LOG_SEM_OFLAG, LOG_SEM_PERMISSIONS, LOG_SEM_INIT_VAL);
    if(log_sem==SEM_FAILED){
        // only print statement without synchronization
        fprintf(stderr, "Client failed to create named semaphore\n");
        fprintf(stderr, "It is likely that the semaphore is not unlinked\n");
        fprintf(stderr, "Please remove the semaphore %s from /dev/shm/\n", LOG_SEM_NAME);
        return EXIT_FAILURE;
    }

    sem_unlink(LOG_SEM_NAME);   // unlink the semaphore

    // create log file
    createLogFile(); 

    char print_string[MAX_PRINT_LENGTH];   // buffer for any string output
    setbuf(logfp, NULL);

    // all outputs will be printed using the printLog() function below
    
    // process input arguments
    int extra_flag =0;  // initialize extra credit flag to be 0 (no EC)

    if(argc<5){    // check there are at least 4 input arguments
        sprintf(print_string, "Usage: %s <Folder Name> <# of Clients> <Server IP> <Server Port> [-e]\n", argv[0]);
        printLog(print_string, log_sem);   
        return EXIT_FAILURE;
    }

    // check that input directory is valid
    char *input_dir = argv[1];
    struct stat buf;
    if(!(stat(input_dir, &buf) == 0) || !(S_ISDIR(buf.st_mode))){
        sprintf(print_string, "The provided directory is invalid\n");
        printLog(print_string, log_sem);  
        return EXIT_FAILURE; 
    }

    int num_files_total = getFileNames(input_dir, input_file_names, 0);    // add file names to array

    if(num_files_total<0){
        sprintf(print_string, "Failed to read file names\n");
        printLog(print_string, log_sem); 
        return EXIT_FAILURE;  
    }

    // get the number of clients
    int num_clients = strtol(argv[2], NULL, 10);
    if(num_clients <=0 || num_clients>20){
        sprintf(print_string, "The number of clients must be between 1 and 20 (inclusive)\n");
        printLog(print_string, log_sem);   

        sprintf(print_string, "Usage: %s <Folder Name> <# of Clients> <Server IP> <Server Port> [e]\n", argv[0]);
        printLog(print_string, log_sem);   
        return EXIT_FAILURE;
    }

    char *server_ip = argv[3];    // get server IP address
    unsigned int server_port = strtoul(argv[4], NULL, 10);    // get server port

    if(argc==6){    // check for extra credit flag
        if(strcmp(argv[5],"-e")==0){
            extra_flag = 1;
        }
    }

	// Specify an address to connect to (we use the local host or 'loop-back' address).
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(server_port);
	address.sin_addr.s_addr = inet_addr(server_ip);

    // spawn client processes
    pid_t client_pid;   
    int sockfd;
    int send_request[REQUEST_MSG_SIZE];
    int receive_response[LONG_RESPONSE_MSG_SIZE];
    int size_sent;
    int size_received;
    int num_files_client = 0;

    if(extra_flag==1){ // the extra credit implementation will be followed
        sprintf(print_string, "Persistent connection option enabled\n");
        printLog(print_string, log_sem);
    /********************************* extra credit *************************************************/
        // spawn the client child processes
        for(int client_id = 1; client_id <=num_clients; client_id++){
            if((client_pid = fork())<0){
                sprintf(print_string, "Failed to fork child client process\n");
                printLog(print_string, log_sem);   
            }
            
            if(client_pid!=0){ // parent process
                continue; // skips remaining loop code
            }

            // connect to server
            sockfd = socket(AF_INET , SOCK_STREAM , 0); // create socket
            if(sockfd==-1){
                sprintf(print_string, "[%d] Failed to craete socket\n", client_id);
                printLog(print_string, log_sem);   
            }

            if(connect(sockfd, (struct sockaddr *) &address, sizeof(address))!=0){  // connect socket to server
                sprintf(print_string, "[%d] Failed to initiate connection\n", client_id);
                printLog(print_string, log_sem);   
            } 

            sprintf(print_string, "[%d] open connection\n", client_id);
            printLog(print_string, log_sem);   

            // client child process
            send_request[RQS_CLIENT_ID] = client_id;    // initialize client ID
            send_request[RQS_PERSISTENT_FLAG] = PERSIST;  // initialize persist flag to 1

            // send UPDATE_WSTAT for each file this client parses 
            for(int i = 0; i < num_files_total; i++){ // iterate through each file in the file names array
                if(client_id > num_files_total){ // no files assigned to this client
                    break;
                }

                // assigning files in a round-robin fashion
                if((i%(num_clients)+1)!=client_id){ // do not assign to file to this client
                    continue;
                }

                send_request[RQS_RQS_CODE_NUM] = UPDATE_WSTAT;

                for(int j=RQS_DATA; j<RQS_PERSISTENT_FLAG; j++){    // zero out data in request message 
                    send_request[j]=0;
                }

                // fprintf(stderr, "[%d] client ID: %d\n", client_id, send_request[RQS_CLIENT_ID]);

                count(input_file_names[i], &send_request[RQS_DATA]); // count lengths and add to count array
                num_files_client++;

                //print message to be send////////////////////////////////////////////////////////////
                // fprintf(stderr, "[%d] Client sending UPDATE_WSTAT: ", client_id);
                // for(int idx = 0; idx<REQUEST_MSG_SIZE; idx++){
                //     fprintf(stderr, "%d ", send_request[idx]);
                // }
                // fprintf(stderr, "\n");
                //////////////////////////////////////////////////////////////////////////////////////

                size_sent = send(sockfd, send_request, sizeof(int)*REQUEST_MSG_SIZE, 0);    // send request to server
                if(size_sent!=sizeof(int)*REQUEST_MSG_SIZE){
                    sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                    printLog(print_string, log_sem);   
                    sprintf(print_string, "[%d] Failed to send request\n", client_id);
                    printLog(print_string, log_sem);   
                    return EXIT_FAILURE;
                }
                size_received = recv(sockfd, receive_response, sizeof(int)*RESPONSE_MSG_SIZE, 0);  // receive short response
                if(size_received!=sizeof(int)*RESPONSE_MSG_SIZE){
                    sprintf(print_string, "[%d] Failed to receive response\n", client_id);
                    printLog(print_string, log_sem);   
                    sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                    printLog(print_string, log_sem);   
                    return EXIT_FAILURE;
                }
                if(receive_response[RSP_RSP_CODE_NUM]!=RSP_OK){ // check response code is OK
                    sprintf(print_string, "[%d] received error from server\n", client_id);
                    printLog(print_string, log_sem);   
                    sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                    printLog(print_string, log_sem);   
                    return EXIT_FAILURE;
                }

                if(receive_response[RSP_RQS_CODE_NUM]!=UPDATE_WSTAT){   // check request code is correct
                    sprintf(print_string, "received request code num: %d\n", receive_response[RSP_RQS_CODE_NUM]);
                    printLog(print_string, log_sem);   
                    sprintf(print_string, "actual request code num: %d\n", UPDATE_WSTAT);
                    printLog(print_string, log_sem);
                    sprintf(print_string, "[%d] received incorrect request code number\n", client_id);
                    printLog(print_string, log_sem);
                    sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                    printLog(print_string, log_sem);
                    return EXIT_FAILURE;
                }

                if(receive_response[RSP_DATA]!=client_id){  // check client ID is correct
                    sprintf(print_string, "received ID: %d\n", receive_response[RSP_DATA]);
                    printLog(print_string, log_sem);
                    sprintf(print_string, "actual ID: %d\n", client_id);
                    printLog(print_string, log_sem);
                    sprintf(print_string, "[%d] received incorrect client ID\n", client_id);
                    printLog(print_string, log_sem);
                    sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                    printLog(print_string, log_sem);
                    return EXIT_FAILURE;
                }

            }

            if(num_files_client>0){ // only print this message if client parsed at least one file
                sprintf(print_string, "[%d] UPDATE_WSTAT: %d\n", client_id, num_files_client);
                printLog(print_string, log_sem);
            }

            for(int k = RQS_DATA; k < RQS_PERSISTENT_FLAG; k++){ // zero out data for remaining three requests
                send_request[k]=0;
            }

            // SEND GET_MY_UPDATES

            //print message to be send////////////////////////////////////////////////////////////
            // fprintf(stderr, "[%d] Client sending GET_MY_UPDATES: ", client_id);
            // for(int idx = 0; idx<REQUEST_MSG_SIZE; idx++){
            //     fprintf(stderr, "%d ", send_request[idx]);
            // }
            // fprintf(stderr, "\n");
            //////////////////////////////////////////////////////////////////////////////////////

            send_request[RQS_RQS_CODE_NUM] = GET_MY_UPDATES;
            size_sent = send(sockfd, send_request, sizeof(int)*REQUEST_MSG_SIZE, 0);    // send request
            if(size_sent!=sizeof(int)*REQUEST_MSG_SIZE){
                sprintf(print_string,"[%d] failed to send request\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }
            size_received = recv(sockfd, receive_response, sizeof(int)*RESPONSE_MSG_SIZE, 0);   // receive response
            if(size_received!=sizeof(int)*RESPONSE_MSG_SIZE){
                sprintf(print_string, "[%d] failed to receive response\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }
            if(receive_response[RSP_RSP_CODE_NUM]!=RSP_OK){ // check response code is OK
                sprintf(print_string, "[%d] received error from server\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }
            if(receive_response[RSP_RQS_CODE_NUM]!=GET_MY_UPDATES){ // check request code is correct
                sprintf(print_string, "[%d] Received incorrect request code number\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            sprintf(print_string, "[%d] GET_MY_UPDATES: %d %d\n", client_id, receive_response[RSP_RSP_CODE_NUM], receive_response[RSP_DATA]);
            printLog(print_string, log_sem);

            // SEND GET_ALL_UPDATES
            send_request[RQS_RQS_CODE_NUM] = GET_ALL_UPDATES;

            //print message to be send////////////////////////////////////////////////////////////
            // fprintf(stderr, "[%d] Client sending GET_ALL_UPDATES: ", client_id);
            // for(int idx = 0; idx<REQUEST_MSG_SIZE; idx++){
            //     fprintf(stderr, "%d ", send_request[idx]);
            // }
            // fprintf(stderr, "\n");
            /////////////////////////////////////////////////////////////////////////////////////

            size_sent = send(sockfd, send_request, sizeof(int)*REQUEST_MSG_SIZE, 0);    // send request
            if(size_sent!=sizeof(int)*REQUEST_MSG_SIZE){
                sprintf(print_string, "[%d] failed to send request\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }
            size_received = recv(sockfd, receive_response, sizeof(int)*RESPONSE_MSG_SIZE, 0);   // receive request
            if(size_received!=sizeof(int)*RESPONSE_MSG_SIZE){
                sprintf(print_string, "[%d] failed to receive response\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            if(receive_response[RSP_RSP_CODE_NUM]!=RSP_OK){ // check response code is OK
                sprintf(print_string, "[%d] received error from server\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            if(receive_response[RSP_RQS_CODE_NUM]!=GET_ALL_UPDATES){    // check total number of updates correct
            sprintf(print_string, "[%d] received incorrect request code number\n", client_id);
            printLog(print_string, log_sem);
            sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
            printLog(print_string, log_sem);
            return EXIT_FAILURE;
            }

            sprintf(print_string, "[%d] GET_ALL_UPDATES: %d %d\n", client_id, receive_response[RSP_RSP_CODE_NUM], receive_response[RSP_DATA]);
            printLog(print_string, log_sem);

            // SEND GET_WSTAT
            send_request[RQS_RQS_CODE_NUM] = GET_WSTAT;
            send_request[RQS_PERSISTENT_FLAG] = NO_PERSIST;  // tell server to terminate connection
            
            //print message to be send////////////////////////////////////////////////////////////
            // fprintf(stderr, "[%d] Client sending GET_WSTAT: ", client_id);
            // for(int idx = 0; idx<REQUEST_MSG_SIZE; idx++){
            //     fprintf(stderr, "%d ", send_request[idx]);
            // }
            // fprintf(stderr, "\n");
            /////////////////////////////////////////////////////////////////////////////////////

            size_sent = send(sockfd, send_request, sizeof(int)*REQUEST_MSG_SIZE, 0);    // send request
            if(size_sent!=sizeof(int)*REQUEST_MSG_SIZE){
                sprintf(print_string, "[%d] failed to send request\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }
            size_received = recv(sockfd, receive_response, sizeof(int)*LONG_RESPONSE_MSG_SIZE, 0);  // receive response
            if(size_received!=sizeof(int)*LONG_RESPONSE_MSG_SIZE){

                sprintf(print_string, "[%d] failed to receive response\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            if(receive_response[RSP_RSP_CODE_NUM]!=RSP_OK){ // check response code is OK
                sprintf(print_string, "[%d] received error from server\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            if(receive_response[RSP_RQS_CODE_NUM]!=GET_WSTAT){  // check request code is correct
                sprintf(print_string, "[%d] received incorrect request code number\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            sprintf(print_string, "[%d] GET_WSTAT: %d ", client_id, receive_response[RSP_RSP_CODE_NUM]);
            char buf[MAX_PRINT_LENGTH];
            for(int l=RSP_DATA; l<RSP_DATA+WORD_LENGTH_RANGE; l++){
                sprintf(buf, "%d ", receive_response[l]);
                strcat(print_string, buf);
            }
            sprintf(buf, "\n");
            strcat(print_string, buf);
            printLog(print_string, log_sem);
            
            close(sockfd);  // close the socket between child client process and server thread
            sprintf(print_string, "[%d] close connection (successful execution)\n", client_id);
            printLog(print_string, log_sem);
            sem_close(log_sem); // child client closes semaphore    // close the log semaphore
            return EXIT_SUCCESS;
            // client child process exits
        }

    /**************************************************************************************************/
    } else {    // the non-extra credit implemntation will be followed
        sprintf(print_string, "Extra credit disabled\n");            
        printLog(print_string, log_sem);
    /********************************* no extra credit *************************************************/
    char path_name[MAX_FILE_NAME_LENGTH];
    for(int client_id = 1; client_id<=num_clients; client_id++){ // each client reads at one or no file
        if(client_id>num_files_total){ // this client gets no files
            break;
        } 
        sprintf(path_name, "%s/%d.txt", input_dir, client_id);
        struct stat buf;   
        if(stat(path_name, &buf)!=0){
            break;
        }

        send_request[RQS_CLIENT_ID] = client_id;    // initialize client ID
        send_request[RQS_PERSISTENT_FLAG] = NO_PERSIST;  // initialize persist flag to 0

        for(int j=RQS_DATA; j < RQS_PERSISTENT_FLAG; j++){  // zero out data in request message
            send_request[j]=0;
        }
        
        // communicate with server to update client_id.txt

        /**************UPDATE_WSTAT******************/
        if((client_pid = fork())<0){
            sprintf(print_string, "Failed to fork child client process\n");
            printLog(print_string, log_sem);   
        }

        if(client_pid==0){  // child process to send UPDATE_WSTAT
            sockfd = socket(AF_INET, SOCK_STREAM, 0);   // create socket
            if(sockfd==-1){
                sprintf(print_string, "[%d] Failed to create socket\n", client_id);
                printLog(print_string, log_sem);   
            }
            if(connect(sockfd, (struct sockaddr *) &address, sizeof(address))!=0){
                sprintf(print_string, "[%d] Failed to initiate connection\n", client_id);
                printLog(print_string, log_sem);   
            }
            
            sprintf(print_string, "[%d] open connection\n", client_id);
            printLog(print_string, log_sem); 

            send_request[RQS_RQS_CODE_NUM] =  UPDATE_WSTAT; // initialize request code

            count(path_name, &send_request[RQS_DATA]);
            num_files_client++;

            size_sent = send(sockfd, send_request, sizeof(int)*REQUEST_MSG_SIZE, 0);    // send request to server
            if(size_sent!=sizeof(int)*REQUEST_MSG_SIZE){
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);   
                sprintf(print_string, "[%d] Failed to send request\n", client_id);
                printLog(print_string, log_sem);   
                return EXIT_FAILURE;
            }
            size_received = recv(sockfd, receive_response, sizeof(int)*RESPONSE_MSG_SIZE, 0);  // receive short response
            if(size_received!=sizeof(int)*RESPONSE_MSG_SIZE){
                sprintf(print_string, "[%d] Failed to receive response\n", client_id);
                printLog(print_string, log_sem);   
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);   
                return EXIT_FAILURE;
            }
            if(receive_response[RSP_RSP_CODE_NUM]!=RSP_OK){ // check response code is OK
                sprintf(print_string, "[%d] received error from server\n", client_id);
                printLog(print_string, log_sem);   
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);   
                return EXIT_FAILURE;
            }

            if(receive_response[RSP_RQS_CODE_NUM]!=UPDATE_WSTAT){   // check request code is correct
                sprintf(print_string, "received request code num: %d\n", receive_response[RSP_RQS_CODE_NUM]);
                printLog(print_string, log_sem);   
                sprintf(print_string, "actual request code num: %d\n", UPDATE_WSTAT);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] received incorrect request code number\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            if(receive_response[RSP_DATA]!=client_id){  // check client ID is correct
                sprintf(print_string, "received ID: %d\n", receive_response[RSP_DATA]);
                printLog(print_string, log_sem);
                sprintf(print_string, "actual ID: %d\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] received incorrect client ID\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            sprintf(print_string, "[%d] UPDATE_WSTAT: %d\n", client_id, num_files_client);
            printLog(print_string, log_sem);

            close(sockfd);  // close the socket
            sprintf(print_string, "[%d] close connection (successful execution)\n", client_id);
            printLog(print_string, log_sem);
            sem_close(log_sem);
            return EXIT_SUCCESS;    // finish UPDATE_WSTAT    
        }
    }

    // wait for all UPDATE_WSTAT clients to terminate
    pid_t terminated_update_pid;    // pid of terminated client
    int update_exit_stat;   // status of client on exit
    while((terminated_update_pid = wait(&update_exit_stat))>0){
        if(update_exit_stat == EXIT_FAILURE){
            printLog("UPDATE_WSTAT process exited with error. \n", log_sem);
            return EXIT_FAILURE;
        }
    }

    for(int client_id = 1; client_id<=num_clients; client_id++){ // each client now sends remaining 3 messages

        send_request[RQS_CLIENT_ID] = client_id;    // initialize client ID
        send_request[RQS_PERSISTENT_FLAG] = NO_PERSIST;  // initialize persist flag to 0

        for(int j=RQS_DATA; j < RQS_PERSISTENT_FLAG; j++){  // zero out data in request message
            send_request[j]=0;
        }

        /**************GET_MY_UPDATES******************/
        client_pid = fork();
        if(client_pid==0){  // child process to send GET_MY_UPDATES   
            sockfd = socket(AF_INET, SOCK_STREAM, 0);   // create socket
            if(sockfd==-1){
                sprintf(print_string, "[%d] Failed to create socket\n", client_id);
                printLog(print_string, log_sem);   
            }
            if(connect(sockfd, (struct sockaddr *) &address, sizeof(address))!=0){
                sprintf(print_string, "[%d] Failed to initiate connection\n", client_id);
                printLog(print_string, log_sem);   
            }

            sprintf(print_string, "[%d] open connection\n", client_id);
            printLog(print_string, log_sem); 

            send_request[RQS_RQS_CODE_NUM] = GET_MY_UPDATES;
            
            size_sent = send(sockfd, send_request, sizeof(int)*REQUEST_MSG_SIZE, 0);    // send request
            if(size_sent!=sizeof(int)*REQUEST_MSG_SIZE){
                sprintf(print_string,"[%d] failed to send request\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }
            size_received = recv(sockfd, receive_response, sizeof(int)*RESPONSE_MSG_SIZE, 0);   // receive response
            if(size_received!=sizeof(int)*RESPONSE_MSG_SIZE){
                sprintf(print_string, "[%d] failed to receive response\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }
            if(receive_response[RSP_RSP_CODE_NUM]!=RSP_OK){ // check response code is OK
                sprintf(print_string, "[%d] received error from server\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }
            if(receive_response[RSP_RQS_CODE_NUM]!=GET_MY_UPDATES){ // check request code is correct
                sprintf(print_string, "[%d] Received incorrect request code number\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }
            sprintf(print_string, "[%d] GET_MY_UPDATES: %d %d\n", client_id, receive_response[RSP_RSP_CODE_NUM], receive_response[RSP_DATA]);
            printLog(print_string, log_sem);
            
            close(sockfd);  // close the socket
            sprintf(print_string, "[%d] close connection (successful execution)\n", client_id);
            printLog(print_string, log_sem);
            sem_close(log_sem);
            return EXIT_SUCCESS;
        }

        /**************GET_ALL_UPDATES******************/
        client_pid = fork();
        if(client_pid==0){  // child process to send GET_ALL_UPDATES    
            sockfd = socket(AF_INET, SOCK_STREAM, 0);   // create socket
            if(sockfd==-1){
                sprintf(print_string, "[%d] Failed to create socket\n", client_id);
                printLog(print_string, log_sem);   
            }
            if(connect(sockfd, (struct sockaddr *) &address, sizeof(address))!=0){
                sprintf(print_string, "[%d] Failed to initiate connection\n", client_id);
                printLog(print_string, log_sem);   
            }
            sprintf(print_string, "[%d] open connection\n", client_id);
            printLog(print_string, log_sem); 

            send_request[RQS_RQS_CODE_NUM] = GET_ALL_UPDATES;   // update request code number

            size_sent = send(sockfd, send_request, sizeof(int)*REQUEST_MSG_SIZE, 0);    // send request
            if(size_sent!=sizeof(int)*REQUEST_MSG_SIZE){
                sprintf(print_string, "[%d] failed to send request\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }
            size_received = recv(sockfd, receive_response, sizeof(int)*RESPONSE_MSG_SIZE, 0);   // receive request
            if(size_received!=sizeof(int)*RESPONSE_MSG_SIZE){
                sprintf(print_string, "[%d] failed to receive response\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            if(receive_response[RSP_RSP_CODE_NUM]!=RSP_OK){ // check response code is OK
                sprintf(print_string, "[%d] received error from server\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            if(receive_response[RSP_RQS_CODE_NUM]!=GET_ALL_UPDATES){    // check total number of updates correct
                sprintf(print_string, "[%d] received incorrect request code number\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            sprintf(print_string, "[%d] GET_ALL_UPDATES: %d %d\n", client_id, receive_response[RSP_RSP_CODE_NUM], receive_response[RSP_DATA]);
            printLog(print_string, log_sem); 

            close(sockfd);
            sprintf(print_string, "[%d] close connection (successful execution)\n", client_id);
            printLog(print_string, log_sem);
            sem_close(log_sem);
            return EXIT_SUCCESS;
        }

        /**************GET_ALL_UPDATES******************/
        client_pid = fork();
        if(client_pid==0){  // child process to send GET_ALL_UPDATES    
            sockfd = socket(AF_INET, SOCK_STREAM, 0);   // create socket
            if(sockfd==-1){
                sprintf(print_string, "[%d] Failed to create socket\n", client_id);
                printLog(print_string, log_sem);   
            }
            if(connect(sockfd, (struct sockaddr *) &address, sizeof(address))!=0){
                sprintf(print_string, "[%d] Failed to initiate connection\n", client_id);
                printLog(print_string, log_sem);   
            }
            sprintf(print_string, "[%d] open connection\n", client_id);
            printLog(print_string, log_sem); 

            send_request[RQS_RQS_CODE_NUM] = GET_WSTAT; // update request code number

            size_sent = send(sockfd, send_request, sizeof(int)*REQUEST_MSG_SIZE, 0);    // send request
            if(size_sent!=sizeof(int)*REQUEST_MSG_SIZE){
                sprintf(print_string, "[%d] failed to send request\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }
            size_received = recv(sockfd, receive_response, sizeof(int)*LONG_RESPONSE_MSG_SIZE, 0);  // receive response
            if(size_received!=sizeof(int)*LONG_RESPONSE_MSG_SIZE){
                sprintf(print_string, "[%d] failed to receive response\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            if(receive_response[RSP_RSP_CODE_NUM]!=RSP_OK){ // check response code is OK
                sprintf(print_string, "[%d] received error from server\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            if(receive_response[RSP_RQS_CODE_NUM]!=GET_WSTAT){  // check request code is correct
                sprintf(print_string, "[%d] received incorrect request code number\n", client_id);
                printLog(print_string, log_sem);
                sprintf(print_string, "[%d] close connection (unsuccessful execution)\n", client_id);
                printLog(print_string, log_sem);
                return EXIT_FAILURE;
            }

            sprintf(print_string, "[%d] GET_WSTAT: %d ", client_id, receive_response[RSP_RSP_CODE_NUM]);
            char buf[MAX_PRINT_LENGTH];
            for(int l=RSP_DATA; l<RSP_DATA+WORD_LENGTH_RANGE; l++){
                sprintf(buf, "%d ", receive_response[l]);
                strcat(print_string, buf);
            }
            sprintf(buf, "\n");
            strcat(print_string, buf);
            printLog(print_string, log_sem);

            close(sockfd);  // close socket
            sprintf(print_string, "[%d] close connection (successful execution)\n", client_id);
            printLog(print_string, log_sem);
            sem_close(log_sem);
            return EXIT_SUCCESS;
        }
    }
    /**************************************************************************************************/
}

    // wait for all client processes to terminate
    pid_t terminated_client_pid;    // pid of terminated client
    int client_exit_stat;   // status of client on exit
    while((terminated_client_pid = wait(&client_exit_stat))>0){
        if(client_exit_stat == EXIT_FAILURE){
            fprintf(stderr, "Client exited with error. \n");
            return EXIT_FAILURE;
        }
    }

    // check if an error occurred with wait()
    // errno = ECHILD means there were no children to wait for left
    if ((terminated_client_pid == -1) && (errno!=ECHILD)){
        fprintf(stderr, "An error occured waiting for all clients\n");
        return EXIT_FAILURE;
    }

    // close log file
    fclose(logfp);
    sem_close(log_sem); // close the semaphore
    return EXIT_SUCCESS;
}

/*********************************************************************************/

/**
 * create the log file
 */
void createLogFile(void) {
    pid_t p = fork();
    if (p == 0)
        execl("/bin/rm", "rm", "-rf", "log", NULL);

    wait(NULL);
    mkdir("log", ACCESSPERMS);
    logfp = fopen("log/log_client.txt", "w");
}

/**
 * put the file names in the specified directory into array of strings (adapted from HW2)
 * @param input_file_dir the name of the directory
 * @param file_name_array the array of strings to store the file names
 * @param idx the starting index to store names in the array (for recursion purposes)
 * @return error if any
 */
int getFileNames(char *input_file_dir, char file_names_array[][MAX_FILE_NAME_LENGTH], int idx) {

    if(idx<0) { // the index must be a positive integer
        return -1;
    }

    DIR *dr;
    if ((dr = opendir(input_file_dir)) == NULL) {
        fprintf(stderr, "failed to open directory %s\n", input_file_dir);
        return -1;
    }

    struct dirent *entry;
    struct stat file_stat;
    char path_name[MAX_FILE_NAME_LENGTH];
    char *hard_link;
    
    while ((entry = readdir(dr)) != NULL) {  // read each entry of the current directory table
        path_name[0] = '\0';
        strcat(path_name, input_file_dir);  // reset the path name to the path fo the current directory
        strcat(path_name, "/");

        if ((!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))) continue;  // ignore current and upper directories

        strcat(path_name, entry->d_name);                                              // append entry name to the directory
        if (entry->d_type == DT_DIR) {  // recurse and update the index
            idx = getFileNames(path_name, file_names_array, idx);  
        } else {    // process the file
            // get the stats of the file entry
            if (stat(path_name, &file_stat) != 0) { // stat returned error       
                if (entry->d_type == DT_LNK && errno == ENOENT) {  // symbolic link pointing to deleted file
                    fprintf(stderr, "%s is a dangling symoblic link\n", path_name);
                    continue;
                } else {
                    perror("Failed to get stat of directory entry");
                    return -1;
                }
            }
            
            if (entry->d_type == DT_LNK) {                // the file is a symbolic link so we must get real link
                hard_link = realpath(path_name, NULL);      // get the pointer to a buffer containing the absolute hard link
                sprintf(file_names_array[idx], "%s", hard_link);  // add the hard link rather than the symbolic link to the fileNames array
                free(hard_link);
            } else {                                      // the file is a hard link so we directly add name to the fileNames array
                sprintf(file_names_array[idx], "%s", path_name);  // add the filename to the fileNames array
            }
            idx++;
        }
    }

    if (closedir(dr) != 0) {  // close the (sub)directory
        perror("Failed to close directory");
        return -1;
    };

    return idx;
}

/**
 * count word lengths in a file (adapted from HW1)
 * @param intput_file_name the name of the file
 * @param count_array the array to store the counts
 * @return error if any
 */
void count(char *input_file_name, int *count_array) {
    FILE *fp = fopen(input_file_name, "r");
    char curr_line[CHUNK_SIZE];  // line to be read
    int curr_word_len;           // word length
    char *curr_word;            // delcare current word read
    char delim[] = " ";
    while ((getLineFromFile(fp, curr_line, CHUNK_SIZE)) != -1) {  // read the file by line
        curr_word = strtok(curr_line, delim);                     // read the first word
        while (curr_word != NULL) {                              // read line word by word until end of line
            curr_word_len = strcspn(curr_word, "\n");
            if ((curr_word_len > 0) && (curr_word_len <= WORD_LENGTH_RANGE)) {              // does not consider words longer than 20 characters
                count_array[curr_word_len - 1]++;  // increment the length count
            }
            curr_word = strtok(NULL, delim);  // read next word
        }
    }
}

/**
 * get the next line in the file (from HW2 utils.c)
 * @param fp file pointer
 * @param line buffer to store the line
  *@param len the number fo characters to read from line
 * @return the size read from line
 */
ssize_t getLineFromFile(FILE *fp, char *line, size_t len) {
    memset(line, '\0', len);
    return getline(&line, &len, fp);
}

/**
 * print string to standard output and log file given (synchronous)
 * @param line_to_print the string to be printed
 * @param log_sem the semaphore used for synchronization
 * @return error if any
 */
int printLog(char * line_to_print, sem_t *log_sem){
    if(sem_wait(log_sem)!=0){
        fprintf(stderr, "Failed to wait semaphore");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "%s", line_to_print);
    fprintf(logfp, "%s", line_to_print);
    if(sem_post(log_sem)!=0){
        fprintf(stderr, "Failed to post semaphore");
    }
    return EXIT_SUCCESS;
}
