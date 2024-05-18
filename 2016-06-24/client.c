#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/signal.h>
#include <unistd.h>
#include <stdint.h>

//HONEKIN 9 GAKO LOR DAITEZKE
//8 eta 12 gakoak lortzeko honelako 2 paraleloan exekutatu behar dira, bakoitzak DEV-X izen ezberdin batekin!

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

#define CNL_SRV 0x1L

#define OFF_REG_TBL 0	// Desplazamiento de la tabla de sesiones en SHM

#define ST_FREE 0
#define ST_PID 1
#define ST_DATA 2
#define LEN_NAME 16	//Usar como maximo nombres de 8

#define LEN_FIFO_NAME 32
#define ROOT_NAME_FIFO "/tmp/fifo-"

struct st_reg {
	int state;	// State of register
	char name[LEN_NAME];	// Name of device
	pid_t pid;	// Process identifier
};

#define OFF_DATA_TBL 300	// Desplazamiento de la tabla de datos en SHM

struct st_data {
	float speed;	// Speed parameter as float value 
	int rpm;	// Speed parameter as float value 
	int sem;	// Sem number 
	int semval;	// Sem value 
	
};

//BEHARBADA SEMAFOROENTZAT BEHARKO DUGU!
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

struct msgbuf{
  long mtype;
  int pid_client;
  char mtext[128];
};

  /*
  if(()<0){
    perror("Errorea:\n");
    exit(-1);
  }
  */

/*************************************************************************/

int ariketa2();
int ariketa1();


int main(int argc, char *argv[])
{  

  //printf("Sizeof Float: %ld\n", sizeof(float));
  //Float: 4 Byte
  //Int: 4 Byte
  
  char aukera = 0;
  printf("Aukeratu ariketa!:\n");
  __fpurge(stdin);
  aukera = getchar();
  printf("Aukera: %c", aukera);

  switch(aukera){
    case '1':
      printf("1. Ariketa hasten!\n");
      ariketa1();
      break;
    case '2':
      printf("2. Ariketa hasten!\n");
      ariketa2();
      break;
    default:
      exit(-1);
  }
  
  return(0);
}

