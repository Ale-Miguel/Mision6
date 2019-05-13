/*
Alejandro Miguel Sánchez Mora   A01272385
Mariano Hurtado de Mendoza Carranza A00820039


*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <errno.h>
#include <sys/inotify.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>

#define WORKING_DIRECTORY "/home/ale/Desktop"
//Constantes para el inotify
#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))


#define BUFFER_SIZE 256
#define CREATE_FILE "CREATE"
#define DELETE_FILE "DELETE"
#define UPDATE_FILE "UPDATE"

char *sServerIP;
char *sPort;


#define DAEMON_FILE_OUTPUT_NAME "ArchivoDemonioMision6.txt"



void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void fileHandler(char *sFileName, char *sAction, int sockfd);
void sendFile(char *sFileName, int sockfd);
void createFile(char *sFileName, int sockfd);
void executeClient();

static void signalHandler(int sig) {
    switch(sig) {
        case SIGUSR1:
            //log_message(LOG_FILE,"USR1 signal catched");
            executeClient();
            break;
        case SIGUSR2:
            //log_message(LOG_FILE,"USR2 signal catched");
            exit(0);
            break;
    }
}

//Función daemonize que crea el daemo.
//El código de esta función es una mezcla del código de clase, código propio y código obtenido de 
//https://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux
static void daemonize(){
    pid_t pid;

    //Se crea un fork del proceso padre
    pid = fork();

   //En caso que un error suceda
    if (pid < 0)
        exit(EXIT_FAILURE);

    //Si se creó el thread, se muere el padre
    if (pid > 0)
        exit(EXIT_SUCCESS);

    //Si hubo un error
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    //Se implementan todos los signal handlers
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGCHLD,SIG_IGN); /* ignore child */
    signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGUSR1,signalHandler); /* catch hangup signal */
    signal(SIGUSR2,signalHandler); /* catch kill signal */


    //Se obtienen todos los permisos
    umask(0);

    //Se cambia el working directory
    chdir(WORKING_DIRECTORY);

    //Se cierran todos los file descriptors abiertos
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--){
        close (x);
    }

    //Se abre un log para escribir los errores que puedan surgir
    openlog ("Mision6Daemon", LOG_PID, LOG_DAEMON);
}

int main(int argc, char *argv[]){

    if (argc < 3) {
       //fprintf(stderr,"usage %s host_IP port\n", argv[0]);
       exit(0);
    }

    sServerIP = argv[1];
    sPort = argv[2];

    daemonize();

    while(1){
      //Ciclo infinito para evitar que el daemon se muera
    }

    closelog(); //Se cierra el log
    return 0;
}

void executeClient(){
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    FILE * daemonOutputFile;
    char buffer[BUFFER_SIZE];//Buffer donde se almacena la informacion que se recibe y se envía
    //Los argumentos son la dirección IP del servidor y su puerto
    daemonOutputFile = fopen(DAEMON_FILE_OUTPUT_NAME, "w");

    portno = atoi(sPort); //Se obtiene el número de puerto
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   //Se crea el socket del cliente
    if (sockfd < 0) 
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));  //Se inicializa la estructura del socket para el servidor
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(sServerIP);    //Se le dice la IP del servidor
    serv_addr.sin_port = htons(portno); //Se le pone el puerto del servidor

    //Se conecta con el servdor
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    char sFileName[sizeof(buffer)]; //Cadena que guarda el nombre del archivo
    char sAction[BUFFER_SIZE];

    /* #####################inotify##################*/
    int length, i = 0;
    int fd;
    int wd;
    char fileBuffer[BUF_LEN];

    fd = inotify_init();

    if (fd < 0) {
        perror("inotify_init");
    }

    wd = inotify_add_watch(fd, ".",
        IN_MODIFY | IN_CREATE | IN_DELETE);
    length = read(fd, fileBuffer, BUF_LEN);

    if (length < 0) {
        perror("read");
    }
