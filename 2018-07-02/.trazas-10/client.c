#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/signal.h>

#define SERVER_PORT 3010
#define SERVERIP "127.0.0.1"

#define CLAVE 0x45994891L	// Clave de los recursos. Sustituir por DNI.

#define SIZE_SHM 4096		// Tamaño del segmento de memoria compartida

#define MAX_DEV	5		// Máximo # dispositivos # [0..4]



/* El dispositivo 1 comienza en la posición 0 de la memoria
	El dispositivo 2 en la posición 0+OFFSET
	El dispositivo 3 en OFFSET*2
	El dispositivo 4 en OFFSET*3
*/

/* Formato de un registro de dispositivo */

struct shm_dev_reg{
	int estado;	/* 1 activo, cualquier otra cosa libre */
	int num_dev;	/* numero de dispositivo #[1..4] */
	char descr[16];	/* descripción del dispositivo*/
	int contador[4];	/* contadores para el dispositivo*/
};

/* Detrás del registro del dispositivo (+sizeof(struct shm_dev_reg)) 
   se almacenan uno tras otro sus valores en formato (int)
*/


/* El array de semáforos a crear tiene tamaño 5
	El semáforo 0 no se utilizará
	El semáforo 1 controla el acceso al registro del dispositivo 1 
	El semáforo # controla el acceso al registro del dispositivo # [1..4]
*/
#define MAX_SEM 5	// Número de semáforos en el array

/* Lista de mensajes UDP */
#define HELLO	"HLO"
#define OK	"OK"
#define WRITE	'W'

/* Comandos DUMP cola de mensajes mediante señales */

#define MSQ_TYPE_BASE 100
#define DUMP_ALL -1			//Código para el dump de la totalidad de los dispositivos
#define DEV_DUMP 1500		// Posición del dispositivo del que se pide el dump

/*
if(()<0){
  perror("Errorea:\n");
  exit(-1);
}
*/

int ariketa1();
int ariketa2();
int ariketa3();

void sem_down(int semid, int semnum);
void sem_up(int semid, int semnum);

int main(int argc, char *argv[])
{
  char aukera;
  
  printf("Aukeratu ariketa:\n");
  fflush(stdin);
  aukera = getchar();

  switch(aukera){
    case '1':
      ariketa1();
      break;
    case '2':
      ariketa2();
      break;
    case '3':
      ariketa3();
      break;
    case '4':
      ariketa4();
      break;
    default:
      break;
  }
  exit(0);
}



