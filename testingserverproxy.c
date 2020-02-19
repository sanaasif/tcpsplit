//assuming its a request for one site only...
#include<pthread.h>
#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <inttypes.h> /* strtoimax */


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
    servaddr.sin_port = htons(8088); 

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

int serverproxyfunc(char* buff){
    char buffer[510],t1[300],t2[300],t3[10];
    char* temp=NULL;
    int port, flag, i;
    struct hostent* host; //(as a client to server)
    int clientsocket;
    sscanf(buff,"%s %s %s",t1,t2,t3);
   
    if(((strncmp(t1,"GET",3)==0))&&((strncmp(t3,"HTTP/1.1",8)==0)||(strncmp(t3,"HTTP/1.0",8)==0))&&(strncmp(t2,"http://",7)==0))
    {
        strcpy(t1,t2);
    //t2 is copied to t1
        flag=0;   
        for(i=7;i<strlen(t2);i++)
        {
            if(t2[i]==':')
            {
                flag=1;
                break;
            }
        }
           
        temp=strtok(t2,"//");
        //splitting a string by "//" and returns string after // which will be host and port
        if(flag==0)
        {
            port=80;
            temp=strtok(NULL,"/");
        }
        else
        {
            temp=strtok(NULL,":");
            temp=strtok(NULL,"/"); //modified
            port=atoi(temp); //modified
        //so if port is specified, temp=port
        }
           
        sprintf(t2,"%s",temp);
        printf("host = %s",t2);
        host=gethostbyname(t2);
           
        strcat(t1,"^]");
        temp=strtok(t1,"//");
        temp=strtok(NULL,"/");
        if(temp!=NULL)
        temp=strtok(NULL,"^]");
        printf("\npath = %s\nPort = %d\n",temp,port);
        int c=0;
      
        clientsocket=connecting_server(port, host);
        printf("connected to host");
        bzero((char*)buff,sizeof(buff));
        if(temp!=NULL)
        sprintf(buff,"GET /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",temp,t3,t2);
        else
        sprintf(buff,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",t3,t2);
        if (write(clientsocket, buff, sizeof(buff))<0){
            perror("error writing to server");
        }
        return clientsocket;
    }
}

int main(){
    int serversocket, connfd, len; 
    int clientsocket;
    int index[2];
    pthread_t thread_id[2]; 
    struct sockets s;
    int* ptr[2];
    //char buff[510];
    char* temp=NULL;
    int port, flag, i;
    struct hostent* host;
    char buff[510],t1[300],t2[300],t3[10];

    //connfd is the connection with the client proxy - acts as a serversocket
    //clientsocket talks to server address provided in packet header 

    connfd=serversocketfunc();
    printf("does it not even reach here?");
    if (connfd>=0){
        bzero(buff, 510); 
        if (read(connfd, buff, sizeof(buff))<0){
            perror("error in reading");
        } 
        printf("packet received \n");
        clientsocket=serverproxyfunc(buff);
        //clientsocket=serverproxyfunc(buff);
            
        s.clientsocket=clientsocket;
        s.serversocket=connfd;
        pthread_create(&thread_id[0],NULL,fromserver,(void*)&s);
        pthread_create(&thread_id[1],NULL,fromclient,(void*)&s); //from client proxy
        pthread_join(thread_id[0], (void**) &(ptr[0]));
        pthread_join(thread_id[1], (void**)&(ptr[1]));
    }

}
    
