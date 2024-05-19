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

#define SERVER_PORT 3001
#define SERVER_IP "127.0.0.1"

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

struct msgbuf {
  long mtype;         /* message type, must be > 0 */
  char mtext[16];     /* message data */
};


//semctl-rentzat
 union semun {
     int              val;    /* Value for SETVAL */
     struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
     unsigned short  *array;  /* Array for GETALL, SETALL */
     struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                 (Linux-specific) */
 };


/*
if(()<0){
  perror("Errorea:\n");
  exit(-1);
}
*/

int ariketa1();
int ariketa2();


int main(int argc, char *argv[])
{
  char aukera;
  printf("Nahi duzun ariketa sartu:\n");
  aukera = getchar();
  
  switch(aukera){
    case '1':
      ariketa1();
      break;
    case '2':
      ariketa2();
      break;
    default:
      exit(-1);
  }
}


//#========================== ARIKETA 2 ==========================#//
int ariketa2(){
  
  int msgqid;
  struct msgbuf messageBuffer;
  
  if((msgqid = msgget(CLAVE, 0600|IPC_CREAT))<0){
    perror("Mezu ilara lortzean Errorea:\n");
    exit(-1);
  }
  
  /**********************************/
  //MEZU ILARA PREST
  
  int shmid;
  char* shmptr;
  
  struct st_data shmstruct;
  
  if((shmid = shmget(CLAVE, SIZE_SHM, 0600|IPC_CREAT))<0){
    perror("Memoria partekatua lortzean Errorea:\n");
    exit(-1);
  }
  
  if((shmptr = shmat(shmid, 0, 0))<0){
    perror("Memoria partekatura attach egitean Errorea:\n");
    exit(-1);
  }
  /**********************************/
  //MEMORIA PARTEKATUA PREST
  
  int semid;
  
  if((semid = semget(CLAVE, MAX_SEM, 0600|IPC_CREAT))<0){
    perror("Semaforoa lortzen Errorea:\n");
    exit(-1);
  }
  
  /**********************************/
  //SEMAFORO ARRAY-A PREST

  char bufferIn[255];
  char bufferOut[255];
  struct sockaddr_in params;
  struct sockaddr_in rxParams;
  int udp_socket;
  
  if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("Socket errorea:\n");
    exit(-1);
  }
  
  //HELLO
  sprintf(bufferOut, "HELLO %d DEV-%4d", getpid(), getpid);
  
  params.sin_family = AF_INET;
  params.sin_port = htons(SERVER_PORT);
  params.sin_addr.s_addr = inet_addr(SERVER_IP);
  
  int sizeofParams = sizeof(params);
  /**********************************/
  //UDP PREST
  
  
  if((sendto(udp_socket, bufferOut, strlen(bufferOut), 0, &params, sizeofParams))<0){
    perror("Bidaltzean Errorea:\n");
    exit(-1);
  }
  
  //OK jaso
  if((recvfrom(udp_socket, bufferIn, sizeof(bufferIn), 0, &rxParams, &sizeofParams))<0){
    perror("Irakurtzean Errorea:\n");
    exit(-1);
  }
  printf("Erantzuna:\t%s\n", bufferIn);
  
  //PORT
  if((getsockname(udp_socket, &rxParams, &sizeofParams))<0){
    perror("GetSockName Errorea:\n");
    exit(-1);
  }
  
  printf("Jatorriko portua: %d\n", ntohs(rxParams.sin_port));
  
  sprintf(bufferOut, "PORT %d", ntohs(rxParams.sin_port));
  
  if((sendto(udp_socket, bufferOut, strlen(bufferOut), 0, &params, sizeofParams))<0){
    perror("Bidaltzean Errorea:\n");
    exit(-1);
  }
  
  //OK jaso
  if((recvfrom(udp_socket, bufferIn, sizeof(bufferIn), 0, &rxParams, &sizeofParams))<0){
    perror("Irakurtzean Errorea:\n");
    exit(-1);
  }
  printf("Erantzuna:\t%s\n", bufferIn);
  
  int aurkituta = 0;
  int iterazio = 0;
  printf("Shared memory-ko tokatu zaigun atala bilatzen!\n");
  while(aurkituta == 0){
    memcpy(&shmstruct, (struct st_data*)(shmptr + iterazio*sizeof(shmstruct)),sizeof(shmstruct));
    printf("%d - %s\n", iterazio, shmstruct.name);
    
    
    if(strlen(shmstruct.name)>2){
      printf("Aurkituta!\n");
      aurkituta = iterazio;
    }
    
    iterazio++;
    
    if(iterazio > 3 && aurkituta == 0){
      aurkituta = -1;
    }
    
  }
  
  printf("Memoria partekatutako %d posizioan (%d) idatziko dugu struct-a!\n", aurkituta, (struct st_data*)(shmptr + aurkituta*sizeof(shmstruct)));
  
  //Struct-a memoriatik kopiatu
  semDown(semid);
  memcpy(&shmstruct, (struct st_data*)(shmptr + aurkituta*sizeof(shmstruct)),sizeof(shmstruct));
  //Balioa eraldatu
  shmstruct.pid = getpid();
  shmstruct.state = 1;
  shmstruct.port = ntohs(rxParams.sin_port);
  //Struct-a memoriala itsatsi
  semUp(semid);
  memcpy((struct st_data*)(shmptr + aurkituta*sizeof(shmstruct)), &shmstruct, sizeof(shmstruct));
  
  while(1){
    
    if((msgrcv(msgqid, &messageBuffer, sizeof(messageBuffer), getpid(), 0))<0){
      perror("Errorea:\n");
      exit(-1);
    }

    printf("Mezu ilaratik jasotakoa:%s\n", messageBuffer.mtext);
    char cmd = messageBuffer.mtext[0];
    switch (cmd){
      case '2':
        //SPEED
        char* token;
        
        semDown(semid);
        
        token = strtok(messageBuffer.mtext, "<");
        printf("1: %s\n", token);
        token = strtok(NULL, ">");
        printf("2: %s\n", token);
        
        //Struct-a memoriatik kopiatu
        memcpy(&shmstruct, (struct st_data*)(shmptr + aurkituta*sizeof(shmstruct)),sizeof(shmstruct));
        //Balioa eraldatu
        shmstruct.speed = atoi(token);
        //Struct-a memoriala itsatsi
        memcpy((struct st_data*)(shmptr + aurkituta*sizeof(shmstruct)), &shmstruct, sizeof(shmstruct));
        semUp(semid);
        
        sprintf(bufferOut, "SPEED %d", atoi(token));
        if((sendto(udp_socket, bufferOut, strlen(bufferOut), 0, &params, sizeofParams))<0){
          perror("Bidaltzean Errorea:\n");
          exit(-1);
        }
        
        //OK jaso
        if((recvfrom(udp_socket, bufferIn, sizeof(bufferIn), 0, &rxParams, &sizeofParams))<0){
          perror("Irakurtzean Errorea:\n");
          exit(-1);
        }
        printf("Erantzuna:\t%s\n", bufferIn);
        break;
      case '3':
      
        semDown(semid);
      
        int rpm;
        memcpy(&rpm, (char*)(messageBuffer.mtext + 1), sizeof(rpm));
        printf("Jasotako mezu interpretatua: %c - %d\n", cmd, rpm);
        
        //Struct-a memoriatik kopiatu
        memcpy(&shmstruct, (struct st_data*)(shmptr + aurkituta*sizeof(shmstruct)),sizeof(shmstruct));
        //Balioa eraldatu
        shmstruct.rpm = rpm;
        //Struct-a memoriala itsatsi
        memcpy((struct st_data*)(shmptr + aurkituta*sizeof(shmstruct)), &shmstruct, sizeof(shmstruct));
        semUp(semid);
        
        sprintf(bufferOut, "RPM %d", rpm);
        if((sendto(udp_socket, bufferOut, strlen(bufferOut), 0, &params, sizeofParams))<0){
          perror("Bidaltzean Errorea:\n");
          exit(-1);
        }
        break;
      case '4':
        //SEM
        int sem;
        memcpy(&sem, (char*)(messageBuffer.mtext + 1), sizeof(sem));
        printf("Jasotako mezu interpretatua: %c - %d\n", cmd, sem);
        
        semDown(semid);
        
        int semval = 0;
        if((semval = semctl(semid, sem, GETVAL))<0){
          perror("Errorea:\n");
          exit(-1);
        }
        
        //Struct-a memoriatik kopiatu
        
        memcpy(&shmstruct, (struct st_data*)(shmptr + aurkituta*sizeof(shmstruct)),sizeof(shmstruct));
        //Balioa eraldatu
        shmstruct.sem = sem;
        shmstruct.semval = semval;
        //Struct-a memoriala itsatsi
        memcpy((struct st_data*)(shmptr + aurkituta*sizeof(shmstruct)), &shmstruct, sizeof(shmstruct));
        semUp(semid);
        
        sprintf(bufferOut, "SEM %d %d", sem, semval);
        if((sendto(udp_socket, bufferOut, strlen(bufferOut), 0, &params, sizeofParams))<0){
          perror("Bidaltzean Errorea:\n");
          exit(-1);
        }
        break;
        //BYE
      case '5':
        sprintf(bufferOut, "BYE");
        if((sendto(udp_socket, bufferOut, strlen(bufferOut), 0, &params, sizeofParams))<0){
          perror("Bidaltzean Errorea:\n");
          exit(-1);
        }
        return(0);
    }
  }
}