//#=============================================== 2. ARIKETA ======================================================#
int ariketa2(){
  
  int semid = 0;
  if((semid = semget(CLAVE, MAX_SEM, 0600|IPC_CREAT))<0){
    perror("Semget Errorea:\n");
    exit(-1);
  }
  
  char pathname[128];
  sprintf(pathname, "/tmp/fifo-%d", getpid());
  mkfifo(pathname, 0600|IPC_CREAT);
  
  printf("Fifoa sortuta!\n");
  
  int fifofd;
  fifofd = open(pathname, O_RDWR);
  
  printf("Fifoa irekita!\n");
  
  struct msgbuf messageBuffer, receivedMessageBuffer;

  char buffer[128];
  int msgqid = 0;
  
  if((msgqid = msgget(CLAVE, 0600|IPC_CREAT))<0){
    perror("Msgget errorea:\n");
    exit(-1);
  }
  
  //PREPARING HELLO
  messageBuffer.mtype = CNL_SRV;
  messageBuffer.pid_client = getpid();
  sprintf(messageBuffer.mtext, "HELLO %d %s", getpid(), "DEV-1");
  printf("Sending: %s\n", messageBuffer.mtext);
  
  //SEND
  if((msgsnd(msgqid, &messageBuffer, sizeof(messageBuffer), 0))<0){
    perror("Msgsend Errorea:\n");
    exit(-1);
  }
  
  //AWAIT OK
  if((msgrcv(msgqid, &receivedMessageBuffer, sizeof(receivedMessageBuffer), getpid(), 0))<0){
    perror("Msgrcv Errorea:\n");
    exit(-1);
  }
  printf("PID: %d\n", getpid());
  //printf("%c, %c, %c\n", 67, 46, 39);
  printf("Received: %ld %d %s\n",receivedMessageBuffer.mtype, receivedMessageBuffer.pid_client, receivedMessageBuffer.mtext);
  
  char cmd = 0;
  char bigBuf[128];
  float speed = 0.0F;
  int rpm = 0;
  int sem = 0;
  while(cmd != '5'){
  
    if((read(fifofd, &bigBuf, sizeof(bigBuf)))<0){
      perror("Errorea bigBuf irakurtzen:\n");
      exit(-1);
    }
    printf("Jasotako bigBuf: %s\n", bigBuf);
    cmd = bigBuf[0];
    
    switch(cmd){
      /*********************/
      case '2':
        printf("Jasotako mezu interpretatua: %c - %f\n", cmd, speed);
        
        memcpy(&speed, (char*)(bigBuf + 1), sizeof(speed));
        
        sprintf(messageBuffer.mtext, "SPEED %f", speed);
        printf("Sending: %s\n", messageBuffer.mtext);
        break;
        
      /*********************/
      case '3':
        
        char *token;
        token = strtok(bigBuf, "<");
        printf("Token: %s\n", token);
        token = strtok(NULL, ">");
        rpm = atoi(token);
        printf("Token: %s\n", token);
        
        
        sprintf(messageBuffer.mtext, "RPM %d", rpm);
        printf("Sending: %s\n", messageBuffer.mtext);
        break;
        
      /*********************/
      case '4':
        //sem = (int)(((uint8_t)bigBuf[4]<<24)+((uint8_t)bigBuf[3]<<16)+((uint8_t)bigBuf[2]<<8)+((uint8_t)bigBuf[1]));
        memcpy(&sem, (char*)(bigBuf + 1), sizeof(sem));
        printf("Jasotako mezu interpretatua: %c - %d\n", cmd, sem);
        
        int semval = 0;
        if((semval = semctl(semid, sem, GETVAL))<0){
          perror("GETVAL Errorea:\n");
          exit(-1);
        }
        
        sprintf(messageBuffer.mtext, "SEM %d %d", sem, semval);
        printf("Sending: %s\n", messageBuffer.mtext);
        break;
        
      default:
        sprintf(messageBuffer.mtext, "BYE");
        printf("Sending: %s\n", messageBuffer.mtext);
        break;
    }
    
  //SEND
  if((msgsnd(msgqid, &messageBuffer, sizeof(messageBuffer), 0))<0){
  perror("Msgsend Errorea:\n");
  exit(-1);
  }
    
  }


  
  //getchar();
  //SEND
  /*if((msgsnd(msgqid, &messageBuffer, sizeof(messageBuffer), 0))<0){
    perror("Msgsend Errorea:\n");
    exit(-1);
  }
*/

  return(0);
}

















//#=============================================== 1. ARIKETA ======================================================#

int ariketa1(){
  char pathname[128];
  sprintf(pathname, "/tmp/fifo-%d", getpid());
  mkfifo(pathname, 0600|IPC_CREAT);
  
  printf("Fifoa sortuta!\n");
  
  open(pathname, O_RDWR);
  
  printf("Fifoa irekita!\n");
  
  struct msgbuf messageBuffer, receivedMessageBuffer;

  char buffer[128];
  int msgqid = 0;
  
  if((msgqid = msgget(CLAVE, 0600|IPC_CREAT))<0){
    perror("Msgget errorea:\n");
    exit(-1);
  }
  
  //PREPARING HELLO
  messageBuffer.mtype = CNL_SRV;
  messageBuffer.pid_client = getpid();
  sprintf(messageBuffer.mtext, "HELLO %d %s", getpid(), "DEV-1");
  printf("Sending: %s\n", messageBuffer.mtext);
  
  //SEND
  if((msgsnd(msgqid, &messageBuffer, sizeof(messageBuffer), 0))<0){
    perror("Msgsend Errorea:\n");
    exit(-1);
  }
  
  //AWAIT OK
  if((msgrcv(msgqid, &receivedMessageBuffer, sizeof(receivedMessageBuffer), getpid(), 0))<0){
    perror("Msgrcv Errorea:\n");
    exit(-1);
  }
  printf("PID: %d\n", getpid());
  //printf("%c, %c, %c\n", 67, 46, 39);
  printf("Received: %ld %d %s\n",receivedMessageBuffer.mtype, receivedMessageBuffer.pid_client, receivedMessageBuffer.mtext);


  sprintf(messageBuffer.mtext, "BYE");
  printf("Sending: %s\n", messageBuffer.mtext);
  
  //getchar();
  //SEND
  if((msgsnd(msgqid, &messageBuffer, sizeof(messageBuffer), 0))<0){
    perror("Msgsend Errorea:\n");
    exit(-1);
  }
  return(0);
}
