#include <stdio.h>
#include "mylist.h"
#include <stdlib.h>
#include "mdb.h"
#include <string.h>
static void printChar(void *p,int x)
{
    printf("%4d: {%s} said {%s}\n",x,(char *)p,(char *)p+16);
}
static int compareStr(void *p,char *s){
	char *first = (char *)p;
	char *second = (char *)(first+16);
	if((strstr(first,s)!=NULL)||(strstr(second,s)!=NULL))
		return 1;
	return 0;
}
int main(int argc, char **argv){
	char *name=argv[1];
    if (argc != 2) {
        fprintf(stderr, "%s\n", "usage: mdb-lookup <file_name>");
        exit(1);
    }
    FILE *fp = fopen(name, "r");
	if (fp == NULL) {
        	perror(name);
		exit(1);
	}
	
	struct List listname;
	initList(&listname);/*
	struct List listmessage;
	initList(&listmessage);
	*/
	char message[40];
	struct Node *node=NULL;
	while(fgets(message,sizeof(message)+1,fp)!=NULL){
		char *n = malloc(sizeof(message));
		strcpy(n,message);
		strcpy(n+16,message+16);
		node=addAfter(&listname,node,n);
		if(node==NULL){
                        printf("failed insert\n");
                        return 0;
                }
	}


	char search[1000];
while(1){
	printf("lookup: ");
	int lineno=1;
	int x=0;
	int this=0;
	while(this!=-1&&this!=(int)'\n'){
		this=fgetc(stdin);
		*(search+x)=this;
		x++;
	}
	if(this==(int)'\n'){
		*(search+x-1)=0;
	}else if(this==-1){
		break;
	}
	char find[6];
	strncpy(find,search,5);
	find[5]=0;
	if(*search==0){
		node=listname.head;
		while(node){
			printChar(node->data,lineno);
			node=node->next;
			lineno++;
		}
	}else{
	node = listname.head;
	while(node){
		if(compareStr(node->data,find)){
			printChar(node->data,lineno);
		}
		node=node->next;
		lineno++;
		}	
	}
}
	node = listname.head;
	while(node){
		free(node->data);
		node = node->next;
	}
	removeAllNodes(&listname);
if (ferror(fp)) {
        perror(name);
        exit(1);
}
	 fclose(fp);
	return 0;

}