int semUp(semid){

  struct sembuf sb;
  
  sb.sem_num = 0;
  sb.sem_op = 1;
  sb.sem_flg = 0;
  
  if((semop(semid, &sb, 1))<0){
    perror("SemUp Errorea:\n");
    exit(-1);
  }

}

int semDown(semid){

    struct sembuf sb;
  
  sb.sem_num = 0;
  sb.sem_op = -1;
  sb.sem_flg = 0;
  
  if((semop(semid, &sb, 1))<0){
    perror("SemDown Errorea:\n");
    exit(-1);
  }

}

















//#========================== ARIKETA 1 ==========================#//
int ariketa1(){
  char bufferIn[255];
  char bufferOut[255];
  struct sockaddr_in params;
  struct sockaddr_in rxParams;
  int udp_socket;
  
  if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("Socket errorea:\n");
    exit(-1);
  }
  
  //HELLO
  sprintf(bufferOut, "HELLO %d DEV-%4d", getpid(), getpid());
  
  params.sin_family = AF_INET;
  params.sin_port = htons(SERVER_PORT);
  params.sin_addr.s_addr = inet_addr(SERVER_IP);
  
  int sizeofParams = sizeof(params);
  
  if((sendto(udp_socket, bufferOut, strlen(bufferOut), 0, &params, sizeofParams))<0){
    perror("Bidaltzean Errorea:\n");
    exit(-1);
  }
  
  //OK jaso
  if((recvfrom(udp_socket, bufferIn, sizeof(bufferIn), 0, &rxParams, &sizeofParams))<0){
    perror("Irakurtzean Errorea:\n");
    exit(-1);
  }
  printf("Erantzuna:\t%s\n", bufferIn);
  
  //PORT
  if((getsockname(udp_socket, &rxParams, &sizeofParams))<0){
    perror("GetSockName Errorea:\n");
    exit(-1);
  }
  
  printf("Jatorriko portua: %d\n", ntohs(rxParams.sin_port));
  
  sprintf(bufferOut, "PORT %d", ntohs(rxParams.sin_port));
  
  if((sendto(udp_socket, bufferOut, strlen(bufferOut), 0, &params, sizeofParams))<0){
    perror("Bidaltzean Errorea:\n");
    exit(-1);
  }
  
  //OK jaso
  if((recvfrom(udp_socket, bufferIn, sizeof(bufferIn), 0, &rxParams, &sizeofParams))<0){
    perror("Irakurtzean Errorea:\n");
    exit(-1);
  }
  printf("Erantzuna:\t%s\n", bufferIn);
  
  usleep(100000);
  
  sprintf(bufferOut, "BYE");
  
  if((sendto(udp_socket, bufferOut, strlen(bufferOut), 0, &params, sizeofParams))<0){
    perror("Bidaltzean Errorea:\n");
    exit(-1);
  }
}