/*#####################inotify################### */
    while(1){
      i = 0;
      length = read(fd, fileBuffer, BUF_LEN);
      while (i < length) {
        struct inotify_event *event =
            (struct inotify_event *) &fileBuffer[i];
        if (event->len) {
            if (event->mask & IN_CREATE) {
                fprintf(daemonOutputFile, "The file %s was created.\n", event->name);
                strcpy(sFileName, event->name);
                strcpy(sAction, CREATE_FILE);
                fileHandler(sFileName, sAction, sockfd);
            } else if (event->mask & IN_DELETE) {
                fprintf(daemonOutputFile,"The file %s was deleted.\n", event->name);
                strcpy(sFileName, event->name);
                strcpy(sAction, DELETE_FILE);
                fileHandler(sFileName, sAction, sockfd);
            } else if (event->mask & IN_MODIFY) {
                fprintf(daemonOutputFile,"The file %s was modified.\n", event->name);
                strcpy(sFileName, event->name);
                strcpy(sAction, UPDATE_FILE);
                fileHandler(sFileName, sAction, sockfd);
            }
        }
        i += EVENT_SIZE + event->len;
      }

    }
   
    fclose(daemonOutputFile);
    close(sockfd);  //Se cierra el socket
    
}

void fileHandler(char *sFileName, char *sAction, int sockfd){
    if(sFileName[0] == '.') return;

    char buffer[BUFFER_SIZE];
    int n;
    bzero(buffer, BUFFER_SIZE);
    strcpy(buffer, sAction);
    n = write(sockfd,buffer,strlen(buffer));    //Se envía la acción
    //strcpy(sAction, buffer);

    //printf("Please enter the file name: "); //Se le pide al usuario el nombre del archivo que va a enviar
    bzero(buffer,BUFFER_SIZE);  //Se limpia el buffer
    //scanf("%s",buffer); //Se lee el nombre de archivo
    strcpy(buffer, sFileName);

    n = write(sockfd,buffer,strlen(buffer));    //Se envía el nombre del archivo

    if (n < 0) 
     error("ERROR sending file name");

    //strcpy(sFileName, buffer);  //Se guarda el nombre del archivo
    bzero(buffer,BUFFER_SIZE);  //Se limpia el buffer

    if(strcmp(CREATE_FILE, sAction) == 0){
        //fprintf(daemonOutputFile,"%s\n", sAction);
        createFile(sFileName, sockfd);
    }else if(strcmp(UPDATE_FILE, sAction) == 0){
        //fprintf(daemonOutputFile,"%s\n", sAction);
        sendFile(sFileName, sockfd);
    }
    //Se recibe el ok del servidor
    n = read(sockfd,buffer,BUFFER_SIZE-1);
    if (n < 0) error("ERROR reading from socket");

    //fprintf(daemonOutputFile,"%s\n",buffer);  //Se imprime el mensaje del servidor
}
void createFile(char *sFileName, int sockfd){
     int n;

    char buffer[BUFFER_SIZE];

    int words = 0;  //Contador de palabras para indicar al servidor cuántas palabras/secciones se van a enviar
    strcpy(buffer, sFileName);  
}
void sendFile(char *sFileName, int sockfd){
    int n;

    char buffer[BUFFER_SIZE];

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
    char segments = words;
    int correcto = 0;
    do{
        bzero(buffer, BUFFER_SIZE);
        buffer[0] = segments;
        n = write(sockfd,buffer, BUFFER_SIZE-1);
        
        n = read(sockfd, buffer, BUFFER_SIZE)-1;
        if(buffer[0] !=  segments){
            
            bzero(buffer, BUFFER_SIZE);
            strcpy(buffer, "NOT OK");

            n = write(sockfd, buffer, BUFFER_SIZE-1);

        }
        else{
            correcto = 1;
        }
    }while(correcto == 0);
    bzero(buffer, BUFFER_SIZE);
    strcpy(buffer, "OK");
    n = write(sockfd, buffer, BUFFER_SIZE-1);
    
    if (words == 0) return;

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

}