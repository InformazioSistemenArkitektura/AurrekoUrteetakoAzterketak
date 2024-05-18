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

#define CLAVE 0x45994891L	// Baliabideetarako gakoa. Zure NAN jarri.

#define SIZE_SHM 4096		// Memoria partekatuaren segmentu-luzera

#define MAX_DEV	4		// Gailu kopuru maximoa #[1..4]

#define DEVOFFSET 200		// Gailuen arteko Offseta

/* 1. gailua memoriaren 0 kokapenean hasten da
	2 gailua 0+OFFSET kokapenean
	3 gailua 0+OFFSET*2 kokapenean
	4 gailua 0+OFFSET*3 kokapenean
*/

/* Gailuen erregistroen formatua */

struct shm_dev_reg{
	int egoera;	/* 1 lanean, gainerako edozer libre */
	int num_dev;	/* gailuaren zenbakia #[1..4] */
	char deskr[15];	/* gailuaren deskripzioa */
	int n_cont;	/* gailuaren kontagailu kopurua */
};

/* Gailuaren erresgistroaren ondoren (+sizeof(struct shm_dev_reg)) 
   sentsorearen neurriak gordetzen dira ondoz ondo (int) formatuan
*/


/* Sortu beharreko semaforo-arrayak 5 luzera du
	0 semaforoa ez erabiliko
	1 semaforoak 1 gailuaren atzipena kontrolatzen du
	#[1..4] semaforoak #[1..4] gailuaren atzipena kontrolatzen du
*/
#define MAX_SEM 5	// Arrayaren semaforo kopurua

/* UDP mezuak*/
#define HELLO	"HLO"
#define OK	"OK"
#define WRITE	'W'

/* Mezu-ilararen komandoak */
#define DUMP	'D'

/* Komando + gailu zenbakia irakurtzeko egitura: Guztira 8 byte */

struct msgq_input {
	char cmd;	/* komandoa */
	int num_dev;	/* gailu zenbakia */
};

int main(int argc, char* argv[]){
  
  struct sockaddr_in params, client;
  int udp_socket;
  char buffer[255];
  
  int dev_num;
  int n_cont;
  char deskr[32];
  char* token;
  
  if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    perror("Socket-a sortzean arazoak\n");
    exit(-1);
  }
  
  params.sin_family=AF_INET;
  params.sin_port=htons(SERVER_PORT);
  params.sin_addr.s_addr=inet_addr(SERVERIP);
  
  if((bind(udp_socket, &params, sizeof(params))) < 0){
    perror("Socket-a sortzean arazoak\n");
    exit(-1);
  }
  
  int client_size = sizeof(client);
  
  if((recvfrom(udp_socket, buffer, sizeof(buffer), 0, &client, &client_size))<0){
    perror("Irakurtzean errorea:\n");
    exit(-1);
  }
  
  token = strtok(buffer, "<");
  printf("1:%s\n", token);
  token = strtok(NULL, ">");
  printf("2:%s\n", token);
  dev_num = atoi(token);
  token = strtok(NULL, "<");
  printf("3:%s\n", token);
  token = strtok(NULL, ">");
  printf("4:%s\n", token);
  
  sprintf(buffer, "OK <%d>", dev_num);
  printf("%s\n", buffer);
  
  if(sendto(udp_socket,buffer,sizeof(buffer),0,&client,client_size)<0){
    perror("Idaztean errorea: ");
    exit(-1);
  }
  
  printf("2. Ariketa hasteko eman enter\n");
  getchar();
  
  int shmid;
  if((shmid=shmget(CLAVE, SIZE_SHM, 0600|IPC_CREAT))<0){
    perror("Shared memory-a lortzean errorea:");
    exit(-1);
  }
  printf("Shmid: %d\n", shmid);
  
  char *ptr = (char*)shmat(shmid, 0,0);
  if((ptr)==(char*)-1){
    perror("Shmat errorea:\n");
    exit(-1);
  }
  
  struct shm_dev_reg* stptr;
  
  for(int i = 0; i<5; i++){
  
    if((recvfrom(udp_socket, buffer, sizeof(buffer), 0, &client, &client_size))<0){
      perror("Irakurtzean errorea:\n");
      exit(-1);
    }
    
    printf("%s\n", buffer);
    
    token = strtok(buffer, "<");
    printf("1:%s\n", token);
    token = strtok(NULL, ">");
    printf("2:%s\n", token);
    
    dev_num = atoi(token);
    
    token = strtok(NULL, "<");
    printf("3:%s\n", token);
    token = strtok(NULL, ">");
    printf("4:%s\n", token);
    n_cont = atoi(token);
    
    token = strtok(NULL, " ");
    printf("5:%s\n", token);
    strcpy(deskr, token);
    
    sprintf(buffer,"OK <%d>\n", dev_num);
    printf("Reply ready: %s\n", buffer);
    
    printf("Gailua erregistratzeko parametroak:\n\tdev_num=%d\n\tn_cont=%d\n\tDeskribapena: %s\n", dev_num, n_cont, deskr);
    
    //(cast)(puntero aritmetika)
    //EZ DA CASTEATUTAKO TAIMAINAREKIN ARITMETIKA EGITEN; CHAR TAMAINAREKIN BAIZIK!
    stptr=(struct shm_dev_reg*)(ptr + (dev_num-1)*DEVOFFSET);
    
    stptr-> num_dev=dev_num;
    stptr->n_cont=n_cont;
    stptr-> egoera = 1;
    strcpy(stptr->deskr,deskr);
    if(dev_num==3){
      strcpy(stptr->deskr,"Termometer");
    }
    
    printf("Memoria partekatuan sartutakoa:\n\tEgoera\t%d\n\tDevNum\t%d\n\tN_Cont \t%d\n\tDeskr.\t%s\n", stptr->egoera, stptr->num_dev, stptr->n_cont, stptr->deskr);
    
    printf("Bidaliko da: %s\n\n\n",buffer);
    if(sendto(udp_socket,buffer,sizeof(buffer),0,&client,client_size)<0){
      perror("Idaztean errorea: ");
      exit(-1);
    }
  
  }

  return(0);
}
