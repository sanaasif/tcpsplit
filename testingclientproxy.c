//this includes threading
//this does not include raw sockets
//do destroy mutext lock
//do exit threads

#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <pthread.h>


struct sockets{
    int clientsocket;
    int serversocket;
};

int id0, id1;


void fromclient(void* sock){
    char buff[1024];
    struct sockets* s = (struct sockets*) sock;
    int n;
    while (1){
        bzero(buff, 1024); 
        n=read(s->serversocket, buff, sizeof(buff));
        if (n>0){  
            printf("received packet");
            printf(buff);
            if (write(s->clientsocket, buff, sizeof(buff))<0){
                perror("error writing to serverproxy");
                id1=1;
                pthread_exit(&id1);
            }
        }
        if (n<0){
            perror("error reading from client browser");
            id1=1;
            pthread_exit(&id1);
        }
    }
    
}


void fromserver(void* sock){
  char buff[1024];
    struct sockets* s = (struct sockets*) sock;
    int n;
    while (1){
        bzero(buff, 1024);
        n=read(s->clientsocket, buff, sizeof(buff));
        if (n>0){
            printf(buff);
            if (write(s->serversocket, buff, sizeof(buff))<0){
                perror("error writing to client browser");
                //break;
                id0=1;
                pthread_exit(&id0);
            }
        }if (n<0){
            perror("error reading from serverproxy");
            //break;
            id0=1;
            pthread_exit(&id0);
        }
    }      
}


int connecting_server(int port, void* hostname){
    int clientsocket, sock;
    struct sockaddr_in host_addr;
    struct hostent* host;
    host = (struct hostent*)hostname;
    //host=gethostbyname(hostname);
    bzero((char*)&host_addr,sizeof(host_addr));
    host_addr.sin_port=htons(port);
    host_addr.sin_family=AF_INET;
    bcopy((char*)host->h_addr,(char*)&host_addr.sin_addr.s_addr,host->h_length);
       
    sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    clientsocket=connect(sock,(struct sockaddr*)&host_addr,sizeof(struct sockaddr));
    //sprintf(buffer,"\nConnected to %s  IP - %s\n",t2,inet_ntoa(host_addr.sin_addr));
    if(clientsocket<0)
    {
        perror("Error in connecting to remote server");

    }
    return sock;
}

int serversocketfunc(){
    int serversocket, connfd, len;
    struct sockaddr_in servaddr, cli;
    serversocket = socket(AF_INET, SOCK_STREAM, 0); 
    if (serversocket == -1) { 
        printf("serversocket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("serversocket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 

    // assign IP, PORT 
    servaddr.sin_family = AF_UNSPEC; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(8080); 

    // Binding newly created serversocket to given IP and verification 
    if ((bind(serversocket, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
        printf("serversocket bind failed...\n"); 
        return -1; 
    } 
    else
        printf("serversocket successfully binded..\n"); 

    if ((listen(serversocket, 5)) != 0) { 
        printf("Listen failed...\n"); 
        return -1; 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 

    connfd = accept(serversocket, (struct sockaddr *)&cli, &len); 

    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        return -1; 
    } 
    else
        printf("server acccept the client...\n"); 
    return connfd;
}

int clientsocketfunc(){
    int clientsocket;
    struct sockaddr_in proxyserveraddr;
    clientsocket = socket(AF_INET, SOCK_STREAM, 0); 
    if (clientsocket == -1) { 
        printf("clientsocket creation failed...\n"); 
        return clientsocket;
    } 
    else
        printf("clientsocket successfully created..\n"); 
    //bzero(&servaddr, sizeof(servaddr)); 

    proxyserveraddr.sin_family = AF_INET; 
    proxyserveraddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //IP of server proxy 
    proxyserveraddr.sin_port = htons(8088); // port of server proxy

    connect(clientsocket, (struct sockaddr *)&proxyserveraddr, sizeof(proxyserveraddr));
    return clientsocket;
}

int main(){

    pthread_t thread_id[2]; 
    int clientsocket, len, connfd;  
   // struct sockaddr_in proxyserveraddr, servaddr;
    char buff[1024];
    int n;
    struct sockets s;
    int** ptr[2];
    // socket create and verification 
    connfd=serversocketfunc();
    //sending packet 
    clientsocket=clientsocketfunc();
    if (clientsocket==-1 || connfd==-1){
        printf("error in connecting tcp");
        exit(0);
    }
    printf("connected");
    s.clientsocket=clientsocket;
    s.serversocket=connfd;
    pthread_create(&thread_id[1],NULL,fromclient,(void*)&s);
    pthread_create(&thread_id[0], NULL,fromserver,(void*)&s); //from serverproxy
    pthread_join(thread_id[0], (void**) &(ptr[0]));
    pthread_join(thread_id[1], (void**)&(ptr[1]));
}
