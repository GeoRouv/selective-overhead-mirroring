#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc,char** argv)
{
    int z, MirrorServerPort;
    char* MirrorServerAddress, *ContentServerStrings;

    //Getting tokens 
    for(z=0; z<argc; z++){	        
        if(!(strcmp(argv[z],"-n"))){
            MirrorServerAddress=malloc(strlen(argv[z+1])+1);
            strcpy(MirrorServerAddress, argv[z+1]);
        }	
        else if(!(strcmp(argv[z],"-p"))){
            MirrorServerPort=atoi(argv[z+1]);
        }
	    else if(!(strcmp(argv[z],"-s"))){
            ContentServerStrings=malloc(strlen(argv[z+1])+1);
		    strcpy(ContentServerStrings, argv[z+1]);
        }
    }

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    portno = MirrorServerPort;

    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
    }

    server = gethostbyname(MirrorServerAddress);
    if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Now connect to the server */
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR connecting");
      exit(1);
    }

    //Sending the lists of remote servers
    char *token=NULL;
    for (token = strtok(ContentServerStrings, ","); token; token = strtok(NULL, ","))
    {
        strcpy(buffer, token);
        /* Send message to the server */
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0) {
          perror("ERROR writing to socket");
          exit(1);
        }

        /* Now read server response */
        bzero(buffer,256);
        n = read(sockfd, buffer, 255);
        if (n < 0) {
          perror("ERROR reading from socket");
          exit(1);
        }
    
         printf("%s\n",buffer);
    }

    strcpy(buffer, "end");
    /* Send message to the server */
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0) {
    perror("ERROR writing to socket");
    exit(1);
    }

    /* Now read server response */
    bzero(buffer,256);
    n = read(sockfd, buffer, 255);
    if (n < 0) {
    perror("ERROR reading from socket");
    exit(1);
    }

    //Entering CLI mode
    while(1){
       /* Now ask for a message from the user, this message will be read by server*/
       printf("Please enter the message: ");
       bzero(buffer,256);
       fgets(buffer,255,stdin);
       
       /* Send message to the server */
       n = write(sockfd, buffer, strlen(buffer));
       if (n < 0) {
          perror("ERROR writing to socket");
          exit(1);
       }

        if(!strncmp(buffer, "exit", 4)) break;       

       /* Now read server response */
       bzero(buffer,256);
       n = read(sockfd, buffer, 255);
       if (n < 0) {
          perror("ERROR reading from socket");
          exit(1);
       }

       printf("%s\n",buffer);
    }

free(token);
free(MirrorServerAddress);
free(ContentServerStrings);
return 0;
}
