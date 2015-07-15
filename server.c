// Server.c 

/***************************************
// Simple Scrable game via TCP socket.//
// Written by Wael Almattar.          //
// All rights reserved (c).           //
// waelsy123@gmail.com                //
***************************************/

#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "scrlib.h"
#include <time.h>
#include <semaphore.h>
#include <stdbool.h> 
#include <arpa/inet.h>

// Socket to be closed by SigINT Ctrl+c
volatile int pid_socket = 0;

// If there is no game waiting: Initilaze New game by creating struct game in the shard memory 
// If there is already game waiting for the second peer to join: the peer will get the key of 
// this game to access is via shared memory.
int  newgame(int sd ,int id ){
	// shm is flag in memory tell us if there is a client waiting for opinent 
	int shmid , *shm ; 
	if ((shmid = shmget( KEY_SHM , SHMSZ, 0666)) < 0)  ERR("shmget");
	if ((shm = shmat(shmid, NULL, 0)) ==  (void *)-1) ERR("shmat");

        // Get the key of shared memory to access struct game 
        int shmidKey , *key_game ;
        if ((shmidKey = shmget( KEY_OF_KEY_GAME , SHMSZ, 0666)) < 0) 
                ERR("shmget");
        if ((key_game = shmat(shmidKey, NULL, 0)) == (void *) -1) 
                ERR("shmat");

	if(*shm == 0){
		// as a main player create new Struct game;
		*shm = id ;
	    	int shmidGame;
	    	key_t keyGame = *key_game ;
	    	struct game  *daGame;
	
	    	if ((shmidGame = shmget(keyGame, SHMSZ, IPC_CREAT | 0666)) < 0) 
       			ERR("shmget");
  	   
	  	if ((daGame = shmat(shmidGame, NULL, 0)) == (void *) -1) {
  	  	      	ERR("shmat");
  	  	}

		initGame(daGame); 
		daGame->turn = id ; 	
	 	return *key_game ;	
	}
	else{
		// as a second player get the key to access pre-initilized struct game; 
		*shm =  0 ; 
		*key_game = *key_game +1  ; 
		return (*key_game-1) ; 
	}

}


