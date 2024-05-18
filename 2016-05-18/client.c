#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <unistd.h>

int ariketa1();
int ariketa2();

#define SERVER_PORT 3001
#define DESTIP "127.0.0.1"

#define CLAVE 0x45994891L	// Clave de los recursos. Sustituir por DNI.
#define TIME 4			// Temporizador de retransmisión
#define BUFLEN 256	// Tamaño de bufferes genérico

#define SIZE_SHM 1024	//Tamaño del segmento de memoria compartida

#define MAX_DEV	4	//Maximo # dispositivos en la tabla de sesiones


/* El array de semáforos a crear tiene tamaño 4
	El semáforo 0 servirá para controlar el acceso a lecturas de valores de
		los semáforos 1 y 2
	El semáforo 1 para poder modificar su valor en comando SEM
	El semáforo 2 para poder modificar su valor en comando SEM
	el semáforo 3 para controlar los accesos a la tabla de sesiones en 
		fase de registro.
*/
#define MAX_SEM 4	// Número de semáforos en el array

/* Lista de tipos de mensaje recibidos en la cola de mensajes. */

#define COMM_SPEED '2'
#define COMM_RPM '3'
#define COMM_SEM '4'
#define COMM_BYE '5'

#define MAX_COMMAND 6

char *udp_cmd[]={
	"HELLO", "PORT","SPEED","RPM","SEM","BYE",""
};

#define OFF_DATA_TBL 0	// Desplazamiento de la tabla de sesiones en SHM

#define ST_FREE 0
#define ST_PID 1
#define ST_DATA 2
#define LEN_NAME 16	//Usar como maximo nombres de 8

struct msgbuf {
  long mtype;         /* message type, must be > 0 */
  char mtext[255];    /* message data */
};


struct st_data {
	int state;	// State of register
	char name[LEN_NAME];	// Name of device
	int speed;	// velocidad 
	int rpm;	// revoluciones por minuto
	int port;	// Original port 
	int sem;	// Sem number 
	int semval;	// Sem value 
	pid_t pid;	// Process identifier
};

union semun {
   int              val;    /* Value for SETVAL */
   struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
   unsigned short  *array;  /* Array for GETALL, SETALL */
   struct seminfo  *__buf;  /* Buffer for IPC_INFO
                               (Linux-specific) */
};


/*************************************************************************/
/* Función a utilizar para sustituir a signal() de la libreria.
Esta función permite programar la recepción de la señál de temporización de
alarm() para que pueda interrumpir una funcion bloqueante.
El alumno debe saber como utilizarla.
*/
int signal_EINTR(int sig,void(*handler)())
{
struct sigaction sa;
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	return(sigaction(sig,&sa,NULL));
}

/*
  if(()<0){
    perror("Errorea:\n");
    exit(-1);
  }
*/

/*************************************************************************/


int main(int argc, char *argv[])
{
  printf("Aukeratu ariketa:\n");
  char aukera = getchar();
  switch(aukera){
    case('1'):
      if((ariketa1())<0){
        perror("1 ariketan errorea:\n");
        exit(-1);
      }
      return(0);
    case('2'):
      if((ariketa2())<0){
        perror("2 ariketan errorea:\n");
        exit(-1);
      }
      printf("\n---------------------------\n2. Ariketa amaituta!\n");
      //__fpurge(stdin);
      getchar();
      return(0);
  }
}

//#================================================================ARIEKTA2================================================================#

