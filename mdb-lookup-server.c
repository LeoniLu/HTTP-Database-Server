#include <stdio.h>
#include "mylist.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "mdb.h"
static void die(const char *s) { perror(s); exit(1); }
int main(int argc, char **argv){
    if (argc != 3) {
        fprintf(stderr, "%s\n", "usage: mdb-lookup <database_file> <port>");
        exit(1);
    }
    char *filename = argv[1];

    int port = atoi(argv[2]);
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
            die("signal() failed");
    int servsock;
    if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            die("socket failed");
    struct sockaddr_in serveraddr;
    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);
    serveraddr.sin_port=htons(port);
        
    if(bind(servsock,(struct sockaddr *)&serveraddr,sizeof(serveraddr))<0)
            die("bind failed");
    if(listen(servsock,5)<0)
            die("listen failed");
    int r;
    char buf[100];
    char buff[100];

    int clntsock;
    struct sockaddr_in clntaddr;
    socklen_t clntlen=sizeof(clntsock);


    while (1) {
        if ((clntsock = accept(servsock,(struct sockaddr *) &clntaddr, &clntlen)) < 0){
            die("accept failed");
        }else if(clntsock>0){
            while((r = recv(clntsock, buf, sizeof(buf), 0)) > 0){
                printf("received r is %d\n",r);
		buf[r]=0;
                FILE *fp = fopen(filename, "rb");
                if (fp == NULL)
                     die(filename);
                
                struct List list;
                initList(&list);
                int loaded = loadmdb(fp, &list);
                if (loaded < 0)
                     die("loadmdb");
                fclose(fp);   
                
                char key[5 + 1];
    
                    strncpy(key, buf, sizeof(key) - 1);
                    key[sizeof(key) - 1] = '\0';
                    size_t last = strlen(key) - 1;
                    if (key[last] == '\n')
                        key[last] = '\0';
                    struct Node *node = list.head;
                    int recNo = 1;
		    printf("now the key is %s\n",key);
                    while (node) {
                        struct MdbRec *rec = (struct MdbRec *)node->data;
                        if (strstr(rec->name, key) || strstr(rec->msg, key)) {
                            sprintf(buff,"%4d: {%s} said {%s}\n", recNo, rec->name, rec->msg);
                            r=send(clntsock,buff,strlen(buff),0);
                            printf("sent r is %d\n",r);
                        }
                        node = node->next;
                        recNo++;
                    }
                    sprintf(buff,"\n");
                    send(clntsock,buff,strlen(buff),0);
        	
		freemdb(&list);
	    }
	 }
    }
}

