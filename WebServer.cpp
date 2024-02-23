#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
using namespace std;
#define ServerString "Server : OldColorGhost's httpd/1.0\r\n"
int ServerSocket;
int port;
sockaddr_in LocalIP;
socklen_t LocalIP_len;
void error(const char *message){
    printf("%s:%d\n",message,errno);
    exit(0);
}
int getLinefromSocket(int socket,char *buf,int size){
    int count=0;
    char c='\0';
    int res;
    while(c!='\n'&&count<size-1){
        res=recv(socket,&c,1,0);
        if(res>0){
            if(c=='\r'){
                res=recv(socket,&c,1,MSG_PEEK);
                if(res>0&&c=='\n')
                    recv(socket,&c,1,0);
                else
                    c='\n';
            }
            buf[count]=c;
            count++;
        }
        else
            c='\n';
    }
    buf[count]='\0';
    return count;
}
void getfile(int clinet,FILE *resource){
    char buffer[1024];
    fgets(buffer,sizeof(buffer),resource);
    while(!feof(resource)){
        send(clinet,buffer,strlen(buffer),0);
        fgets(buffer,sizeof(buffer),resource);
    }
}
void init(){
    LocalIP.sin_family=AF_INET;
    LocalIP.sin_port=htons(1234);
    LocalIP.sin_addr.s_addr=inet_addr("127.0.0.1");
    LocalIP_len=sizeof(LocalIP);
    ServerSocket=socket(AF_INET,SOCK_STREAM,0);
    int on=1;
    if(ServerSocket==-1)
        error("socket");
    if(setsockopt(ServerSocket,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on))==-1)
        error("setsockopt");
    if(bind(ServerSocket,(sockaddr *)&LocalIP,LocalIP_len)==-1)
        error("bind");
    if(listen(ServerSocket,5)==-1)
        error("listen");
}
void method501(int Socket){
    int msg_len=1024;
    char *msg=(char *)malloc(msg_len);
    sprintf(msg,"HTTP/1.0 501 Method Not Implemented\r\n");
    send(Socket,msg,strlen(msg),0);
    sprintf(msg,ServerString);
    send(Socket,msg,strlen(msg),0);
    sprintf(msg,"Content-Type: text/html\r\n");
    send(Socket,msg,strlen(msg),0);
    sprintf(msg,"\r\n");
    send(Socket,msg,strlen(msg),0);
    FILE *fd=fopen("html/501.html","r");
    if(fd==NULL)
        error("fopen");
    getfile(Socket,fd);
    fclose(fd);
    free(msg);
}
void method404(int Socket){
    int msg_len=1024;
    char *msg=(char *)malloc(msg_len);
    sprintf(msg,"HTTP/1.0 404 Not Found\r\n");
    send(Socket,msg,strlen(msg),0);
    sprintf(msg,ServerString);
    send(Socket,msg,strlen(msg),0);
    sprintf(msg,"Content-Type: text/html\r\n");
    send(Socket,msg,strlen(msg),0);
    sprintf(msg,"\r\n");
    send(Socket,msg,strlen(msg),0);
    FILE *fd=fopen("html/404.html","r");
    if(fd==NULL)
        error("fopen");
    getfile(Socket,fd);
    fclose(fd);
    free(msg);
}
void header(int clinet,FILE *resource){
    char buffer[1024];
    strcpy(buffer,"HTTP/1.1 200 OK\r\n");
    send(clinet,buffer,strlen(buffer),0);
    strcpy(buffer,ServerString);
    send(clinet,buffer,strlen(buffer),0);
    strcpy(buffer,"Content-Type: text/html\r\n");
    send(clinet,buffer,strlen(buffer),0);
    strcpy(buffer,"\r\n");
    send(clinet,buffer,strlen(buffer),0);
}
void serve_file(int clinet,const char *path){
    FILE *resource=NULL;
    char *buf=(char*)malloc(1024);
    int numchar=1;
    buf[0]='$',buf[1]='\0';
    while((numchar>0)&&(strcmp(buf,"\n")))
    {
        numchar=getLinefromSocket(clinet,buf,1024);
        printf("%s",buf);
    }
    resource=fopen(path,"r");
    if(resource==NULL)
        error("resource");
    header(clinet,resource);
    getfile(clinet,resource);
    fclose(resource);
}
void *acceptRequest(void *arg){
    int Client=(intptr_t)arg;
    printf("%d\n",Client);
    int buffer_len=1000;
    char *buffer=(char *)malloc(buffer_len);
    char method[20];
    char url[200];
    char path[200];
    int count;
    int isDone=0;
    count=getLinefromSocket(Client,buffer,buffer_len);
    int index=0,j=0;
    while(isspace(buffer[index])==0&&(j<sizeof(method)-1)){
        method[j]=buffer[index];
        index++;j++;
    }
    method[j]='\0';j=0;
    while(isspace(buffer[index])!=0&&(index<count))
        index++;
    while(isspace(buffer[index])==0&&(j<sizeof(url)-1)){
        url[j]=buffer[index];
        j++;index++;
    }
    free(buffer);
    url[j]='\0';j=0;
    path[0]='.';path[1]='\0';
    printf("%s %s\n",method,url);


    if(strcasecmp(method,"GET")==0){
        if(strcmp(url,"/\0")==0)
            strcat(path,"/index.html");
        else
            strcat(path,url);
        isDone=1;
    }
    printf("%s %s\n",method,path);


    if(isDone==0)
        method501(Client);
    else{
        serve_file(Client,path);
    }

    close(Client);
    return ((void*)0);
}
int main(){
    init();
    sockaddr_in ClientIP;
    socklen_t ClinetIP_len=sizeof(ClientIP);
    int ClinetSocket;
    pthread_t newclient;
    
    /*
    while(1){
        ClinetSocket=accept(ServerSocket,(sockaddr *)&ClientIP,&ClinetIP_len);
        if(ClinetSocket==-1)
            error("accept");
        if(pthread_create(&newclient,NULL,acceptRequest,(void *)(intptr_t)ClinetSocket)!=0)
            error("threat");
    }
    */
    
    ClinetSocket=accept(ServerSocket,(sockaddr *)&ClientIP,&ClinetIP_len);
    acceptRequest((void *)(intptr_t)ClinetSocket);
    return 0;
}