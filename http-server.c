#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>

static void die(const char *s) { perror(s); exit(1); }

int main(int argc, char **argv)
{
    if (argc != 5){
        fprintf(stderr, "usage: ./http-server <server_port> <web_root> <mdb-lookup-host> <mdb-lookup-port>");
        exit(1);
    }
    unsigned short serverport = atoi(argv[1]);
    const char *web_root = argv[2];
    const char *mdb_host = argv[3];
    unsigned short mdbport = atoi(argv[4]);
    
    int serversock;
    if((serversock = socket(AF_INET, SOCK_STREAM, 0))<0)
        die("socket failed");
    
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0,sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(serverport);
    
    if(bind(serversock,(struct sockaddr *)&serveraddr, sizeof(serveraddr))<0)
        die("bind failed");
    
    if(listen(serversock, 5)<0)
        die("listen failed");
    
    int clientsock;
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    
    char buf[4096];
    char request[4096];
    int r;
    char msg[1024];
    
    
//code related to part2 (b)
    int mdbsock;
    if((mdbsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("mdbsocket failed");
    struct sockaddr_in mdbaddr;
    memset(&mdbaddr,0,sizeof(mdbaddr));
    mdbaddr.sin_family = AF_INET;
    mdbaddr.sin_addr.s_addr=inet_addr(mdb_host);
    mdbaddr.sin_port = htons(mdbport);
    
    if(connect(mdbsock,(struct sockaddr *)&mdbaddr,sizeof(mdbaddr))<0)
        die("mdbsocket connect failed");
    FILE *mdb = fdopen(mdbsock,"w+");
//end of code related to part2 (b)    
    while(1){
        if((clientsock = accept(serversock,(struct sockaddr *)&clientaddr, &len))<0)
            die("accepted failed");
        char *clientip = inet_ntoa(clientaddr.sin_addr);
        fprintf(stderr, "%s ",clientip);
        
        if(recv(clientsock, request,sizeof(request),0)>0){
            char *token_separators = "\t \r\n"; // tab, space, new line 
            char *method = strtok(request, token_separators);
            char *requestURL = strtok(NULL, token_separators);
            char *httpVersion = strtok(NULL, token_separators);
            fprintf(stderr,"\"%s %s %s\" ",method, requestURL, httpVersion);
            char checkmdb[12];
            strncpy(checkmdb,requestURL,11);
            *(checkmdb+11)=0;
            if(strcmp(checkmdb,"/mdb-lookup")){
                if(strcmp(method,"GET")){
                    //bad implementation
                    sprintf(buf,"HTTP/1.0 501 Not Implemented\r\n\r\n<html><body><h1>501 Not Implemented</h1></body></html>\r\n");
                    send(clientsock,buf,strlen(buf),0);
                    fprintf(stderr,"501 Not Impletmented\n");
                    close(clientsock);
                }else{
                    char URL[100];
                    strcpy(URL,requestURL);
                    
                    int URLlen = strlen(URL);
                    if((*(URL+URLlen-1))=='/'){
                        strcpy(URL+URLlen,"index.html");
                    }
                    if((strstr(URL,"/.."))||((*URL)!='/')){
                        //bad path
                        sprintf(buf,"HTTP/1.0 400 Bad Request\r\n\r\n");
                        fprintf(stderr,"400 Bad Request\n");
                        send(clientsock,buf,strlen(buf),0);
                        close(clientsock);
                    }else{
                        char filepath[100];
                        sprintf(filepath,"%s%s",web_root,URL);
                        struct stat status;
                        
                        if((stat(filepath,&status)<0)||(strstr(requestURL,"/.."))){
                            //file not found
                            sprintf(buf,"HTTP/1.0 404 Not Found\r\n\r\n");
                            send(clientsock,buf,strlen(buf),0);
                                fprintf(stderr,"404 Not Found\n");
                                close(clientsock);
                        }else{
                            //good path
                            if(S_ISREG(status.st_mode)){
                                //is regular file
                                sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n");
                                send(clientsock,buf,strlen(buf),0);
                                fprintf(stderr,"200 OK\n");
                                FILE *myfile = fopen(filepath,"rb");
                                if(myfile<0)
                                    die("open file failed");
                                while((r=fread(buf,1,sizeof(buf),myfile))>0){
                                    send(clientsock,buf,r,0);
                                }
                                fclose(myfile);
                                close(clientsock);
                            }else{
                                //is a directory
                                char newpath[100];
                                sprintf(newpath,"%s/",URL);
                                sprintf(buf,"HTTP/1.0 301 Moved Permanently\r\nLocation: http://clac.cs.columbia:%d%s\r\n\r\n",serverport,newpath);
                                send(clientsock,buf,strlen(buf),0);
                                fprintf(stderr,"301 Moved Permanently\n");
                                close(clientsock);
                            }
                        }
                    }
                    
                    
                    
                }
                
                
            }else{
                //enter mdb-lookup
                if(!strcmp(requestURL,"/mdb-lookup")){
                    sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n");
                    send(clientsock,buf,strlen(buf),0);
                    const char *form = "<h1>mdb-lookup</h1>\n"
                            "<p>\n"
                            "<form method=GET action=/mdb-lookup>\n" 
                            "lookup: <input type=text name=key>\n" 
                            "<input type=submit>\n"
                            "</form>\n"
                            "<p>\n"
                            "</body></html>\n";
                    send(clientsock,form,strlen(form),0);
                    fprintf(stderr,"200 OK\n");
                    close(clientsock);
                }else if(strstr(requestURL,"/mdb-lookup?key=")){
                    char word[100];
                    strcpy(word,requestURL+16);
                    r = strlen(word);
                    word[r]='\n';
                    word[r+1]=0;
                    sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n");
                    send(clientsock,buf,strlen(buf),0);
                    const char *form = "<h1>mdb-lookup</h1>\n"
                            "<p>\n"
                            "<form method=GET action=/mdb-lookup>\n" 
                            "lookup: <input type=text name=key>\n" 
                            "<input type=submit>\n"
                            "</form>\n"
                            "<p>\n"
                            "<p><table border>\n";
                    send(clientsock,form,strlen(form),0);
                    fwrite(word,1,strlen(word),mdb);
                    while((fgets(msg,sizeof(msg),mdb)!=0)&&strcmp(msg,"\n")){
                        sprintf(buf,"\n<tr><td> %s",msg);
                        send(clientsock,buf,strlen(buf),0);
                    }
                    sprintf(buf,"\n</table>\n</body></html>");
                    send(clientsock,buf,strlen(buf),0);
                    fprintf(stderr,"200 OK\n");
                    close(clientsock);
                    
                }
            }
            
            


        }

    }

}