int ariketa2(){

  //MEMORIA PARTEKATUA hasieratu
  int shmid;
  char* shmptr;
  if((shmid = shmget(CLAVE, SIZE_SHM, 0600|IPC_CREAT))<0){
    perror("shmget Errorea:\n");
    exit(-1);
  }
  printf("Shared Memory ID: %d\n", shmid);
  if((shmptr = (uint8_t*)shmat(shmid, NULL, 0600|IPC_CREAT))<0){
    perror("shmat Errorea:\n");
    exit(-1);
  }

  int msgQID;
  struct msgbuf msgbuffer;
  //MEZU ILARA hasieratu
  if((msgQID = msgget(CLAVE, 0600|IPC_CREAT))<0){
    perror("msgget Errorea:\n");
    exit(-1);
  }
  printf("Message Queue ID: %d\n", msgQID);

  //SEMAFOROAK hasieratu
  int semid = 0;
  if((semid = semget(CLAVE, MAX_SEM, 0600|IPC_CREAT))<0){
    perror("semget Errorea:\n");
    exit(-1);
  }
  printf("Semaphore Array ID: %d\n", msgQID);

  //UDP hasieratu
  int udp_socket;
  struct sockaddr_in params, servParams;
  char buffer[255];
  
  params.sin_family = AF_INET;
  params.sin_port = htons(SERVER_PORT);
  params.sin_addr.s_addr = inet_addr(DESTIP);

  if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("socket Errorea:\n");
    exit(-1);
  }
  
  printf("Socket-a sortua!\n");
  int paramSize = sizeof(params);
  
  for(int i = 0; i<2; i++){
    //HELLO
    char devName[5];
    sprintf(devName, "DEV%1d", i);
    printf("devName: %s\n", devName);
    sprintf(buffer, "%s %d %s", udp_cmd[0], getpid(), devName);

    printf("BIDALIKO DA:\n%s\n", buffer);
    if((sendto(udp_socket, buffer, sizeof(buffer), 0, &params, paramSize))<0){
      perror("sendto Errorea:\n");
      exit(-1);
    }
    
    if((recvfrom(udp_socket, buffer, sizeof(buffer), 0, &params, &paramSize))<0){
      perror("recvfrom Errorea:\n");
      exit(-1);
    }
    
    if((getsockname(udp_socket, &servParams, &paramSize))<0){
      perror("Errorea:\n");
      exit(-1);
    }
    printf("Received: %s\n", buffer);
    int portNumber = ntohs(servParams.sin_port);
    printf("Portua: %d\n", portNumber);
    
    //PORT
    sprintf(buffer, "%s %d", udp_cmd[1], portNumber);
    printf("BIDALIKO DA:\n%s\n", buffer);
    if((sendto(udp_socket, buffer, sizeof(buffer), 0, &params, paramSize))<0){
      perror("sendto Errorea:\n");
      exit(-1);
    }
    
    if((recvfrom(udp_socket, buffer, sizeof(buffer), 0, &params, &paramSize))<0){
      perror("recvfrom Errorea:\n");
      exit(-1);
    }
    printf("Received: %s\n", buffer);
    
    //MEZU ILARAKO AGINDUA HARTZEN ETA INTERPRETATZEN
    msgrcv(msgQID, &msgbuffer, sizeof(msgbuffer), getpid(), 0);
    printf("MEZU ILARATIK JASOTAKOA:\n%s\n", msgbuffer.mtext);
    
    if((char)msgbuffer.mtext[0] == '5'){
      //BYE
      sprintf(buffer, "%s", udp_cmd[5]);
      printf("BIDALIKO DA:\n%s\n", buffer);
      if((sendto(udp_socket, buffer, sizeof(buffer), 0, &params, paramSize))<0){
        perror("sendto Errorea:\n");
        exit(-1);
      }
      return(0);
    }
    
    char *token;
    token = strtok(msgbuffer.mtext, "<");
    printf("Token: %s\n", token);
    int aukera = atoi(token);
    token = strtok(NULL, ">");
    printf("Token: %s\n", token);
    int balioa = atoi(token);
    
    //MEMORIAKOA EGUNERATU
    int enabled = 1;
    //memcpy(shmptr, &enabled, sizeof(int));
    //*((int*)shmptr+20) = balioa;
    //strcpy((char*)(shmptr + 4), devName);
    //(int)*(shmptr+20) = balioa;
    //memcpy((shmptr+20), &balioa, sizeof(int));
    
    for(int k = 0; k<MAX_DEV; k++){
      printf("====SHMEM====\n");
      printf("State: \t%d\n", *((int*)shmptr + k*sizeof(struct st_data)));
      printf("Name:  \t%s\n", shmptr + 4);
      printf("Speed: \t%d\n", *((int*)shmptr + 20 + k*sizeof(struct st_data)));
      *((int*)shmptr + 20 + k*sizeof(struct st_data)) = balioa;
      printf("RPM:   \t%d\n", *((int*)shmptr + 24 + k*sizeof(struct st_data)));
      printf("PORT:  \t%d\n", *((int*)shmptr + 28 + k*sizeof(struct st_data)));
      printf("SEM:   \t%d\n", *((int*)shmptr + 32 + k*sizeof(struct st_data)));
      printf("SEMVAL:\t%d\n", *((int*)shmptr + 36 + k*sizeof(struct st_data)));
      printf("pID:   \t%d\n", *((int*)shmptr + 40 + k*sizeof(struct st_data)));
      printf("====SHMEM====\n");
    }
    
    for(int k = 0; k<MAX_DEV; k++){
      printf("====SHMEM====\n");
      printf("State: \t%d\n", *((int*)shmptr + k*sizeof(struct st_data)));
      printf("Name:  \t%s\n", shmptr + 4);
      printf("Speed: \t%d\n", *((int*)shmptr + 20 + k*sizeof(struct st_data)));
      printf("RPM:   \t%d\n", *((int*)shmptr + 24 + k*sizeof(struct st_data)));
      printf("PORT:  \t%d\n", *((int*)shmptr + 28 + k*sizeof(struct st_data)));
      printf("SEM:   \t%d\n", *((int*)shmptr + 32 + k*sizeof(struct st_data)));
      printf("SEMVAL:\t%d\n", *((int*)shmptr + 36 + k*sizeof(struct st_data)));
      printf("pID:   \t%d\n", *((int*)shmptr + 40 + k*sizeof(struct st_data)));
      printf("====SHMEM====\n");
    }    
    
    //SPEED
    sprintf(buffer, "%s %d", udp_cmd[aukera], balioa);
    printf("BIDALIKO DA:\n%s\n", buffer);
    if((sendto(udp_socket, buffer, sizeof(buffer), 0, &params, paramSize))<0){
      perror("sendto Errorea:\n");
      exit(-1);
    }
    
    //MEZU ILARAKO AGINDUA HARTZEN ETA INTERPRETATZEN
    msgrcv(msgQID, &msgbuffer, sizeof(msgbuffer), getpid(), 0);
    printf("MEZU ILARATIK JASOTAKOA:\n%s\n", msgbuffer.mtext);
    
    aukera = (int)(msgbuffer.mtext[0] - 48);
    printf("Aukera: %d\n", aukera);
    balioa = (int)(((uint8_t)msgbuffer.mtext[4]<<24) + ((uint8_t)msgbuffer.mtext[3]<<16) + ((uint8_t)msgbuffer.mtext[2]<<8) + ((uint8_t)msgbuffer.mtext[1]));
    printf("AUKERA ETA BALIOA PREST!\n");
    
    //MEMORIAKOA EGUNERATU
    
    //RPM
    sprintf(buffer, "%s %d", udp_cmd[aukera], balioa);
    printf("BIDALIKO DA:\n%s\n", buffer);
    if((sendto(udp_socket, buffer, sizeof(buffer), 0, &params, paramSize))<0){
      perror("sendto Errorea:\n");
      exit(-1);
    }
    
    //MEZU ILARAKO AGINDUA HARTZEN ETA INTERPRETATZEN
    msgrcv(msgQID, &msgbuffer, sizeof(msgbuffer), getpid(), 0);
    printf("MEZU ILARATIK JASOTAKOA:\n%s\n", msgbuffer.mtext);
    
    aukera = (int)(msgbuffer.mtext[0] - 48);
    printf("Aukera: %d\n", aukera);
    balioa = (int)(((uint8_t)msgbuffer.mtext[4]<<24) + ((uint8_t)msgbuffer.mtext[3]<<16) + ((uint8_t)msgbuffer.mtext[2]<<8) + ((uint8_t)msgbuffer.mtext[1]));
    printf("AUKERA ETA BALIOA PREST!\n");
    
    //MEMORIAKOA EGUNERATU
    //ARRAY-KO LEHEN STRUCT-A!
    /*
    printf("State:%d\n", (int)*shmptr);
    shmptr = shmptr + 4;
    printf("State:%s\n", (char*)shmptr);
    shmptr = shmptr + 16;
    printf("RPM:%d\n", (int)*shmptr);
    shmptr = shmptr + 4;
    printf("PORT:%d\n", (int)*shmptr);
    shmptr = shmptr + 4;
    printf("SEM:%d\n", (int)*shmptr);
    shmptr = shmptr + 4;
    printf("SEMVAL:%d\n", (int)*shmptr);
    shmptr = shmptr + 4;
    printf("PID:%d\n", (int)*shmptr);
    shmptr = shmptr + 4;
    */
    
    //SEM
    //Semaforoaren balioa konprobatu behar dugu
    int semval;
    semval = semctl(semid, balioa, GETVAL);
    
    sprintf(buffer, "%s %d %d", udp_cmd[aukera], balioa, semval);
    printf("BIDALIKO DA:\n%s\n", buffer);
    if((sendto(udp_socket, buffer, sizeof(buffer), 0, &params, paramSize))<0){
      perror("sendto Errorea:\n");
      exit(-1);
    }
    
    //BYE
    sprintf(buffer, "%s", udp_cmd[5]);
    printf("BIDALIKO DA:\n%s\n", buffer);
    if((sendto(udp_socket, buffer, sizeof(buffer), 0, &params, paramSize))<0){
      perror("sendto Errorea:\n");
      exit(-1);
    }
    
  }
  printf("For-etik irtenda!\n");

  return(0);
}


