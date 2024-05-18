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
#include <ctype.h>
#include <fcntl.h>

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

struct mezuIlaraMsg{
  long idMezu;
  char testuMezu[255];
};

struct mezuIlaraRPM{
  long idMezu;
  char agindu;
  char kop[4];
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
/*************************************************************************/


int main(int argc, char *argv[])
{
  int udp_socket = 0;
  struct sockaddr_in parameters, outbound;
  char buffer[255];
  struct mezuIlaraMsg mezuBuffer;
  struct mezuIlaraRPM bufferMezuIlaraRPM;
  
  /* ===== 1. ARIKETA ===== */
  printf("1. Ariketa hasteko eman enter:\n");
  getchar();
  
  //UDP socketa prestatzen:
  if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("Socket-a sortzean arazoak:\n");
    exit(-1);
  }
  
  parameters.sin_family = AF_INET;
  parameters.sin_port=htons(SERVER_PORT);
  parameters.sin_addr.s_addr=inet_addr(DESTIP);
  
  if(connect(udp_socket,(const struct sockaddr *)&parameters,sizeof(parameters))<0){
    perror("Konexio arazoa:\n");
    exit(-1);
  }
  
  int outboundSize = sizeof(outbound);
  if((getsockname(udp_socket, (struct sockaddr *restrict)&outbound, &outboundSize))<0){
    perror("Socket datuak jasotzean arazoa:\n");
    exit(-1);
  }
  
  sprintf(buffer, "%s %d %s", udp_cmd[0], getpid(), "EAS");
  printf("%s\n", buffer);
  
  if(write(udp_socket, buffer, strlen(buffer))<0){
    perror("Datuak idaztean arazoa:\n");
    exit(-1);
  }
  
  sprintf(buffer, "%s %d", udp_cmd[1], ntohs(outbound.sin_port));
  printf("%s\n", buffer);
  
  if(write(udp_socket, buffer, strlen(buffer))<0){
    perror("Datuak idaztean arazoa:\n");
    exit(-1);
  }
  
  printf("BYE agindua emateko eman enter:\n");
  getchar();
  
  sprintf(buffer, "%s", udp_cmd[5]);
  printf("%s\n", buffer);
  
  if(write(udp_socket, buffer, strlen(buffer))<0){
    perror("Datuak idaztean arazoa:\n");
    exit(-1);
  }
  
  if((close(udp_socket))<0){
    perror("Ezin da socket-a itxi!:\n");
    exit(-1);
  }
  
  /* ===== 2. ARIKETA ===== */
  printf("2. Ariketa hasteko eman enter:\n");
  getchar();
  printf("2. ariketa egiten...\n");
  
  //UDP socketa prestatzen:
  if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0))<0){
    perror("Socket-a sortzean arazoak:\n");
    exit(-1);
  }
  
  printf("Socket-aren fd-a lortuta!\n");
  
  parameters.sin_family = AF_INET;
  parameters.sin_port=htons(SERVER_PORT);
  parameters.sin_addr.s_addr=inet_addr(DESTIP);
  
  if(connect(udp_socket,(const struct sockaddr *)&parameters,sizeof(parameters))<0){
    perror("Konexio arazoa:\n");
    exit(-1);
  }
  
  printf("Socket-a konektatzea lortuta!\n");
  
  if((getsockname(udp_socket, (struct sockaddr *restrict)&outbound, &outboundSize))<0){
    perror("Socket datuak jasotzean arazoa:\n");
    exit(-1);
  }
  
  //MEZU ILARA prestatzen:
  printf("Mezu Ilara prestatzen!\n");
  key_t Giltza = CLAVE;
  
  mode_t permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  int mezuIlara = msgget(Giltza, permissions | IPC_CREAT);
  if(mezuIlara == -1){
    perror("Mezu ilara errorea:\n");
    exit(-1);
  }
  
  printf("Mezu ilara prest!\n");
  
  printf("Jasotako mezua %d mezuId-an:\n", getpid());
  printf("%s\n", mezuBuffer.testuMezu);
  
  //UDP-ren gaineko handshake-a
  sprintf(buffer, "%s %d %s", udp_cmd[0], getpid(), "EAS");
  printf("%s\n", buffer);
  
  if(write(udp_socket, buffer, strlen(buffer))<0){
    perror("Datuak idaztean arazoa:\n");
    exit(-1);
  }
  
  sprintf(buffer, "%s %d", udp_cmd[1], ntohs(outbound.sin_port));
  printf("%s\n", buffer);
  
  if(write(udp_socket, buffer, strlen(buffer))<0){
    perror("Datuak idaztean arazoa:\n");
    exit(-1);
  }
  
  printf("UDP hasieratzea eginda, mezu ilararen zain!\n");
  
  printf("Mezua %d mezuId-an itxaroten:\n", getpid());
  int msgRet = msgrcv(mezuIlara, (struct mezuIlaraMsg*)&mezuBuffer, (sizeof(mezuBuffer) - sizeof(mezuBuffer.idMezu)), getpid(), 0);
  if(msgRet < 0){
    perror("Mezua jasotzean errorea:\n");
    exit(-1);
  }
  
  printf("JASOTA:\n%s\n", mezuBuffer.testuMezu);
  
  //ABIADURA
  char *token;
  token = strtok(mezuBuffer.testuMezu, "<");
  printf("1:%s\n", token);
  int aginduMota = atoi(token);
  
  token = strtok(NULL, ">");
  printf("2:%s\n", token);
  
  sprintf(buffer, "%s %s", udp_cmd[aginduMota], token);
  printf("%s\n", buffer);
  
  if(write(udp_socket, buffer, strlen(buffer))<0){
    perror("Datuak idaztean arazoa:\n");
    exit(-1);
  }
  
  printf("Mezua %d mezuId-an itxaroten:\n", getpid());
  msgRet = msgrcv(mezuIlara, (struct mezuIlaraRPM*)&bufferMezuIlaraRPM, (sizeof(bufferMezuIlaraRPM) - sizeof(bufferMezuIlaraRPM.idMezu)), getpid(), 0);
  if(msgRet < 0){
    perror("Mezua jasotzean errorea:\n");
    exit(-1);
  }
  
  //RPM-AK
  //aginduMota = atoi(bufferMezuIlaraRPM.agindu);
  printf("kop:%d\n", atoi(bufferMezuIlaraRPM.kop));
  sprintf(buffer, "%s %s", udp_cmd[(bufferMezuIlaraRPM.agindu-48)], bufferMezuIlaraRPM.kop);
  printf("%s\n", buffer);
  
  //KONKURRENTZIA EGINGO DU RITA LA POLLERA XD
  
  if(write(udp_socket, buffer, strlen(buffer))<0){
    perror("Datuak idaztean arazoa:\n");
    exit(-1);
  }
  
  printf("BYE agindua emateko eman enter:\n");
  getchar();
  
  sprintf(buffer, "%s", udp_cmd[5]);
  printf("%s\n", buffer);
  
  if(write(udp_socket, buffer, strlen(buffer))<0){
    perror("Datuak idaztean arazoa:\n");
    exit(-1);
  }
  
}
