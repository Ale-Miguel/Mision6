/* 
Alejandro Miguel Sánchez Mora   A01272385  
Mariano Hurtado
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 256 //Constante que guarda el tamaño del buffer

//Función que maneja los errores
void error(const char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){

    int sockfd; //Variable que guarda el socket del servidor
    int newsockfd; //Variable que guarda el socket del cliente
    int portno;    //Variable que almacena el número de puerto

    socklen_t clilen; //tamaño de la dirección del cliente

    char buffer[BUFFER_SIZE];  //Buffer que almaena lo que manda y recibe del cliente

    struct sockaddr_in serv_addr, cli_addr; 

    int n; //Variable  que almacena el código que regresan las operaciones de mandar y recibir

     //El servidor solo pide el número de puerto sobre el que va a escuchar
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
     }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  //Se crea el socket del servidor

    //Si hubo un problema con crear el socket
    if (sockfd < 0){
        error("ERROR opening socket");
    } 

    //Se limpia la dirección del servidor
    bzero((char *) &serv_addr, sizeof(serv_addr));

    portno = atoi(argv[1]);    //Se obtiene el número de puerto
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;    //Se le asigna la dirección IP local
    serv_addr.sin_port = htons(portno);    //Se le asigna el puerto que el usuario indicó

    //Se pide el puerto al SO del socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)   
              error("ERROR on binding");

    //Se escucha por algún cliente
    listen(sockfd,5);

    clilen = sizeof(cli_addr);

    //Se acepta la conexión con el cliente
    newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
    //Si no se pudo conectar con el cliente
    if (newsockfd < 0) 
          error("ERROR on accept");

    //Se limpua el buffer
    bzero(buffer,BUFFER_SIZE);

    n = read(newsockfd,buffer,BUFFER_SIZE-1);   //Se lee el nombre del archivo

    //Si no se pudo recibir el nombre del archivo
    if (n < 0) error("ERROR no se recibio el nombre del archivo");

    printf("Here is the file name: %s\n",buffer);   //Se imprime el nombre del archivo a la consola

    char sFileName[sizeof(buffer)]; //Arreglo oque almacena el nombre del archivo

    strcpy(sFileName, buffer);  //Se guarda el nombre del archivo

    FILE * archivo; //Archivo que se va a crear o modificar

    int words = 0;  //Variable que guarda la cantidad de palabras/segmentos que va a recibir

    archivo = fopen(sFileName, "w");  //Se abre el archivo o se crea y se abre

    n = read(newsockfd, &words, sizeof(words)); //Se obtiene la cantidad de segmentos que se van a recibir

    char sPalabra[BUFFER_SIZE]; //Cadena que guarda el segmento que se recibe 
   
    //Ciclo para leer los segmentos que envía el cliente    
    for(int i = 1; i <= words; i++){

        //Ciclo que se ejecuta hasta recibir el OK del cliente
        //Confirmando que lo que se recibió fue correcto
        do{
            //Se limian las variables antes de usarse
            strcpy(sPalabra, "");
            bzero(buffer, BUFFER_SIZE);

            n=read(newsockfd, buffer, BUFFER_SIZE-1);   //Se recibe la información del cliente

            strcpy(sPalabra, buffer);   //Se guarda la informaicón  que se recibió
            
            n=write(newsockfd, buffer, BUFFER_SIZE-1);  //Se envía lo que se recibió para esperar el acknowledgement

            bzero(buffer, BUFFER_SIZE); //Se limpia el buffer

            n=read(newsockfd, buffer, BUFFER_SIZE-1);   //Se recibe el mensaje del cliente
        }while(strcmp(buffer, "OK") != 0);  //Se espera que se reciba un OK

        fprintf(archivo, "%s", sPalabra);   //Se escribe en el archivo lo que se recibió del cliente

        
    }

    //Se notifica que se recibió el archivo
    printf("File recieved: %s\n", sFileName );

    if (n < 0) error("ERROR reading to socket");

    fclose(archivo);    //Se cierra el archivo

    n = write(newsockfd,"I got your file",18);  //Se envía confirmación al cliente

    if (n < 0) error("ERROR writing to socket");

    //Se cierran los sockets
    close(newsockfd);
    close(sockfd);
    
    return 0; 
}