//#================================================================ARIEKTA1================================================================#
int ariketa1(){

  int udp_socket;
  struct sockaddr_in params, servParams;
  char buffer[255];
  
  params.sin_family = AF_INET;
  params.sin_port = htons(SERVER_PORT);
  params.sin_addr.s_addr = inet_addr(DESTIP);

  if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("socket Errorea:\n");
    exit(-1);
  }
  
  printf("Socket-a sortua!\n");
  int paramSize = sizeof(params);

  sprintf(buffer, "%s %d %s", udp_cmd[0], getpid(), "DEV-1");
  printf("BIDALIKO DA:\n%s\n", buffer);
  if((sendto(udp_socket, buffer, sizeof(buffer), 0, &params, paramSize))<0){
    perror("sendto Errorea:\n");
    exit(-1);
  }
  
  if((recvfrom(udp_socket, buffer, sizeof(buffer), 0, &params, &paramSize))<0){
    perror("recvfrom Errorea:\n");
    exit(-1);
  }
  
  if((getsockname(udp_socket, &servParams, &paramSize))<0){
    perror("Errorea:\n");
    exit(-1);
  }
  printf("Received: %s\n", buffer);
  int portNumber = ntohs(servParams.sin_port);
  printf("Portua: %d\n", portNumber);
  
  //PORT
  sprintf(buffer, "%s %d", udp_cmd[1], portNumber);
  printf("BIDALIKO DA:\n%s\n", buffer);
  if((sendto(udp_socket, buffer, sizeof(buffer), 0, &params, paramSize))<0){
    perror("sendto Errorea:\n");
    exit(-1);
  }
  
  //BYE
  sprintf(buffer, "%s", udp_cmd[5]);
  printf("BIDALIKO DA:\n%s\n", buffer);
  if((sendto(udp_socket, buffer, sizeof(buffer), 0, &params, paramSize))<0){
    perror("sendto Errorea:\n");
    exit(-1);
  }
  
  exit(0);

  return(0);
}

