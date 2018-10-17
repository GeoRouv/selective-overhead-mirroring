#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include "MirrorServer.h"
#include <signal.h>
#include <stdbool.h>

#define XBUFFER_SIZE 25

pthread_mutex_t mtx;                    /* Mutex for synchronization */
pthread_mutex_t mtxBuf[XBUFFER_SIZE];   /*Mutex for every position of Buffer*/
XBUFFER xbuffer[XBUFFER_SIZE];  /* Shared Buffer */
int front;	
int rear;
int buffer_counter;
int block;

int is_regular_file(const char *path);
bool isEmpty();
bool isFulll();

volatile sig_atomic_t done = 0;

void sig_handler (int sig) {
    printf("Received signal %d\n", sig);
    done = 1;
}

void* Manager(void *arg)
{
    dirorfile a = *((dirorfile *) arg);

    int sockfd, portno, n,position;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];
    char temp[20];
	char *pch;

    portno = a.ContentServerPort;

    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
    }

    server = gethostbyname(a.ContentServerAddress);
    if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      return NULL;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Now connect to the server */
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR connecting to the specific server");
      return NULL;
    }

    sprintf(temp, "LIST 999%d %d", a.ContentServerPort, a.delay);
    strcpy(buffer, temp);
    /* Send message to the server */
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0) {
    perror("ERROR writing to socket");
    exit(1);
    }

    /* Now read server response */
    bzero(buffer,256);
    while(1){
        n = read(sockfd, buffer, 255);
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }

        /*Insertion to shared buffer*/
        pch=strstr(buffer, a.dirorfile_name);
        if(pch && buffer_counter<XBUFFER_SIZE)
        {   
            if (isEmpty())
		    { 
                pthread_mutex_lock(&mtx);
			    front =0;
                rear = 0;
                position=rear; 
                pthread_mutex_unlock(&mtx);
            }
            else
            {
                pthread_mutex_lock(&mtx);
                rear = (rear+1)% XBUFFER_SIZE;
                position=rear;
                pthread_mutex_unlock(&mtx);
            }
                pthread_mutex_lock(&mtxBuf[position]);
                printf(" =>> %s, position %d\n", buffer, position);
                xbuffer[position].dirorfilename=malloc(strlen(buffer)+1);
                strcpy(xbuffer[position].dirorfilename, buffer);
                xbuffer[position].ContentServerAddress=malloc(strlen(a.ContentServerAddress)+1);
                strcpy(xbuffer[position].ContentServerAddress, a.ContentServerAddress);
                xbuffer[position].ContentServerPort=a.ContentServerPort; 
                pthread_mutex_unlock(&mtxBuf[position]);

                pthread_mutex_lock(&mtx);
                buffer_counter++;
                //if(buffer_counter==XBUFFER_SIZE) block=1;
                pthread_mutex_unlock(&mtx);
            
        }

        if(strncmp(buffer,"end",3)) printf("%s\n",buffer);
        else { printf("\n"); break; }
    }

    return NULL;
}

