#include <stdio.h>
#include <unistd.h>
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

struct mezuilara{
	long mtype;
	int pid;
	char mezua[20];
};

union semun {
	int              val;    /* Value for SETVAL */
	struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
	unsigned short  *array;  /* Array for GETALL, SETALL */
	struct seminfo  *__buf;  /* Buffer for IPC_INFO
								(Linux-specific) */
};


int main(int argc, char *argv[])
{
	int msgid,semid,shmid,pid,fifoid,Amaitua=1,indize;
	struct mezuilara buffer;
	char fifo[BUFLEN],bufferfifo[BUFLEN],*info,*mem;
	struct shmid_ds *buf1;
	struct msqid_ds *buf2;
	union semun arg;
	float speed;
	int RPM,sem,balio;
	struct st_reg *memreg;
	struct st_data *memdata;

	pid=getpid();
	if((msgid=msgget(CLAVE,0600|IPC_CREAT))<0)
	{
		perror("msg:");
	}
	if((shmid=shmget(CLAVE,SIZE_SHM,0600|IPC_CREAT))<0)
	{
		perror("shm:");
	}
	if((semid=semget(CLAVE,MAX_SEM,0600|IPC_CREAT))<0)
	{
		perror("sem:");
	}
	sprintf(fifo,"%s%d",ROOT_NAME_FIFO,pid);
	mkfifo(fifo,0600|IPC_CREAT);
	fifoid=open(fifo,O_RDWR);
	mem=(char*)shmat(shmid,0,0);
	memreg=(struct st_reg*)mem;
	memdata=(struct st_data*)(mem+300);


	
	printf("\nAriketa1\n");
	getchar();

	buffer.mtype=0x01L;
	buffer.pid=pid;
	sprintf(buffer.mezua,"HELLO %d %s",pid,argv[1]);

	printf("\n%s\n",buffer.mezua);
	msgsnd(msgid,&buffer,sizeof(buffer),0);

	msgrcv(msgid,&buffer,sizeof(buffer),(long)pid,0);

	buffer.mtype=0x01L;
	buffer.pid=pid;
	strcpy(buffer.mezua,"BYE");
	printf("\n%s\n",buffer.mezua);
	msgsnd(msgid,&buffer,sizeof(buffer),0);

	printf("\nAriketa2\n");
	getchar();

        

	buffer.mtype=0x01L;
	buffer.pid=pid;
	sprintf(buffer.mezua,"HELLO %d %s",pid,argv[1]);

	msgsnd(msgid,&buffer,sizeof(buffer),0);

	msgrcv(msgid,&buffer,sizeof(buffer),(long)pid,0);

	for(int i=0;i<4;i++)
	{
		if(!strcmp(memreg->name,argv[1]))
		{
			indize=i;
		}
	}

	while(Amaitua)
	{
	        bufferfifo[0] = 0;
	        bufferfifo[1] = 0;
		if((read(fifoid,&bufferfifo,sizeof(bufferfifo)))<0){
		  perror("Ezin da fifo-tik irakurri:\n");
		  exit(-1);
		}
		
		for(int i = 0; i<sizeof(bufferfifo); i++){
		  bufferfifo[i] = bufferfifo[i+1];
		}
		
		printf("FIFO RECEIVED: %s\n",bufferfifo);
		
		if(bufferfifo[0]=='2')
		{
		        printf("SPEED:\n");
			speed=(float)bufferfifo[1];
			(memdata+indize)->speed=speed;
			sprintf(buffer.mezua,"SPEED %f",speed);
		}else{
			if(bufferfifo[0]=='3')
			{
			        printf("RPM:\n");
				strtok(bufferfifo,"<");
				info=strtok(NULL,">");
				RPM=atoi(info);
				(memdata+indize)->rpm=RPM;
				sprintf(buffer.mezua,"RPM %d",RPM);
			}else{
				if(bufferfifo[0]=='4')
				{
				        printf("SEM:\n");
					sem=(int)bufferfifo[1];
					balio=semctl(semid,sem,GETVAL,arg);
					(memdata+indize)->sem=sem;
					(memdata+indize)->semval=balio;
					sprintf(buffer.mezua,"RPM %d %d",sem,balio);
				}else{
					if(bufferfifo[0]=='5')
					{
					        printf("BYE:\n");
						Amaitua=0;
						strcpy(buffer.mezua,"BYE");
					}
				}
			}
		}
		buffer.mtype=0x01L;
		buffer.pid=pid;
		printf("\n%s\n",buffer.mezua);
		msgsnd(msgid,&buffer,sizeof(buffer),0);
	}

	printf("\nAriketa3\n");
	getchar();
        
	close(mem);
	close(fifoid);
	shmctl(shmid,IPC_RMID,buf1);
	msgctl(msgid,IPC_RMID,buf2);
	semctl(semid,0,IPC_RMID,arg);



}
