#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s host_IP port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    /*bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);*/
    //erv_addr.sin_addr.s_addr = inet_addr(argv[1]);
     serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    char sFileName[sizeof(buffer)];
    printf("Please enter the file name: ");
    bzero(buffer,256);
    scanf("%s",buffer);
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
     error("ERROR writing to socket");
    
    strcpy(sFileName, buffer);
    bzero(buffer,256);
    
    char sWords[255];
    FILE * archivo;
    int words = 0;
    char ch;

    archivo = fopen(sFileName, "r");

    while((ch = fgetc(archivo)) != EOF){
        fscanf(archivo, "%s", sWords);
        printf("%s ch=%d\n",sWords, ch );

        //if(ch == '\t' || ch == '\n' || ch == 32) //32 es el valor en ASCII para el espacio
            words+=2;
    }
    //words *= 2;
    ch = '\0';
    n = write(sockfd,&words,sizeof(words));

    rewind(archivo);
    char sPalabra[sizeof(buffer)];
    while((ch = fgetc(archivo)) != EOF){
        
        //printf("%s ch=%d\n",sWords, ch );
        //strcpy(sPalabra, &ch);
        //strcat(sPalabra, sWords);
        bzero(buffer,256);
        buffer[0] = ch;
        n = write(sockfd, buffer, 255);
        bzero(buffer,256);
        fscanf(archivo, "%s", buffer);
        n = write(sockfd, buffer, 255);
        //if(!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')))
            

        if (n < 0) 
            error("ERROR writing to socket");

        //strcpy(buffer, "");
        bzero(buffer, 256);
        ch = 0;
        
    }

  /*  while( fscanf(archivo, "%s", sWords)!= EOF){
        
        //printf("%s ch=%d\n",sWords, ch );

        n = write(sockfd, sWords, 255);

        if (n < 0) 
            error("ERROR writing to socket");

        strcpy(sWords, "");
        
    }*/


    strcpy(sPalabra, "$EOF$");
    //n = write(sockfd, sPalabra, strlen(sPalabra));
    n = read(sockfd,buffer,255);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",buffer);
    close(sockfd);
    return 0;
}