// #======================================= ARIKETA 4 =======================================# //
int ariketa4(){
  
  int semid;
  if((semid = semget(CLAVE, MAX_SEM, 0600|IPC_CREAT))<0){
    perror("Errorea semaforoak lortzean:\n");
    exit(-1);
  }
  
  /***********************************************/
  // SEMAFOROAK PREST
  
  int udp_socket;
  struct sockaddr_in params, client;
  char bufferIn[255];
  char bufferShort[5];
  char bufferOut[9];
  char cmd_mota;
  int dev_num;

  if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("Socket Errorea:\n");
    exit(-1);
  }
  
  printf("Socket-a zabalik!\n");

  params.sin_family = AF_INET;
  params.sin_port = htons(SERVER_PORT);
  params.sin_addr.s_addr = inet_addr(SERVERIP);

  if((bind(udp_socket, &params, sizeof(params)))<0){
    perror("Entzutean errorea:\n");
    exit(-1);
  }
  int sizeofClientAddr = sizeof(struct sockaddr_in);
  
  printf("Socket-a entzuten!\n");
  /***********************************************/
  // UDP PREST
  
  char* shmptr;
  int shmid;
  int kont_num;
  int kont_bal;
  
  if((shmid = shmget(CLAVE, SIZE_SHM, 0600|IPC_CREAT))<0){
    perror("Memoria partekatua lortzean errorea:\n");
    exit(-1);
  }
  
  if((shmptr = shmat(shmid, 0, 0))<0){
    perror("Memoria partekatura attach egitean errorea:\n");
    exit(-1);
  }
  
  printf("Memoria partekatua prest!\n");
  
  /***********************************************/
  // MEMORIA PARTEKATUA PREST
   
  while(1){
    struct shm_dev_reg erregistroa;
  
    if((recvfrom(udp_socket, bufferIn, sizeof(bufferIn), 0, &client, &sizeofClientAddr))<0){
      perror("Mezu bat jasotzean errorea:\n");
      exit(-1);
    }
    
    cmd_mota = (char)bufferIn[0];
    dev_num = *(int*)(bufferIn+1);
    
    switch(cmd_mota){
      case 'H':
        printf("Jaso da: %c\t%d\t%s\n", bufferIn[0], *((int*)(bufferIn+1)), (char*)(bufferIn+5));
        printf("Alta eman:\n");
        
        //Memoria partekatuan erregistroan dagoena kopiatzen dugu lehen
        memcpy(&erregistroa, (struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))), sizeof(erregistroa));
        
        //Erregistroa eraldatzen dugu
        erregistroa.estado = 1;
        erregistroa.num_dev = dev_num;
        sprintf(erregistroa.descr, (char*)(bufferIn+5));
        
        //Erregistro osoa memoria partekatura eramaten dugu
        memcpy((struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))), &erregistroa, sizeof(erregistroa));
        //printf("Memoria partekatuko %d posizioan (%d) sartzen!\n", (dev_num*(sizeof(erregistroa))), (struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))));
       
        sprintf(bufferShort, "O%c%c%c%c", (char)bufferIn[1], (char)bufferIn[2], (char)bufferIn[3], (char)bufferIn[4]);
        if((sendto(udp_socket, bufferShort, sizeof(bufferShort), 0, &client, sizeofClientAddr))<0){
          perror("Mezu bat bidaltzean errorea:\n");
          exit(-1);
        }
        break;
      case 'W':
        printf("Jaso da: %c\t%d\t%d\t%d\n", bufferIn[0], *((int*)(bufferIn+1)), *(int*)(bufferIn+5), *(int*)(bufferIn+9));
        kont_num = *(int*)(bufferIn + 5);
        kont_bal = *(int*)(bufferIn + 9);
        printf("Idatzi: %d - %d\n", kont_num, kont_bal);
        
        //Atal kritikora sartzen ari gara!
        sem_down(semid, dev_num);
        
        //Memoria partekatuan erregistroan dagoena kopiatzen dugu lehen
        memcpy(&erregistroa, (struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))), sizeof(erregistroa));
        
        //Erregistroa eraldatzen dugu
        erregistroa.contador[kont_num] = kont_bal;
        
        //Erregistro osoa memoria partekatura eramaten dugu
        memcpy((struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))), &erregistroa, sizeof(erregistroa));
        
        //Atal kritikotik irteten ari gara!
        sem_up(semid, dev_num);
        
        sprintf(bufferShort, "O%c%c%c%c", (char)bufferIn[1], (char)bufferIn[2], (char)bufferIn[3], (char)bufferIn[4]);
        if((sendto(udp_socket, bufferShort, sizeof(bufferShort), 0, &client, sizeofClientAddr))<0){
          perror("Mezu bat bidaltzean errorea:\n");
          exit(-1);
        }
        break;
      case 'R':
        printf("Jaso da: %c\t%d\t%s\n", bufferIn[0], *((int*)(bufferIn+1)), (char*)(bufferIn+5));
        printf("Irakurri:\n");
        
        //Atal kritikora sartzen ari gara!
        sem_down(semid, dev_num);
        
        //Memoria partekatuan erregistroan dagoena kopiatzen dugu lehen
        memcpy(&erregistroa, (struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))), sizeof(erregistroa));
        
        kont_num = *(int*)(bufferIn + 5);
        kont_bal = erregistroa.contador[kont_num];
        
        //Atal kritikotik irteten ari gara!
        sem_up(semid, dev_num);
        
        sprintf(bufferOut, "O%c%c%c%c%s", (char)bufferIn[1], (char)bufferIn[2], (char)bufferIn[3], (char)bufferIn[4], (char*)&erregistroa.contador[kont_num]);
        
        if((sendto(udp_socket, bufferOut, sizeof(bufferOut), 0, &client, sizeofClientAddr))<0){
          perror("Mezu bat bidaltzean errorea:\n");
          exit(-1);
        }
        break;
    }
  }
}










