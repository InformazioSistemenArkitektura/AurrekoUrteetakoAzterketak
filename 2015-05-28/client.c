#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>



#define SERVER_PORT 3010
#define SERVER_IP "127.0.0.1"
#define CLAVE 0x45994891L
#define SHM_SIZE 1024

union semun {
   int              val;    /* Value for SETVAL */
   struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
   unsigned short  *array;  /* Array for GETALL, SETALL */
   struct seminfo  *__buf;  /* Buffer for IPC_INFO
                               (Linux-specific) */
};

int ariketa1();
int semDown(semid, semnum);
int semUp(semid, semnum);

  /*
  
  if(()<0){
    perror("ERROR:\n");
    exit(-1);
  }
  
  */

int main(int argc, char* argv[]){
  
  char aukera;
  printf("Aukeratu ariketa bat!\n");
  aukera = getchar();
  
  switch(aukera){
    case '1':
      ariketa1();
      break;
    default:
      exit(-1);
  }


  return(0);
}


 //#====================================================== ARIKETA 1 ======================================================#//
int ariketa1(){

  int shmid = 0;
  char* shmptr;
  
  if((shmid = shmget(CLAVE, SHM_SIZE, 0600|IPC_CREAT))<0){
    perror("SHMGET ERROR:\n");
    exit(-1);
  }

  if((shmptr = shmat(shmid, 0, 0))<0){
    perror("SHMAT ERROR:\n");
    exit(-1);
  }

  //************** SHMEM PREST **************//
  
  int semid;
  if((semid = semget(CLAVE, 3, 0600|IPC_CREAT))<0){
    perror("SEMGET ERROR:\n");
    exit(-1);
  }
  
  //************** SEM PREST **************//


  int tcp_socket;
  struct sockaddr_in params;
  
  if((tcp_socket = socket(AF_INET, SOCK_STREAM, 0))<0){
    perror("TCP SOCKETERROR:\n");
    exit(-1);
  }
  
  params.sin_family = AF_INET;
  params.sin_port = htons(SERVER_PORT);
  params.sin_addr.s_addr = inet_addr(SERVER_IP);
  int sizeofParams = sizeof(params);
  
  if((bind(tcp_socket, &params, sizeofParams))<0){
    perror("BIND-EGITEAN ERROR:\n");
    exit(-1);
  }
  
  if((listen(tcp_socket, 50))<0){
    perror("LISTEN ERROR:\n");
    exit(-1);
  }
  //************** TCP PREST **************//
   
   int pid1 = 0;
   if((pid1 = fork()) == 0){
   
      while(1){
      
        int tcp_accept;
        if((tcp_accept = accept(tcp_socket, &params, &sizeofParams))<0){
          perror("ERROR:\n");
          exit(-1);
        }
        
        semDown(semid, 0);
        
        if(fork()==0){
          char childBuffer[255];
          char outBuffer[255];
          struct sockaddr_in clientParams;
          
          close(tcp_socket);
          printf("Konexioa jasota!\n");
          
          while((recvfrom(tcp_accept, childBuffer,sizeof(childBuffer), 0, &clientParams, &sizeofParams))>0){
            
            semDown(semid, 1);
            
            printf("Konexio eskatua\n");
            printf("Iragaziko den komandoa: %s\n", childBuffer);
            
            char cmdDet = childBuffer[0];
            
            switch(cmdDet){
              case 'h':
                  printf("Jasotako mezua:\t%s\n", childBuffer);
                
                  sprintf(outBuffer, "OK\n");
                  
                  printf("Erantzukgo dena:\t%s\n", outBuffer);
                  
                  if((sendto(tcp_accept, outBuffer, strlen(outBuffer), 0, &clientParams, sizeofParams))<0){
                    perror("SENDTO ERROR:\n");
                    exit(-1);
                  }
                break;
                
              case '<':
              
                int offset = 0;
              
                char* token;
                token = strtok(childBuffer, "<>");
                printf("1:%s\n", token);
                offset = atoi(token);
                token = strtok(NULL, "<>");
                printf("2:%s\n", token);
                
                semDown(semid, 2);switch(token[0]){
                  case '=':
                    printf("Idazteko komandoa!\n");
                    int idaztekoa = atoi((char*)(token + 1));
                    printf("Idatzi behar da: %d\n", idaztekoa);
                    
                    memcpy((int*)(shmptr + (offset)), &idaztekoa, sizeof(idaztekoa));
                    
                    sprintf(outBuffer, "\n\0");
                    printf("Erantzukgo dena:\t%s\n", outBuffer);
                    
                    if((sendto(tcp_accept, outBuffer, strlen(outBuffer), 0, &clientParams, sizeofParams))<0){
                      perror("SENDTO ERROR:\n");
                      exit(-1);
                    }
                    
                    break;
                  case '?':
                    printf("Irakurtzeko komandoa!\n");
                    
                    int irakurtzekoa = 0;
                    
                    memcpy(&irakurtzekoa, (int*)(shmptr + (offset)), sizeof(irakurtzekoa));
                    
                    sprintf(outBuffer, "%d\n\0", irakurtzekoa);
                    printf("Erantzukgo dena:\t%s\n", outBuffer);
                    
                    if((sendto(tcp_accept, outBuffer, strlen(outBuffer), 0, &clientParams, sizeofParams))<0){
                      perror("SENDTO ERROR:\n");
                      exit(-1);
                    }
                    
                    break;
                  case '+':
                  
                    int operandA = 0;
                    int operandB = 0;
                    int offsetB = 0;

                    printf("Gehiketarako komandoa!\n");
                    token = strtok(NULL, "<>");
                    offsetB = atoi(token);
                    printf("3:%s\n", token);
                    printf("Gehitu behar dira: %d eta %d helbideetakoak\n", offset, offsetB);
                    
                    memcpy(&operandA, (int*)(shmptr + (offset)), sizeof(operandA));
                    memcpy(&operandB, (int*)(shmptr + (offsetB)), sizeof(operandB));
                    
                    sprintf(outBuffer, "%d\n\0", (operandA + operandB));
                    printf("Erantzukgo dena:\t%s\n", outBuffer);
                    
                    if((sendto(tcp_accept, outBuffer, strlen(outBuffer), 0, &clientParams, sizeofParams))<0){
                      perror("SENDTO ERROR:\n");
                      exit(-1);
                    }
                    
                    break;
                }
                
                semUp(semid, 2);
              
                break;
                
              default:
                printf("Zeozer txarto joan da!\n Jasotakoa: %s", childBuffer);
                exit(-1);
            }
            semUp(semid, 1);
          }
          
          shutdown(tcp_accept, SHUT_WR);
          close(tcp_accept);
          exit(0);
        }
        else{
          close(tcp_accept);
        }
      }
   }
   else{
    int status;
    waitpid(pid1,&status,0);
    semUp(semid, 0);
    printf("Konnexioa galduta");
   }
}

int semUp(semid, semnum){

  struct sembuf sb;
  
  sb.sem_num = semnum;
  sb.sem_op = 1;
  sb.sem_flg = 0;
  
  if((semop(semid, &sb, 1))<0){
    perror("SEMUP ERROR:\n");
    exit(-1);
  }
}

int semDown(semid, semnum){

  struct sembuf sb;
  
  sb.sem_num = semnum;
  sb.sem_op = -1;
  sb.sem_flg = 0;
  
  if((semop(semid, &sb, 1))<0){
    perror("SEMUP ERROR:\n");
    exit(-1);
  }
}