void* Worker(void *arg)
{
    XBUFFER a;
    int sockfd, portno, n,position;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];
    char temp[300], temp2[300];
    FILE *received_file;
	int filesize;
	int numbytes,count,bolRecvFile;
	int type;
	char* token;

    //This is the saving directory
    info z = *((info *) arg);

    while(1)
    { 
        if(buffer_counter>0) //If shared buffer is not empty
        {
            if(front == rear) 
		    {
                pthread_mutex_lock(&mtx);
		            //position=front;
					rear = -1;
		            front = -1;
                pthread_mutex_unlock(&mtx);
            }
            else{
                pthread_mutex_lock(&mtx);
		            position = front;
		            front = (front+1) % XBUFFER_SIZE;
                pthread_mutex_unlock(&mtx);
             }   
                pthread_mutex_lock(&mtxBuf[position]);
				    a.dirorfilename = malloc(strlen(xbuffer[position].dirorfilename)+1);
					memset( a.dirorfilename, '\0', strlen(a.dirorfilename) );
				    strcpy(a.dirorfilename, xbuffer[position].dirorfilename);
				    a.ContentServerAddress=malloc(strlen(xbuffer[position].ContentServerAddress)+1);
					memset( a.ContentServerAddress, '\0', strlen(a.ContentServerAddress) );
				    strcpy(a.ContentServerAddress, xbuffer[position].ContentServerAddress);
				    a.ContentServerPort=xbuffer[position].ContentServerPort;
                pthread_mutex_unlock(&mtxBuf[position]);

                pthread_mutex_lock(&mtx);
                	buffer_counter--;
                pthread_mutex_unlock(&mtx);

                portno = a.ContentServerPort;

                // Create a socket point 
                sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                  perror("ERROR opening socket");
                  exit(1);
                }

                server = gethostbyname(a.ContentServerAddress);
                if (server == NULL) {
                  fprintf(stderr,"ERROR, no such host\n");
                  return NULL;
                }

                bzero((char *) &serv_addr, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
                serv_addr.sin_port = htons(portno);

                // Now connect to the server 
                if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
                  perror("ERROR connecting to the specific server");
                  return NULL;
                }

	            sprintf(buffer, "FETCH %s", a.dirorfilename);
	            // Send message to the server 
	            n = write(sockfd, buffer, strlen(buffer));
	            if (n < 0) {
	                perror("ERROR writing to socket");
	                exit(1);
	            }
			

	            token=NULL;
	            strcpy(temp, a.dirorfilename);
	            token=strtok(temp,"/");
	            while(token!=NULL)
	            {
	                strcpy(temp2, token);
	                token=strtok(NULL,"/");
	            }
	            
				numbytes=0,count=0,bolRecvFile=0;
	            sprintf(temp,"%s/%s", z.dirname, temp2);
	            

				read(sockfd, &type, sizeof(type));
	            if (!type)
	            {
					if(strncmp(temp2,".",1) && strncmp(temp2,"..",2)){
	                    printf("Going to create a directory for --> %s\n",temp);
	                    mkdir(temp, 0700);
	                    printf("Just created a directory!\n");
					}
	            }
	            else{

					received_file = fopen(temp, "w+");
	                // Receiving file size 
	                read(sockfd, &filesize, sizeof(filesize));
					while(count < filesize){
						//Receiving From Socket
						numbytes = read(sockfd, buffer, sizeof(buffer));
					 	printf("NUM BYTES REceived %d,Count %d\n",numbytes,count);
					 	printf("I AM FILE %s\n",temp);

						//Counting Bytes Received
						count = count + numbytes;
						if (bolRecvFile == 0)
						{
							printf(":>Receiving file\n");
							bolRecvFile = 1;
						}
						fwrite(buffer, sizeof(buffer), numbytes, received_file);
					 
					}
					 
					if (count >= filesize)
					{
						printf("\n:>File receive Completed(Bytes Received:%d)\n",filesize);
						close(sockfd);
						fclose(received_file);
					}
	            }
                
            
        }
        if(done)
        {
            printf("Thread with id just terminated!\n");
            pthread_exit(NULL);
        }
    }

    return NULL;
}

int main(int argc,char** argv)
{
    int z, MirrorServerPort, threadnum;
    char* dirname=NULL;
    int i = 0;
    int err;
    buffer_counter=0;
    pthread_mutex_init(&mtx, NULL);
    front=-1;
    rear=-1;
    signal(SIGTERM, sig_handler);

    //Getting tokens 
	for(z=0; z<argc; z++){	        
        if(!(strcmp(argv[z],"-m"))){
            dirname=malloc(strlen(argv[z+1])+1);
            strcpy(dirname, argv[z+1]);
        }	
        else if(!(strcmp(argv[z],"-p"))){
            MirrorServerPort=atoi(argv[z+1]);
        }
		else if(!(strcmp(argv[z],"-w"))){
            threadnum=atoi(argv[z+1]);
        }
    }
    
    //Checking if dirname exists else create it
    if(dirname!=NULL)
    {
        struct stat st = {0};
        if (stat(dirname, &st) == -1) {
            mkdir(dirname, 0700);
        }
    }

///////////////////////////////////////
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int  n;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = MirrorServerPort;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR on binding");
      exit(1);
    }
      
    /* Now start listening for the clients, here process will go in sleep mode and will wait for the incoming connection*/
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    /* Accept actual connection from the client */
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0) {
      perror("ERROR on accept");
      exit(1);
    }
