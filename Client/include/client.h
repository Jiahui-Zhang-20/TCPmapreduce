/* test machine: csel-kh1250-10.cselabs.umn.edu
 * date: 4/21/2021
 * name: Daniel Black, Tam Nguyen, Jiahui Zhang
 * x500: blac0352, nguy3492, zhan7554
*/

#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include "../include/protocol.h"

// macros for mapreduce
#define MAX_NUM_FILES           200
#define MAX_FILE_NAME_LENGTH    200
#define CHUNK_SIZE              1024

// for synchronous printing to standard out and log file
#define MAX_PRINT_LENGTH        1024
#define LOG_SEM_NAME            "log_sem"
#define LOG_SEM_OFLAG           ( O_CREAT | O_EXCL )
#define LOG_SEM_PERMISSIONS     ( S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP )
#define LOG_SEM_INIT_VAL        1

FILE *logfp;    // file to print log to

char input_file_names[MAX_NUM_FILES][MAX_FILE_NAME_LENGTH];  // array of file names to read

/**
 * create the log file
 */
void createLogFile(void);

/**
 * put the file names in the specified directory into array of strings (adapted from HW2)
 * @param input_file_dir the name of the directory
 * @param file_name_array the array of strings to store the file names
 * @param idx the starting index to store names in the array (for recursion purposes)
 * @return error if any
 */
int getFileNames(char *input_file_dir, char file_names_array[][MAX_FILE_NAME_LENGTH], int idx);

/**
 * count word lengths in a file (adapted from HW1)
 * @param intput_file_name the name of the file
 * @param count_array the array to store the counts
 * @return error if any
 */
void count(char *input_file_name, int *count_array);

/**
 * get the next line in the file (from HW2 utils.c)
 * @param fp file pointer
 * @param line buffer to store the line
  *@param len the number fo characters to read from line
 * @return the size read from line
 */
ssize_t getLineFromFile(FILE *fp, char *line, size_t len);

/**
 * print string to standard output and log file given (synchronous)
 * @param line_to_print the string to be printed
 * @param log_sem the semaphore used for synchronization
 * @return error if any
 */
int printLog(char * line_to_print, sem_t *log_sem);

#endif // CLIENT_H
