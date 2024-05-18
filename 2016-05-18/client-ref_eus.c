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

#define CLAVE 0x11223344L	// Baliabideetarako gakoa. Zure NAN jarri.

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

}


