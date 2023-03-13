// Libs for system layer (IPC part)
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <iconv.h>

// Libs for network layer (p2p part)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

// constants for IPC part
#define MAX 100
#define COOR_MESSAGE_TYPE 1
#define FROM_PY_TO_C 2
#define FROM_C_TO_PY 3
#define KEY 192002

//constants for p2p part
#define PORT 4444
#define name "player_1"
#define BUFFER_SIZE 1024

//message structure to put into ipc message queue 
typedef struct message {
    long message_type;
    char message_body[100];
}message;

// System layer. IPC using message queue to communicate between two processes.
void send_msg_to_python_process(char* msg);

char *read_msg_from_python_process();

// Network layer. P2P using socket to communicate with others peers in network.
void config_peer(int* server_fd, struct sockaddr_in* address);
void *send_to_peer();
void receive_from_peer(int server_fd);
void *receive_thread(void *server_fd);

int remove_enter_in_buffer(char* buffer);

int main() {

    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    config_peer(&server_fd, &address);

    // Each thread has its identifier.
    pthread_t t1;
    pthread_t t2;

    pthread_create(&t1, NULL, send_to_peer, NULL);
    pthread_create(&t2, NULL, receive_thread, &server_fd);

    pthread_join(t2, NULL);
    pthread_join(t1, NULL);    

    return 0;
}

void config_peer(int* server_fd, struct sockaddr_in* address) {
    //create sockfd for peer. In p2p architecture each node can be considered as server as well as client.
    if( ( *server_fd = socket(AF_INET, SOCK_STREAM, 0) ) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    //create address structure for peer
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(PORT);

    printf("IP address is: %s\n", inet_ntoa(address->sin_addr));
    printf("port is: %d\n", (int)ntohs(address->sin_port));

    //bind address structure to peer
    if(bind(*server_fd, (struct sockaddr*)address, sizeof(*address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //listen for the connection
    if(listen(*server_fd, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
}

void *send_to_peer() {
    while(1) {
        // printf("\nstart read msg from python process\n");
        // char* hello = read_msg_from_python_process();
        // printf("\n%s\n", hello);
        char buffer__[2000] = {0};
        int PORT_to_send; // port of peer of player 2
        printf("Enter the port to send message:");
        scanf("%d", &PORT_to_send);

        int sock = 0, valread;
        struct sockaddr_in serv_addr;
    
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\nsocket creation error\n");
            exit(EXIT_FAILURE);
        }

        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(PORT_to_send);
        serv_addr.sin_family = AF_INET;

        if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("\nconnection failed\n");
            exit(EXIT_FAILURE);
        }
        printf("\nconnect sucessfully, start reading msg from python process\n");
        char* buffer = read_msg_from_python_process();

        // sprintf(buffer__, "%s[PORT:%d] says: %s", name, PORT, buffer__);
        if(send(sock, buffer, strlen(buffer) + 1, 0) == -1) {
            perror("sending failure");
            exit(EXIT_FAILURE);
        }
        printf("\nmessage sent\n");
        close(sock);
    }
}

void *receive_thread(void *server_fd) {
    int s_fd = *((int*)server_fd);
    while(1) {
        sleep(2);
        receive_from_peer(s_fd);
    }
}

void receive_from_peer(int server_fd) {
    struct sockaddr_in address;
    int valread;
    char buffer[2000] = {0};
    int addrlen = sizeof(address);
    fd_set current_sockets, ready_sockets;

    //Initialize my current set
    FD_ZERO(&current_sockets);
    FD_SET(server_fd, &current_sockets);
    int k = 0;
    while (1)
    {
        k++;
        ready_sockets = current_sockets;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0)
        {
            perror("Error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &ready_sockets))
            {

                if (i == server_fd)
                {
                    int client_socket;

                    if ((client_socket = accept(server_fd, (struct sockaddr *)&address,
                                                (socklen_t *)&addrlen)) < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_socket, &current_sockets);
                }
                else
                {
                    valread = recv(i, buffer, sizeof(buffer), 0);
                    printf("\nlen: %d, buff: %s\n", strlen(buffer), buffer);
                    // char buffer__[1024];
                    // printf("\nenter the coor: ");
                    // scanf("%s", buffer__);
                    send_msg_to_python_process(buffer);
                    FD_CLR(i, &current_sockets);
                }
            }
        }

        if (k == (FD_SETSIZE * 2))
            break;
    }
}

void send_msg_to_python_process(char* buffer) {
    //Open the message queue to communicate to process python. The message_queue has its integer identifier
    int message_queueID = msgget(KEY, 0666 | IPC_CREAT);
    printf("\nmessage queue id: %d\n", message_queueID);

    message msg;
    msg.message_type = FROM_C_TO_PY;
    buffer[strlen(buffer)] = '\n';
    strncpy(msg.message_body, buffer, strlen(buffer) + 1);

    if(msgsnd(message_queueID, &msg, sizeof(msg), 0) == -1) {
        perror("send msg failed to python process");
    }
}

char *read_msg_from_python_process() {
    //Open the message queue to communicate to process python. The message_queue has its integer identifier
    int message_queueID = msgget(KEY, 0666 | IPC_CREAT);
    printf("\nmessage queue id: %d\n", message_queueID);


    message msg;

    // Receive the coor message type
    msg.message_type = FROM_PY_TO_C; 

    if(msgrcv(message_queueID, &msg, sizeof(msg), msg.message_type, 0) == -1) {
        perror("read msg failed from python process");
    }
    
    char* buffer = (char*)calloc(BUFFER_SIZE, 1);

    strncpy(buffer, msg.message_body, sizeof(msg.message_body));
    remove_enter_in_buffer(buffer);
    printf("\nlen: %d, msg from python: %s\n",strlen(buffer), buffer);

    return buffer;
}

int remove_enter_in_buffer(char* buffer) {
    /*
    replace '\n' in buffer by '\0'
    Params: buffer
    Return: the len of buffer from beginning to '\0'
    */
    int k;
    for(k = 0; k < BUFFER_SIZE; k++) {
        if(buffer[k] == '\n') {
            buffer[k] = '\0';
            break;
        }
    }
    return k;
}