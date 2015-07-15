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
#include <sys/time.h>

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

// get the iterator of last element of array which is not equal to '*'
int getLastLetter(char *letters, int n){ // -1 game finished!
        int i = n-1;
        while(i>=0 && letters[i]=='*') i--;
        return i;

}

// get random card from game->letters and sort letters array.
char getNewCard(struct game *g){
        char c = 'z' ;
        time_t t;
        int n = getLastLetter(g->letters,LETTERSSZ-1 );
        if(n){
                srand((unsigned) time(&t));
                int r = rand()%n ;
                c = g->letters[r] ;
                g->letters[r] = g->letters[n];
                g->letters[n] = '*' ;
        }
        else {
                c = g->letters[n] ;
                g->letters[0] = '*';
        }
        return c ;
}

// recive struct game via socket 
void recvGame( int sockfd, struct game * daGame ){
        int n, i ;
        char buffer[23] ;

        fd_set readfd;
        FD_ZERO(&readfd);
        FD_SET(sockfd, &readfd);

        struct timeval tv;
        tv.tv_sec = TIMEOUT_SEC ;  
        tv.tv_usec = TIMEOUT_USEC;
	n= select(sockfd+1, &readfd, NULL, NULL, &tv); 
	if(n==0){
		daGame->finished= -1; 
	}
	else{

	        bzero(daGame->letters , LETTERSSZ );
		n = read(sockfd, daGame->letters , LETTERSSZ  );	
		if (n < 0)  ERR("ERROR reading from socket");
		 
       	 	for(i =0;i<5;i++){
       		        bzero(daGame->board[i],6);
               	 	if( ( n=read(sockfd,daGame->board[i],6 ) ) < 0 ) ERR("Error: reading from socket");
		}

        	bzero(buffer, BUFTOREAD);
        	n = read(sockfd, buffer , BUFTOREAD );
        	if (n < 0) ERR("ERROR reading from socket");
        	daGame->turn = atoi(buffer) ;

        	bzero(buffer, BUFTOREAD);
        	n = read(sockfd, buffer ,  BUFTOREAD );
        	if (n < 0)
               		ERR("ERROR reading from socket");
        	daGame->op = atoi(buffer) ;

        	bzero(daGame->p1 , HANDSZ );
        	n = read(sockfd, daGame->p1 , HANDSZ );
        	if (n < 0)  ERR("ERROR reading from socket");

        	bzero(daGame->p2 , HANDSZ );
        	n = read(sockfd, daGame->p2 , HANDSZ );
        	if (n < 0)  ERR("ERROR reading from socket");

	       	bzero(buffer , BUFTOREAD );
	        n = read(sockfd, buffer , BUFTOREAD );
        	if (n < 0)  ERR("ERROR reading from socket");
		daGame->finished = atoi(buffer);

        	bzero(buffer , BUFTOREAD );
        	n = read(sockfd, buffer , BUFTOREAD );
        	if (n < 0)  ERR("ERROR reading from socket");
        	daGame->score1 = atoi(buffer);

        	bzero(buffer , BUFTOREAD );
        	n = read(sockfd, buffer , BUFTOREAD );
        	if (n < 0)  ERR("ERROR reading from socket");
        	daGame->score2 = atoi(buffer);
	
	}
}

// Send struct game by socket 
void sendGame( int sd , struct game daGame){
        int n, i  ;
        n = write(sd,daGame.letters , LETTERSSZ );
        if (n <= 0) ERR("ERROR writing to socket");

        for(i =0;i<5;i++){
                n = write(sd,daGame.board[i] , 6 );
                if (n <= 0) ERR("ERROR writing to socket");

        }
        char  sendline[BUFTOREAD] ;
        snprintf(sendline, BUFTOREAD , "%d", daGame.turn  );
        n = write(sd,sendline, sizeof(sendline));
                if (n <= 0) ERR("ERROR writing to socket");
	
        snprintf(sendline, BUFTOREAD , "%d", daGame.op  );
        n = write(sd,sendline, sizeof(sendline));
                if (n <= 0) ERR("ERROR writing to socket");


        n = write(sd,daGame.p1 , HANDSZ );
        if (n <= 0) ERR("ERROR writing to socket");
	
        n = write(sd,daGame.p2 , HANDSZ );
        if (n <= 0) ERR("ERROR writing to socket");


        snprintf(sendline, BUFTOREAD , "%d", daGame.finished  );
        n = write(sd,sendline, sizeof(sendline));
                if (n <= 0) ERR("ERROR writing to socket");

        snprintf(sendline, BUFTOREAD , "%d", daGame.score1  );
        n = write(sd,sendline, sizeof(sendline));
                if (n <= 0) ERR("ERROR writing to socket");	
        snprintf(sendline, BUFTOREAD , "%d", daGame.score2  );
        n = write(sd,sendline, sizeof(sendline));
                if (n <= 0) ERR("ERROR writing to socket");
}

// print struct Game 
void printGame(FILE *f, struct game g ){
        int i;
	
        fprintf(f,"\nLetters: %s\n", g.letters );
	fprintf(f,"    12345\n");
	fprintf(f,"---------\n");
        for(i =0;i<5;i++) fprintf(f,"[%c] %s\n", 'a'+i ,g.board[i]) ;

	fprintf(f,"Player1(Boss) Score: %d\n" , g.score1) ;
	fprintf(f,"Player2(2nd peer) Score: %d\n\n" , g.score2) ;
	//printf("[Player 1] %s\n", g.p1) ; 
	//printf("[Player 2] %s\n", g.p2) ;
        //fprintf(f,"Turn: %d\nOpponent: %d\nFinished: %d\n" , g.turn , g.op, g.finished );
}


// get random cards for both players, it will fill p1,p2 arrays with random letter from game->Letters
void initHands( struct game *g){
        int i;
        for(i=0;i<HANDSZ-1;i++){
                g->p1[i] = getNewCard(g) ;
                g->p2[i] = getNewCard(g) ;
        }
        g->p1[5] = '\0';
        g->p2[5] = '\0';
}

// initialization of struct game 
void initGame( struct game *g){
        int i ;
        g->op = -1 ;
        g->finished = 0 ;
        for(i = 0 ;i<LETTERSSZ-1;i++)
                g->letters[i] = 'A' + i ;
        g->letters[i] = '\0';
        for(i=0;i<5;i++){
                int j ;
                for(j=0;j<5;j++)
                        g->board[i][j] = '*' ;
                g->board[i][5] = '\0' ;
        }
        g->score1= 0 ;
        g->score2= 0 ;
        initHands(g);
	for(i = 0;i< STEPS_NUM ;i++) g->steps[i][0] = ' '; 
}