// #======================================= ARIKETA 3 =======================================# //
int ariketa3(){
  int udp_socket;
  struct sockaddr_in params, client;
  char bufferIn[255];
  char bufferShort[5];
  char bufferOut[9];
  char cmd_mota;
  int dev_num;

  if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("Socket Errorea:\n");
    exit(-1);
  }
  
  printf("Socket-a zabalik!\n");

  params.sin_family = AF_INET;
  params.sin_port = htons(SERVER_PORT);
  params.sin_addr.s_addr = inet_addr(SERVERIP);

  if((bind(udp_socket, &params, sizeof(params)))<0){
    perror("Entzutean errorea:\n");
    exit(-1);
  }
  int sizeofClientAddr = sizeof(struct sockaddr_in);
  
  printf("Socket-a entzuten!\n");
  /***********************************************/
  // UDP PREST
  
  char* shmptr;
  int shmid;
  int kont_num;
  int kont_bal;
  
  if((shmid = shmget(CLAVE, SIZE_SHM, 0600|IPC_CREAT))<0){
    perror("Memoria partekatua lortzean errorea:\n");
    exit(-1);
  }
  
  if((shmptr = shmat(shmid, 0, 0))<0){
    perror("Memoria partekatura attach egitean errorea:\n");
    exit(-1);
  }
  
  printf("Memoria partekatua prest!\n");
  
  /***********************************************/
  // MEMORIA PARTEKATUA PREST
   
  while(1){
    struct shm_dev_reg erregistroa;
  
    if((recvfrom(udp_socket, bufferIn, sizeof(bufferIn), 0, &client, &sizeofClientAddr))<0){
      perror("Mezu bat jasotzean errorea:\n");
      exit(-1);
    }
    
    cmd_mota = (char)bufferIn[0];
    dev_num = *(int*)(bufferIn+1);
    
    switch(cmd_mota){
      case 'H':
        printf("Jaso da: %c\t%d\t%s\n", bufferIn[0], *((int*)(bufferIn+1)), (char*)(bufferIn+5));
        printf("Alta eman:\n");
        
        //Memoria partekatuan erregistroan dagoena kopiatzen dugu lehen
        memcpy(&erregistroa, (struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))), sizeof(erregistroa));
        
        //Erregistroa eraldatzen dugu
        erregistroa.estado = 1;
        erregistroa.num_dev = dev_num;
        sprintf(erregistroa.descr, (char*)(bufferIn+5));
        
        /*
        printf(" ===== Kopiatuko da:\n");
        printf("Status:  %d\n", erregistroa.estado);
        printf("Num_dev: %d\n", erregistroa.num_dev);
        printf("Deskr:   %s\n", erregistroa.descr);
        printf("Deskr:   %d  %d  %d  %d\n", erregistroa.contador[0], erregistroa.contador[1], erregistroa.contador[2], erregistroa.contador[3]);
        printf(" ===== Kopiatuko da^\n");
        */
        
        
        //Erregistro osoa memoria partekatura eramaten dugu
        memcpy((struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))), &erregistroa, sizeof(erregistroa));
        //printf("Memoria partekatuko %d posizioan (%d) sartzen!\n", (dev_num*(sizeof(erregistroa))), (struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))));
       
        sprintf(bufferShort, "O%c%c%c%c", (char)bufferIn[1], (char)bufferIn[2], (char)bufferIn[3], (char)bufferIn[4]);
        if((sendto(udp_socket, bufferShort, sizeof(bufferShort), 0, &client, sizeofClientAddr))<0){
          perror("Mezu bat bidaltzean errorea:\n");
          exit(-1);
        }
        break;
      case 'W':
        printf("Jaso da: %c\t%d\t%d\t%d\n", bufferIn[0], *((int*)(bufferIn+1)), *(int*)(bufferIn+5), *(int*)(bufferIn+9));
        kont_num = *(int*)(bufferIn + 5);
        kont_bal = *(int*)(bufferIn + 9);
        printf("Idatzi: %d - %d\n", kont_num, kont_bal);
        
        //Memoria partekatuan erregistroan dagoena kopiatzen dugu lehen
        memcpy(&erregistroa, (struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))), sizeof(erregistroa));
        
        //Erregistroa eraldatzen dugu
        erregistroa.contador[kont_num] = kont_bal;
        
        /*
        printf(" ===== Kopiatuko da:\n");
        printf("Status:  %d\n", erregistroa.estado);
        printf("Num_dev: %d\n", erregistroa.num_dev);
        printf("Deskr:   %s\n", erregistroa.descr);
        printf("Deskr:   %d  %d  %d  %d\n", erregistroa.contador[0], erregistroa.contador[1], erregistroa.contador[2], erregistroa.contador[3]);
        printf(" ===== Kopiatuko da^\n");
        */
        
        //Erregistro osoa memoria partekatura eramaten dugu
        memcpy((struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))), &erregistroa, sizeof(erregistroa));
        
        sprintf(bufferShort, "O%c%c%c%c", (char)bufferIn[1], (char)bufferIn[2], (char)bufferIn[3], (char)bufferIn[4]);
        if((sendto(udp_socket, bufferShort, sizeof(bufferShort), 0, &client, sizeofClientAddr))<0){
          perror("Mezu bat bidaltzean errorea:\n");
          exit(-1);
        }
        break;
      case 'R':
        printf("Jaso da: %c\t%d\t%s\n", bufferIn[0], *((int*)(bufferIn+1)), (char*)(bufferIn+5));
        printf("Irakurri:\n");
        
        //Memoria partekatuan erregistroan dagoena kopiatzen dugu lehen
        memcpy(&erregistroa, (struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))), sizeof(erregistroa));
        
        kont_num = *(int*)(bufferIn + 5);
        kont_bal = erregistroa.contador[kont_num];
        
        /*
        printf(" ===== Kopiatuko da:\n");
        printf("Status:  %d\n", erregistroa.estado);
        printf("Num_dev: %d\n", erregistroa.num_dev);
        printf("Deskr:   %s\n", erregistroa.descr);
        printf("Deskr:   %d  %d  %d  %d\n", erregistroa.contador[0], erregistroa.contador[1], erregistroa.contador[2], erregistroa.contador[3]);
        printf(" ===== Kopiatuko da^\n");
        */
        
        sprintf(bufferOut, "O%c%c%c%c%s", (char)bufferIn[1], (char)bufferIn[2], (char)bufferIn[3], (char)bufferIn[4], (char*)&erregistroa.contador[kont_num]);
        
        if((sendto(udp_socket, bufferOut, sizeof(bufferOut), 0, &client, sizeofClientAddr))<0){
          perror("Mezu bat bidaltzean errorea:\n");
          exit(-1);
        }
        break;
    }
  }
}








