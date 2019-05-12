/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)   
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");
    bzero(buffer,256);
    n = read(newsockfd,buffer,255);
    if (n < 0) error("ERROR reading from socket");
    printf("Here is the file name: %s, %ld\n",buffer, strlen(buffer));
    char sFileName[sizeof(buffer)];
    strcpy(sFileName, buffer);
    
   

    FILE * archivo;

    archivo = fopen(sFileName, "w");
    int words = 0;
    n = read(newsockfd, &words, sizeof(words));
    //printf("Words: %d\n", words);

    int i = 1;

    for(int i = 1; i <= words; i++){
        n=read(newsockfd, buffer, 255);
        //printf("Recibido:%s, strlen= %ld b[0]=%d, b[1]= %d, b[2]=%d\n", buffer, strlen(buffer), buffer[0], buffer[1], buffer[2]);
        fprintf(archivo, "%s", buffer);
        bzero(buffer, 256);
    }

    printf("File recieved: %s\n", sFileName );

    if (n < 0) error("ERROR reading to socket");

    fclose(archivo);

    n = write(newsockfd,"I got your message",18);
    if (n < 0) error("ERROR writing to socket");

    close(newsockfd);
    close(sockfd);
    return 0; 
}