// Write a summary of each game to log file.
void writeGameToFile(struct game daGame,  int ip1, int ip2){
	
	char ip1str[INET_ADDRSTRLEN];
	inet_ntop( AF_INET, &ip1, ip1str, INET_ADDRSTRLEN );
	
        char ip2str[INET_ADDRSTRLEN];
        inet_ntop( AF_INET, &ip2, ip2str, INET_ADDRSTRLEN );

	FILE *f = fopen("Scrable-logs.txt", "a");
	if (f == NULL) ERR("Open file");
	
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	fprintf(f,"Date: %d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	if(daGame.finished==1) fprintf(f,"Game Finished\n") ;
	else fprintf(f,"Game Unresolved\n") ;

	fprintf(f,"Player1 Ip Address: %s\n" , ip1str); 
	fprintf(f,"Player2 Ip Address: %s\n" , ip2str);
	printGame(f,daGame) ; 
	
	fprintf(f,"\nMOVES:\n\n") ; 
	int i;
	for(i=0;i<LETTERSSZ && daGame.steps[i][4]=='\0'  && daGame.steps[i][0]>='a' && 'z' >= daGame.steps[i][0]  ;i++){
		if(i%2==0) fprintf(f,"Player1: %s\n", daGame.steps[i] );
		else   fprintf(f,"Player2: %s\n", daGame.steps[i] ); 
	}

	fprintf(f, "\n --------------------- \n\n") ; 
	fclose(f);
}

// Setting the semaphore value to zero.
void sem_zero(sem_t *sem){
	int val ;
        while(sem_getvalue(sem, &val)==0 && val!=0) if(sem_wait(sem)<0) ERR("wait");
}

// Each new client connection new proccess will be created and will execute this function 
void dowork(int sd, int id, int ip) {
	pid_socket = sd ; 
	
       	// send the id of client to the client ;
	int n;
	char  sendline[LINESZ] ; 
	snprintf(sendline, LINESZ , "%d", id  ); 
	n = write(sd,sendline, LINESZ);
        if (n <= 0) ERR("ERROR writing to socket");
 	bool again = true ; 

	while(again){
	
		// get the id of the game ;
		int key_game = newgame(sd, id); // oppinion IDs has been sent! 
	
		// Here we will get the board from Shared mem V
		struct game * daGame ; 
	        int shmidGame  ;
       		key_t key_key = key_game ;
		if ((shmidGame = shmget(key_key, SHMSZ, 0666)) < 0)       ERR("shmget");
	        if ((daGame = shmat(shmidGame, NULL, 0)) == (void *) -1)  ERR("shmat");
	       	
       	 	
		snprintf(sendline, LINESZ , "wael1sem%d", daGame->turn  );
       	 	if (n <= 0) ERR("ERROR writing to socket");
		sem_t *sem = sem_open(sendline, O_CREAT, 0600 , 0);	
		if( sem == SEM_FAILED) ERR("sem_open");
		sem_zero(sem);
			
		bool boss = false ; 
		int i = -2 ; 
		if(daGame->turn != id ){
			// Second peer pre-game job.
			sem_post(sem); // is boss alive?
			
			// within these 500 milliseconds the boss sould answer are you alive message by 
			// sending "alive" message to its server process
			usleep(500);
			int val ;
			sem_getvalue(sem, &val);  
			if(!val) continue; // boss dead	
			daGame->op = id ; 
			daGame->ip2 = ip ; 
                        n = write(sd,"alive" , 6 );
                        if (n <= 0) ERR("ERROR writing to socket");
                        n = read(sd, sendline , 6 );
                        if (n < 0)
                                ERR("ERROR reading from socket");

			i++;
		}
		else {
			// First peer pre-game job. (boss)
			boss = true ;
			if(sem_wait(sem)<0) ERR("wait");
			n = write(sd,"alive" , 6 );
			if (n <= 0) ERR("ERROR writing to socket");
			fd_set readfd;
		        FD_ZERO(&readfd);
		        FD_SET(sd, &readfd);

        		struct timeval tv;
      	 		tv.tv_sec = 0;  
        		tv.tv_usec = 500;
        		n= select(sd+1, &readfd, NULL, NULL, &tv);
			if(n==1) sem_post(sem);
			else break;
	                n = read(sd, sendline , 6 );
                        if (n < 0)
        	                ERR("ERROR reading from socket");
		}
		if(sem_wait(sem)<0) ERR("wait"); 

		int ip1 = ip ;  
		int ip2 = 0 ;  
		while( true ){
			// Send game, Recive it, Recive move, Flip turns.
			sem_post(sem);
			usleep(500);	
			if(sem_wait(sem)<0) ERR("wait");
			if(ip2 == 0) { ip2 = daGame->ip2 ; }
				
			if(daGame->finished ==-1 ) break ;
 
			printGame(stdout,*daGame);
			sendGame(sd, *daGame  ); 	
			
			if(daGame->finished == 1) break; 		
			recvGame(sd, daGame  );
			if(daGame->finished ==-1 ) break ;	
			
			// RECV steps	
		        i=i+2;
			bzero(daGame->steps[i],MOVESZ);
		        n = read(sd , daGame->steps[i] , MOVESZ );
		        if (n < 0) ERR("ERROR reading from socket");
	
			if(daGame->finished == 1) break ; 	
			int op = daGame->turn ; 
			daGame->turn = daGame->op ;	
			daGame->op  = op ; 
			if( getLastLetter(daGame->letters, LETTERSSZ-1) == -1 &&
			    getLastLetter(daGame->p1     , HANDSZ-1 ) == -1 && getLastLetter(daGame->p2 ,HANDSZ-1)== -1 ) 
				
				daGame->finished = 1; 
		}

		// Write summary of this game to log file 
		// and Ask if the peer would like to play again. 
		sem_post(sem);
		if( boss ) writeGameToFile( *daGame,  ip1 , ip2 ); 
		if( daGame->finished ==-1 ) printf("Client %d disconnected\n", id) ; 
		
		char buffer[2] ;	
	        bzero(buffer,2);
       		if( (n = read(sd, buffer , 2) )< 0) ERR("ERROR: read");
		if(atoi(buffer) ==0) again=false ;   
	
		sem_zero(sem) ; 
		if( sem_destroy(sem) != 0 ) ERR("Sema Destroy");
	
	}
	
	
}
void sigint_handler(int pid){
	//if(TEMP_FAILURE_RETRY(close(pid_socket ))<0)ERR("close");
	exit(EXIT_SUCCESS);
}
	
int main(int argc, char *argv[])
{
	int sockfd, newsockfd[CLIENTS_NUM] , portno ; 
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	if (argc < 2){ 
		printf("ERROR, no port provided\n");
		return EXIT_SUCCESS;
	}
	if(sethandler( SIG_IGN ,SIGPIPE)) 
		ERR("Seting SIGPIPE:");
	if(sethandler( sigint_handler , SIGINT )) ERR("Setting SIGINT") ;  	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)  ERR("ERROR opening socket");
     
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
     	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
              ERR("ERROR on binding");
     	listen(sockfd,5);
     	clilen = sizeof(cli_addr);
     	int i ;
	
	// shm is shared memory integer tell us if there is game waiting for 2nd peer(0) 
	// or the id of the peer who created a game 
    	int shmid;
    	int *shm;
   	if((shmid = shmget( KEY_SHM , SHMSZ, IPC_CREAT | 0666)) < 0) 	 ERR("shmget");
	if((shm = shmat(shmid, NULL, 0)) == (void *) -1)         ERR("shmat");
	*shm = 0 ; 
	
	// key_game will be stored in shared memory. it's a key for another shared memory 
	// which will allow both peer access a their exact game. 	
	int * key_game ; 
	int shmidKey;
    	if ((shmidKey = shmget( KEY_OF_KEY_GAME , SHMSZ , IPC_CREAT | 0666)) < 0 ) ERR("shmget");
    	if ((key_game = shmat(shmidKey, NULL, 0)) == (void *) -1) ERR("shmat");
	*key_game = KEY_GAME ; 

	pid_socket = sockfd ;  
	for( i =0;i<CLIENTS_NUM;i++){
		
 	        newsockfd[i] = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	        if(newsockfd[i] < 0) ERR("ACCEPT"); 
	        int pidno = fork() ; 
	        if(pidno==0){
			struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&cli_addr;
			int ipAddr = pV4Addr->sin_addr.s_addr;
			dowork(newsockfd[i], i+1, ipAddr);			
			if(TEMP_FAILURE_RETRY(close(newsockfd[i]))<0)ERR("close"); 	
		}
		else if(pidno ==-1) ERR("fork");
		else{
			if(TEMP_FAILURE_RETRY(close(newsockfd[i]))<0)ERR("close"); 	
		}	
	}
	if(TEMP_FAILURE_RETRY(close(sockfd))<0)ERR("close");
     	return EXIT_SUCCESS; 
}