// #======================================= ARIKETA 2 =======================================# //
int ariketa2(){
  int udp_socket;
  struct sockaddr_in params, client;
  char bufferIn[255];
  char bufferOut[5];
  int dev_num;
  char cmd_mota;
  
  struct shm_dev_reg erregistroa;

  if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("Socket Errorea:\n");
    exit(-1);
  }
  
  printf("Socket-a zabalik!\n");

  params.sin_family = AF_INET;
  params.sin_port = htons(SERVER_PORT);
  params.sin_addr.s_addr = inet_addr(SERVERIP);

  if((bind(udp_socket, &params, sizeof(params)))<0){
    perror("Entzutean errorea:\n");
    exit(-1);
  }
  int sizeofClientAddr = sizeof(struct sockaddr_in);
  
  printf("Socket-a entzuten!\n");
  /***********************************************/
  // UDP PREST
  
  char* shmptr;
  int shmid;
  
  if((shmid = shmget(CLAVE, SIZE_SHM, 0600|IPC_CREAT))<0){
    perror("Memoria partekatua lortzean errorea:\n");
    exit(-1);
  }
  
  if((shmptr = shmat(shmid, 0, 0))<0){
    perror("Memoria partekatura attach egitean errorea:\n");
    exit(-1);
  }
  
  printf("Memoria partekatua prest!\n");
  
  /***********************************************/
  // MEMORIA PARTEKATUA PREST
   
  for(int i = 0; i<6; i++){
    if((recvfrom(udp_socket, bufferIn, sizeof(bufferIn), 0, &client, &sizeofClientAddr))<0){
      perror("Mezu bat jasotzean errorea:\n");
      exit(-1);
    }
    
    cmd_mota = (char)bufferIn[0];
    dev_num = (int)*(bufferIn+1);
    
    printf("bufferIn:\n%s\t%d\t%s\n", bufferIn, dev_num, (char*)(bufferIn+5));
    sprintf(bufferOut, "O%c%c%c%c", (char)bufferIn[1], (char)bufferIn[2], (char)bufferIn[3], (char)bufferIn[4]);

    erregistroa.estado = 1;
    erregistroa.num_dev = dev_num;
    sprintf(erregistroa.descr, (char*)(bufferIn+5));
    
    memcpy((struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))), &erregistroa, sizeof(erregistroa));
    printf("Memoria partekatuko %d posizioan (%d) sartzen!\n", (dev_num*(sizeof(erregistroa))), (struct shm_dev_reg*)(shmptr + (dev_num*(sizeof(erregistroa)))));

    if((sendto(udp_socket, bufferOut, sizeof(bufferOut), 0, &client, sizeofClientAddr))<0){
      perror("Mezu bat bidaltzean errorea:\n");
      exit(-1);
    }
  }
}







