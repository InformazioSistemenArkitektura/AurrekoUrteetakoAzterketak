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


int main(int argc, char *argv[])
{
  struct sockaddr_in parameters;
  struct sockaddr_in client;
  int sd;
  char buffer[255];
  
  if((sd=socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("Socket errorea:\n");
    exit(-1);
  }

  //#================= ARIKETA 1 ====================#
  printf("1. ariketa egiteko eman enter:\n");
  getchar;
  
  parameters.sin_family = AF_INET;
  parameters.sin_port=htons(SERVER_PORT);
  parameters.sin_addr.s_addr=inet_addr(SERVERIP);

  int param_size = sizeof(parameters);
  
  if((bind(sd, &parameters, param_size))<0){
    perror("Bind errorea:\n");
    exit(-1);
  }
  
  int client_size = sizeof(client);
  if(recvfrom(sd, buffer, sizeof(buffer), 0, &client, &client_size)<0){
    perror("Irakurtzean errorea:\n");
    exit(-1);
  }
  
  printf("Buffer-eko mezua:\n%s\n", buffer);
  char*token;
  token=strtok(buffer, "<");
  token=strtok(NULL, ">");
  
  sprintf(buffer, "OK <%s>", token);
  
  printf("Bidaliko den mezua:\n%s\n", buffer);
  if(sendto(sd, buffer, sizeof(buffer), 0, &client, client_size)<0){
    perror("Idaztean errorea:\n");
    exit(-1);
  }
  
  //close(sd);
  
  //#================= ARIKETA 2 ====================#
  printf("2. ariketa egiteko eman enter:\n");
  getchar();
  
  //                        R/W User,         R/W Group,            R/W Others
  //mode_t permissions =  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  int shmid;
  void* shmptr;
  
  printf("Memoria partekatua sortzen...\n");
  
  if(shmid = shmget(CLAVE, SIZE_SHM, 0600 | IPC_CREAT)<0){
    perror("Ezin da memoria partekatua lortu:\n");
    exit(-1);
  }
  
  printf("Memoria partekatura atzitzen...\n");
  
  struct shm_dev_reg *ptr = shmat(shmid, NULL, 0600|IPC_CREAT);
  if((ptr) < 0){
    perror("Shared memory ezin da lotu:\n");
    exit(-1);
  }

  printf("Memoria partekatua prest\n");
  
  if(recvfrom(sd, buffer, sizeof(buffer), 0, &client, &client_size)<0){
    perror("Irakurtzean errorea:\n");
    exit(-1);
  }
  
  printf("Buffer-eko mezua:\n%s\n", buffer);
  token=strtok(buffer, "<");
  token=strtok(NULL, ">");
  //Char-a int-era zuzenean bihurtzeko
  int num_dev=(*token)-48;
  printf("num_dev=%d\n", num_dev);
  char buffer2[255];
  sprintf(buffer2, "OK <%d>", num_dev);
  printf("Erantzuna=%s\n", buffer);
  token=strtok(NULL, "<");
  token=strtok(NULL, ">");
  int n_cont=(*token)-48;
  printf("n_cont=%d\n", n_cont);
  token=strtok(NULL, " ");
  char desk[16];
  strcpy(desk, token);
  printf("desk=%s\n", desk);

  ptr->egoera = 1;
  ptr->num_dev = num_dev;
  ptr->n_cont=n_cont;
  strcpy(ptr->deskr, desk);
  
  printf("Kopiatzen...\n");
  
  //memcpy(shmptr, &gailua, sizeof(gailua));
  //(struct shm_dev_reg*)shmptr = gailua;
  int tmp = 1;
  memcpy((int*)shmptr, &tmp, sizeof(int));
  
  printf("Bidaliko den mezua:\n%s\n", buffer);
  if(sendto(sd, buffer, sizeof(buffer), 0, &client, client_size)<0){
    perror("Idaztean errorea:\n");
    exit(-1);
  }
  
  close(sd);
  
  exit(0);
}