///////////////////////////////////////////

    info info;
    info.dirname=malloc(strlen(dirname)+1);
    strcpy(info.dirname, dirname);

    //Creating workers
    pthread_t Workers_id[threadnum];
/*    for(i=0;i<threadnum;i++)*/
/*    {*/
/*        err = pthread_create(&(Workers_id[i]), NULL, &Worker, &info);*/
/*        if (err != 0)*/
/*            printf("can't create thread :[%s]", strerror(err));*/
/*        else*/
/*            printf("Thread created successfully\n");*/
/*    }*/


    dirorfile *ContentServersInfo;
    dirorfile *test;

    int y;
    char *token=NULL;
    int counter=0;
	int x;
    while(1){


        /* If connection is established then start communicating */
        bzero(buffer,256);
        n = read( newsockfd,buffer,255 );
        if (n < 0) {
          perror("ERROR reading from socket");
          exit(1);
        }

        printf("Here is the message: %s\n",buffer);
        

        if(strncmp(buffer,"end",3)){ 
            if(counter==0) ContentServersInfo=malloc(sizeof(dirorfile));
            else 
            {
                test=realloc(ContentServersInfo, sizeof(dirorfile)*(counter+1));
                if (test == NULL) { printf("PROBLEM ON REALLOC\n"); exit(0);}
                else ContentServersInfo=test;
            }

            y=0;
            token = strtok(buffer, ":,");
            while (token) {
                if(y==0){ ContentServersInfo[counter].ContentServerAddress=malloc(strlen(token)+1); strcpy(ContentServersInfo[counter].ContentServerAddress, token);}
                else if(y==1) ContentServersInfo[counter].ContentServerPort=atoi(token);
                else if(y==2){ ContentServersInfo[counter].dirorfile_name=malloc(strlen(token)+1); strcpy(ContentServersInfo[counter].dirorfile_name, token);}
                else if(y==3) ContentServersInfo[counter].delay=atoi(token);
                y++;
                token = strtok(NULL, ":,");
            }

            counter++;
        }
        else //CREATING MANAGERS
        {
            pthread_t Managers_id[counter];
            for(i=0;i<counter;i++)
            {
                err = pthread_create(&(Managers_id[i]), NULL, &Manager, &(ContentServersInfo[i]));
                if (err != 0)
                    printf("can't create thread :[%s]", strerror(err));
                else
                    printf("Thread created successfully\n");
            }
        }

        if(!strncmp(buffer, "exit", 4))
        {
            n = write(newsockfd,"exit",18);
            if (n < 0) {
              perror("ERROR writing to socket");
              exit(1);
            }
            for(i=0;i<counter;i++)
            {
                x=kill(Workers_id[i],SIGTERM);
                if(!x) printf("Sent termination signal to Worker %ld!\n",Workers_id[i]);
            }            
            break;
        }  


        sleep(1);
        for(i=0;i<buffer_counter;i++)
            printf("<%s, %s, %d>\n", xbuffer[i].dirorfilename, xbuffer[i].ContentServerAddress, xbuffer[i].ContentServerPort);

        

        /* Write a response to the client */
        n = write(newsockfd,"I got your message",18);
        if (n < 0) {
          perror("ERROR writing to socket");
          exit(1);
        }
    }

free(token);
free(dirname);
return 0;
}

bool isEmpty()
{
    if(front==-1 && rear==-1) return 1;
    else return 0;
}

bool isFull()
{
    if(((rear+1) % XBUFFER_SIZE) == front) return 1;
    else return 0;
}

int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}