// #======================================= ARIKETA 1 =======================================# //

int ariketa1(){
  int udp_socket;
  struct sockaddr_in params, client;
  char bufferIn[255];
  char bufferOut[5];

  if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("Socket Errorea:\n");
    exit(-1);
  }
  
  printf("Socket-a zabalik!\n");

  params.sin_family = AF_INET;
  params.sin_port = htons(SERVER_PORT);
  params.sin_addr.s_addr = inet_addr(SERVERIP);

  if((bind(udp_socket, &params, sizeof(params)))<0){
    perror("Entzutean errorea:\n");
    exit(-1);
  }

  printf("Socket-a entzuten!\n");

  int sizeofClientAddr = sizeof(struct sockaddr_in);
 
  if((recvfrom(udp_socket, bufferIn, sizeof(bufferIn), 0, &client, &sizeofClientAddr))<0){
    perror("Mezu bat jasotzean errorea:\n");
    exit(-1);
  }
  
  printf("bufferIn:\n%s\t%d\t%s\n", bufferIn, (int)*(bufferIn+1), (char*)(bufferIn+5));
  sprintf(bufferOut, "O%c%c%c%c", (char)bufferIn[1], (char)bufferIn[2], (char)bufferIn[3], (char)bufferIn[4]);

  if((sendto(udp_socket, bufferOut, sizeof(bufferOut), 0, &client, sizeofClientAddr))<0){
    perror("Mezu bat bidaltzean errorea:\n");
    exit(-1);
  }
}

void sem_up(int semid, int semnum){
  struct sembuf sb;
  
  sb.sem_num = semnum;
  sb.sem_op = 1;
  sb.sem_flg = 0;
  
  if((semop(semid, &sb, 1))<0){
    perror("UP egitean errorea:\n");
    exit(-1);
  }
}

void sem_down(int semid, int semnum){
  struct sembuf sb;
  
  sb.sem_num = semnum;
  sb.sem_op = -1;
  sb.sem_flg = 0;
  
  if((semop(semid, &sb, 1))<0){
    perror("DOWN egitean errorea:\n");
    exit(-1);
  }
}
