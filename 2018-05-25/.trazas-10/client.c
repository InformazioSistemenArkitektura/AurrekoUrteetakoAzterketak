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

struct shm_dev_reg{
  int estado;	    /* 1 activo, cualquier otra cosa libre */
  int num_dev;	    /* numero de dispositivo #[1..4] */
  char descr[16];	/* descripción del dispositivo*/
  int contador[4];	/* contadores para el dispositivo*/
};

int main(int argc, char* argv[]){
  
  int udp_socket;
  struct sockaddr_in parameters;
  
  if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("UDP socket-a zabaltzerakoan arazoa:\n");
    exit(-1);
  }

  parameters.sin_family = AF_INET;
  parameters.sin_port=htons(SERVER_PORT);
  parameters.sin_addr.s_addr=inet_addr(SERVERIP);
  
  int parameters_size = sizeof(struct sockaddr_in);
  
  //BIND-en erabilera ez dugu gomendatzen zerbitzariaren lana ez dugunean beti egiten eta antzeko egoeretan. Kontuz!
  //Nekaneri idatzi bind-ren ordez zer erabili behar dugun galdetuz
  
  if((bind(udp_socket, (const struct sockaddr*)&parameters, sizeof(parameters)))<0){
    perror("Portua bindeatzean arazoa:\n");
    exit(-1);
  }
  
  char bufferIn[21];
  char bufferOut[21];
  
  if((recvfrom(udp_socket, bufferIn, sizeof(bufferIn),0,(struct sockaddr * restrict)&parameters, &parameters_size))<0){
    perror("Mezua jasotzean arazoa:\n");
    exit(-1);
  }
  bufferIn[21] = '\0';
  printf("%s\n", bufferIn);
   
  sprintf(bufferOut, "OK<%s>", bufferIn);
  printf("%s\n",bufferOut);
  
    if((sendto(udp_socket, bufferOut, sizeof(bufferOut),0,(struct sockaddr * restrict)&parameters, &parameters_size))<0){
    perror("Mezua bidaltzean arazoa:\n");
    exit(-1);
  }
  
  return(0);
}
