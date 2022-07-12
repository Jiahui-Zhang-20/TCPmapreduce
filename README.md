<div id="top"></div>

<!-- PROJECT LOGO -->
<br />
<div align="center">
  <a href="https://github.com/Jiahui-Zhang-20/TCPmapreduce">
    <img src="https://www.tutorialspoint.com/map_reduce/images/mapreduce_work.jpg" alt="Logo" width="400" height="200">
  </a>

  <h1 align="center">Socket-based MapReduce - Word Counts of Different Lengths</h1>

<!-- ABOUT THE PROJECT -->
## About The Project

This project implements a word counter using the MapReduce paradigm. This is a server-client program in which the client performs the mapping procedure and the server performs the reducing procedure.

The client parses the specified text files and sends the statistics to the server. In this case, the client spawns mapper child processes that map the words in the files to their word lengths and send the statistics to the server. The Server creates a thread to establish a TCP connection with each corresponding client child process. A server thread is a reducer that updates the overall word count histogram.

The client will in return will receive updated word count histogram status as a response from the server. The messages between the client child processes and the server threads are logged in a log file named a log file in log/log_client.txt.

### Project Motivation

The idea of this project originated from CSCI 4061: Operating Systems at the University of Minnesota, Twin Cities taught my Dr. Abhishek Chandra. However, our particular implementation expands on the original specifications. In particular, the MapReduce protocol is improved. The server-client TCP connection is persistent over the lifetime of a client child process. The TCP connection between a client and the server does not terminate until the client process has read all the text files it is responsible for and has sent all statistics to the server.

The implementation of this project heavily involved many operating system concepts such as multi-processing, multi-threading, socket programming, synchronization with POSIX semaphores and mutex locks, and memory management.

<p align="right">(<a href="#top">back to top</a>)</p>

## Usage

### Server (server.c)
The server is multithreaded and responsible for listening for incoming socket connections from the clients. The server receives requests from clients and responds to requests with responses.

### Client (client.c)

The consumer program uses multiple processes to process the text files and send requests to the server. Without the [-e] flag, each client process only sends exactly one request to the server. By enabling the [-e] flag, each client establishes a persistent connection with the server and sends all requests to files assigned to them.

The client prints all output both to the terminal and a log file in log/log_client.txt. A named semaphore named "log_sem" is created then unlinked in /dev/shm during execution to synchronize the outputs in the log.

Go into the root directory and use the provided Makefile to compile the server and the client.

<div align="left">

```
> cd Server
> make clean
> make

> cd Client
> make clean
> make	
```

</div>
Then execute the server on a some chosen port

<div align="left">

```
> .Server/server <Server Port>
```

</div>

Next,  execute the client code using the same port chosen for the server.

<div align="left">

```
> Client/client <Folder Name> <# of Clients> <Server IP> <Server Port> [-e]
```
</div>
An example of the above would be

<div align="left">

```
> ./client Testcases/TC4 10 127.0.0.1 8080 -e
```
</div>


### Test Cases
There are several test cases provided in the Client folder.

### Specifications

We assume that the maximum number of text files in the specified directory is 200 and the maximum file name is 1024.

<p align="right">(<a href="#top">back to top</a>)</p>

<!-- CONTACT -->
## Contact

Jiahui Zhang - jiahui.zhang.20@gmail.com

Daniel Black - https://www.linkedin.com/in/danielblack2022/

Tam Nguyen - https://www.linkedin.com/in/tam-nguyen-7181a7128/

Project Link: [https://github.com/Jiahui-Zhang-20/TCPmapreduce](https://github.com/Jiahui-Zhang-20/TCPmapreduce)

<p align="right">(<a href="#top">back to top</a>)</p>

## References

Beej's Guide to Network Programming Using Internet Sockets  - https://beej.us/guide/bgnet/

Modern Operating Systems (4th Edition) by Andrew S. Tanenbaum - https://www.cs.vu.nl/~ast/

UNIX Systems Programming: Communication, Concurrency and Threads: Communication, Concurrency and Threads by Kay A. Robbins - https://www.pearson.com/store/p/unix-systems-programming-communication-concurrency-and-threads-communication-concurrency-and-threads/P100001429235/9780134424071

Advanced Programming in the Unix Environment by W. Richard Stevens - https://stevens.netmeister.org/631/