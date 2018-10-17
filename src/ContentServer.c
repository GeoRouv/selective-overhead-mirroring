#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h> /* limits.h defines "PATH_MAX". */
#include <sys/sendfile.h>

int is_regular_file(const char *path);
static void list_dir (const char * dir_name, int n, int newsock);
void child_server(int newsock, char *dirorfilename, char* selfaddress, int port);
void perror_exit(char *message);

int main(int argc,char** argv)
{
    int z, MirrorServerPort;
    char* dirorfilename;

    //Getting tokens 
    for(z=0; z<argc; z++){	        
        if(!(strcmp(argv[z],"-d"))){
            dirorfilename=malloc(strlen(argv[z+1])+1);
            strcpy(dirorfilename, argv[z+1]);
        }	
        else if(!(strcmp(argv[z],"-p"))){
            MirrorServerPort=atoi(argv[z+1]);
        }
    }


    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;

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

    gethostname(buffer, 256);
    while (1) {
        /* accept connection */
    	newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) { perror("ERROR on accept"); exit(1); }
    	printf("Accepted connection\n");
    	switch (fork()) {    /* Create child for serving client */
    	case -1:     /* Error */
    	    perror("fork"); break;
    	case 0:	     /* Child process */
    	    close(sockfd); 
            child_server(newsockfd, dirorfilename, buffer, portno);
    	    exit(0);
    	}
    	close(newsockfd); /* parent closes socket to client */
    }
    

free(dirorfilename);
return 0;
}

void child_server(int newsock, char *dirorfilename, char* selfaddress, int port) {

    int n, fd, remain_data;
    ssize_t len, sent_bytes, file_size;
    off_t offset;    
    struct stat file_stat;
	struct stat st; 
    char buffer[256], temp[256], *token;
    int delay=0;
	int type;
    
    /* If connection is established then start communicating */
       bzero(buffer,256);
       n = read( newsock,buffer,255 );
       if (n < 0){
          perror("ERROR reading from socket");
          exit(1);
       }
       
       printf("Here is the message: %s\n",buffer);
       if(!strncmp(buffer, "exit", 4)){ 
            printf("Closing connection.\n");
            close(newsock);	  /* Close socket */ return;
        }

       //LIST REQUEST
       if(!strncmp(buffer ,"LIST", 4))
       {
            sprintf(temp, "~LIST Request results for ContentServer:%s and Port:%d~", selfaddress, port);
            n = write(newsock,temp,255);
            if (n < 0) {
               perror("ERROR writing to socket");
               exit(1);
            }            

            list_dir(dirorfilename, n, newsock);

            //Getting delay token from buffer
            strcpy(temp, buffer);
            token = strtok(temp, " ");
            token = strtok(NULL, " ");
            token = strtok(NULL, " ");
            delay=atoi(token);

    
       }   
	   //FETCH REQUEST
       else if(!strncmp(buffer, "FETCH",5))
       {
            strcpy(temp, buffer);
            token = strtok(temp, " ");
            token = strtok(NULL, " ");

            fd = open(token, O_RDONLY);
            if (fd == -1)
            {
                    fprintf(stderr, "Error opening file --> %s", strerror(errno));
                    exit(EXIT_FAILURE);
            }

            // Get file stats 
            if (fstat(fd, &file_stat) < 0)
            {
                    fprintf(stderr, "Error fstat --> %s", strerror(errno));
                    exit(EXIT_FAILURE);
            }

            sleep(delay);

			if(is_regular_file(token))
			{
				if (stat(token, &st) == 0)
    				file_size=st.st_size;

				sprintf(buffer,"%ld",file_size);
		        len = send(newsock, &file_size, sizeof(file_size), 0);
		        if (len < 0)
		        {
		              fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));
		              exit(EXIT_FAILURE);
		        }

		        offset = 0;
		        remain_data = file_size;
				
				while ( ((sent_bytes = read(fd, buffer, sizeof(buffer))) > 0) && (remain_data > 0))
		        {
						write(newsock , buffer , sizeof(buffer));
		                printf("#Server sent %ld bytes from file's data, offset is now : %ld and remaining data = %d\n", sent_bytes, offset, remain_data);
		                remain_data -= sent_bytes;
		        }
			}
			else {type=0; send(newsock, &type, sizeof(type), 0);}

       }                 

       
       /* Write a response to the client */
       n = write(newsock,"end",255);
       if (n < 0) {
          perror("ERROR writing to socket");
          exit(1);
       }

    printf("Closing connection.\n");
    close(newsock);	  /* Close socket */
    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static void list_dir (const char * dir_name, int n, int newsock)
{
    char temp[256];
    DIR * d;
	const char * d_name;
	struct dirent * entry;
	int path_length;
    char path[PATH_MAX];
    
/* Open the directory specified by "dir_name". */
    d = opendir (dir_name);
	
    /* Check it was opened. */
    if (!d) 
    {
        fprintf (stderr, "Cannot open directory '%s': %s\n", dir_name, strerror (errno));
        exit (EXIT_FAILURE);
    }
    while (1){

        /* "Readdir" gets subsequent entries from "d". */
        entry = readdir(d);
        if (!entry) { /* There are no more entries in this directory, so break out of the while loop. */
            break;
        }
        
        d_name = entry->d_name;
        /* Print the name of the file and directory. */
	    printf ("%s/%s\n", dir_name, d_name);

        sprintf(temp, "%s/%s", dir_name, d_name);
        n = write(newsock,temp,255);
        if (n < 0) {
           perror("ERROR writing to socket");
           exit(1);
        }


        if (entry->d_type & DT_DIR) 
        {
            /* Check that the directory is not "d" or d's parent. */
            if (strcmp (d_name, "..") != 0 && strcmp (d_name, ".") != 0) 
            {
 
                path_length = snprintf(path, PATH_MAX, "%s/%s", dir_name, d_name);
                printf ("%s\n", path);
                if(path_length >= PATH_MAX)
                {
                    fprintf (stderr, "Path length has got too long.\n");
                    exit (EXIT_FAILURE);
                }
                /* Recursively call "list_dir" with the new path. */
                list_dir(path, n, newsock);
            }
	    }
    }
    /* After going through all the entries, close the directory. */
    if (closedir(d))
    {
        fprintf (stderr, "Could not close '%s': %s\n", dir_name, strerror (errno));
        exit (EXIT_FAILURE);
    }
}

int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

