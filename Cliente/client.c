/*
Alejandro Miguel Sánchez Mora   A01272385
Mariano Hurtado de Mendoza Carranza A00820039


*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>

#define BUFFER_SIZE 256
void daemonize()
{
    pid_t pid;
    
    pid = fork();

    if(pid < 0)
        exit(EXIT_FAILURE);

    if(pid > 0)
        exit(EXIT_SUCCESS);

    if(setsid() < 0)
        exit(EXIT_FAILURE);
    
    signal(SIGCHLD,SIG_IGN);
    signal(SIGHUP,SIG_IGN);

    pid = fork();

    if(pid < 0)
        exit(EXIT_FAILURE);

    if(pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    chdir("/home/mhurtado/");

    for(int i = sysconf(_SC_OPEN_MAX); i >= 0; i++)
    {
        close(i);
    }

    openlog("mision6", LOG_PID,LOG_DAEMON);
}
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]){
    daemonize();

    
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;

    char buffer[BUFFER_SIZE];//Buffer donde se almacena la informacion que se recibe y se envía
    //Los argumentos son la dirección IP del servidor y su puerto

    while(1)
    {
        if (argc < 3) {
            fprintf(stderr,"usage %s host_IP port\n", argv[0]);
            exit(0);
        }

        portno = atoi(argv[2]); //Se obtiene el número de puerto
        sockfd = socket(AF_INET, SOCK_STREAM, 0);   //Se crea el socket del cliente
        if (sockfd < 0) 
            error("ERROR opening socket");

        bzero((char *) &serv_addr, sizeof(serv_addr));  //Se inicializa la estructura del socket para el servidor
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(argv[1]);    //Se le dice la IP del servidor
        serv_addr.sin_port = htons(portno); //Se le pone el puerto del servidor

        //Se conecta con el servdor
        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
            error("ERROR connecting");

        char sFileName[sizeof(buffer)]; //Cadena que guarda el nombre del archivo

        printf("Please enter the file name: "); //Se le pide al usuario el nombre del archivo que va a enviar
        bzero(buffer,BUFFER_SIZE);  //Se limpia el buffer
        scanf("%s",buffer); //Se lee el nombre de archivo

        n = write(sockfd,buffer,strlen(buffer));    //Se envía el nombre del archivo

        if (n < 0) 
            error("ERROR sending file name");
    
        strcpy(sFileName, buffer);  //Se guarda el nombre del archivo
        bzero(buffer,BUFFER_SIZE);  //Se limpia el buffer

        char sPalabra[BUFFER_SIZE]; //Cadena que guarda las palabras del archivo que se va a enviar

        FILE * archivo; //Apuntador del archivo a enviar

        int words = 0;  //Contador de palabras para indicar al servidor cuántas palabras/secciones se van a enviar

        char ch;    //Variable para almacenar los espacios, EOL o cualquier elemento que fscanf no lee

        archivo = fopen(sFileName, "r");    //Se abre el archivo que se va a enviar

        //Ciclo para leer el archivo y contar los segmentos que se van a enviar
        while((ch = fgetc(archivo)) != EOF){
            fscanf(archivo, "%s", sPalabra);  //Se lee una palabra
            words+=2;   //Se incrementa en 2 el contador (ch + sPalabra)
        }

        //Se limpian las variables
        ch = '\0';

        //Se envía al servidor el conteo de segmentos que se van a enviar
        n = write(sockfd,&words,sizeof(words));

        rewind(archivo);    //Se apunta al inicio del archvio para volver a leer el archivo

        //Ciclo para leer y enviar el contenido del archivo
        while((ch = fgetc(archivo)) != EOF){
        
            bzero(buffer,BUFFER_SIZE);  //Se limpia el buffer
            buffer[0] = ch; //Se le asigna el valor del ch al buffer para que se envíe
            strcpy(sPalabra, buffer);   //Se almacena el contenido del buffer para enviarlo posteriormente

            //Ciclo para asegurar que el servidor recibió correctamente
            //lo que el cliente envió
            do{
                n = write(sockfd, sPalabra, BUFFER_SIZE-1); //Se envía la información
                bzero(buffer,BUFFER_SIZE);  //Se limpia el buffer
                n = read(sockfd, buffer, BUFFER_SIZE-1);    //Se recibe lo que el servidor ha recibido
            }while(strcmp(sPalabra, buffer) != 0);  //Se compara lo que el servidor recibió con lo que se envió
        
            //Se manda un mensaje de "OK" para indicar al servidor 
            //que recibió correctamente la información
            strcpy(buffer, "OK");   
            n = write(sockfd, buffer, BUFFER_SIZE-1);   //Se envía el acknowledgement

            bzero(buffer, BUFFER_SIZE); //Se limpia el buffer

            fscanf(archivo, "%s", buffer);  //Se lee el archivo

            strcpy(sPalabra, buffer);   //Se almacena el contenido del buffer para enviarlo posteriormente
            //Ciclo para asegurar que el servidor recibió correctamente
            //lo que el cliente envió
            do{
                n = write(sockfd, sPalabra, BUFFER_SIZE-1); //Se envía la información
                bzero(buffer,BUFFER_SIZE);  //Se limpia el buffer
                n = read(sockfd, buffer, BUFFER_SIZE-1);
            }while(strcmp(sPalabra, buffer) != 0);  //Se recibe lo que el servidor ha recibido
        
            //Se manda un mensaje de "OK" para indicar al servidor 
            //que recibió correctamente la información
            strcpy(buffer, "OK");
            n = write(sockfd, buffer, BUFFER_SIZE-1);   //Se envía el acknowledgement
            
            if (n < 0) 
                error("ERROR writing to socket");

            //Se limpian las variables
            bzero(buffer, BUFFER_SIZE);
            ch = 0;
            strcpy(sPalabra, "");
    }

        //Se recibe el ok del servidor
        n = read(sockfd,buffer,BUFFER_SIZE-1);
        if (n < 0) error("ERROR reading from socket");

        printf("%s\n",buffer);  //Se imprime el mensaje del servidor
        close(sockfd);  //Se cierra el socket

        }
        
        return 0;
    }
