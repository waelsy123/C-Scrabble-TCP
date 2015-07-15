// Client.c  

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
#include <stdbool.h>
#include <signal.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "scrlib.h"
#include <semaphore.h> 

volatile bool boss = false ; // Declare who is the main Player.

bool isItBigger( char a , char b ){
	return a > b ; 
}

// Calculate the maximum score for one specific cell by vertical way in its column 
int getScore2( struct game * daGame , int row , int col){
	int tot = 1 ;  
	int max = 1 ; 
	int i ;
	bool key = false ; 
	bool bigger = true ;  
	 
	bigger = isItBigger(daGame->board[1][col] , daGame->board[0][col] ) ;

	for(i=1;i< 5;i++){
		if(row < i  && daGame->board[i][col] == '*'  ) break; 
		if(daGame->board[i][col] == '*'  && !key) {
			tot = 1 ;
			continue; 
		}
                else if(daGame->board[i][col] == '*' && key) {
                        if(tot > max ) 
				max = tot ;
			return max ; 
                }

		if(daGame->board[i-1][col] != '*' && isItBigger(daGame->board[i][col] , daGame->board[i-1][col] ) ==  bigger )  {
			tot++ ; 
			}

		else{	
			if(key) if(tot>max) max=tot; 
			tot = 1 ; 
			if(daGame->board[i-1][col] >= 'A' && daGame->board[i-1][col] <= 'Y'){ // != '*'
				tot++ ;
				bigger = isItBigger(daGame->board[i][col] , daGame->board[i-1][col] ) ;
			}
			
		}
		if(row <= i ) key = true ;
 
	}
	if(tot> max) return tot;
	return max ; 

}

// Calculate the maximum score for one specific cell by horizontal way in its row. 
int getScore( struct game * daGame , int row , int col){
	int tot = 1 ;  
	int max = 1 ; 
	int i  ;
	bool key = false ; 
	bool bigger = true ;  
	 
	bigger = isItBigger(daGame->board[row][1] , daGame->board[row][0] ) ;

	for(i=1;i< 5;i++){
		if(col < i  && daGame->board[row][i] == '*'  ) break; 
		if(daGame->board[row][i] == '*'  && !key) {
			tot = 1 ;
			//i++;
			//if(daGame->board[row][i] != '*' ) tot++ ;
			continue; 
		}
                else if(daGame->board[row][i] == '*' && key) {
                        if(tot > max ) 
				max = tot ;
			return max ; 
                }

		if(daGame->board[row][i-1] != '*' && isItBigger(daGame->board[row][i] , daGame->board[row][i-1] ) ==  bigger )  {
			tot++ ; 
			}

		else{	
			if(key) if(tot>max) max=tot; 
			tot = 1 ; 
			if(daGame->board[row][i-1] >= 'A' && daGame->board[row][i-1] <= 'Y'){ // != '*'
				tot++ ;
				bigger = isItBigger(daGame->board[row][i] , daGame->board[row][i-1] ) ;
			}
			
		}
		if(col <= i ) key = true ;
 
	}
	if(tot> max) return tot;
	return max ; 

}
// select the position and the new card from their hand, if the move is correct new rand-
// om card will be replaced from Letters array 
void doMove(int sd, struct game * daGame, int id, char* mov ){
	char s[LINESZ] ; 
	int flag = 0 ;  
	if( getLastLetter(daGame->letters, LETTERSSZ-1) == (LETTERSSZ - (2*HANDSZ)) ) boss = true ; 
	time_t st = time(NULL) ;
	// if the input is wrong fomatted ask again. flag=1 correct move.
	while(!flag){
		char *p ;
		int * scorePtr; 
		if(boss ){
			 p = daGame->p1 ;
			 scorePtr = &daGame->score1 ;
		}
		else{
			 p = daGame->p2 ; 
			 scorePtr = &daGame->score2 ;
		}
		printf("Hand:%s\n" ,  p) ;	
		printf("\nMake a move, Row letter should be in small letter, and Card should be in capital letters. Ex: e3=M\nMove: ") ; 
		scanf("%s", s  );
		time_t en = time(NULL) ;
		
		if(en-st >= TIMEOUT_SEC ){
			printf("TIME IS OUT! You disconnected and Lost!\n");	
			daGame->finished= -1; 
			return;
		}
		 
		int one = s[0]-'a' ; // row 
		int two = s[1]-'1' ; // col 
		int i;
		for(i=0;i<5;i++){
			if(p[i]==s[3] ) break; 
		}
		if( (one >= 0 && one < 5 ) && (two  >= 0 && two < 5 ) && (daGame->board[one][two] == '*')   && ( i!=5 ) &&
			( p[i] != '*'  )&& ( 
			(
				( one-1 >=0 && daGame->board[one-1][two]!='*') || 
				( two-1 >=0 && daGame->board[one][two-1]!='*') || 
				( one+1 <5 && daGame->board[one+1][two]!='*') || 
				( two+1 <5 && daGame->board[one][two+1]!='*') 
			) || getLastLetter(daGame->letters, LETTERSSZ-1) == 14 )    
			){
			
			flag = 1 ;
			p[i] = '*' ;
			if( getLastLetter(daGame->letters, LETTERSSZ-1) != -1) p[i] = getNewCard(daGame) ;
			daGame->board[one][two] = s[3] ; 
			int score = getScore(daGame, one , two ) ; 
			int score2 = getScore2(daGame, one , two) ; 
			if(score < score2) score = score2 ; 
			*scorePtr += score ; 
			printf("Total score so far: %d\n", *scorePtr );
			// move array to be send the server then it will be stored in steps[]	
			mov[0] = s[0] ;
			mov[1] = s[1] ;
			mov[2] = s[2] ;
			mov[3] = s[3] ;
			mov[4] = '\0' ;
		}
		else{
			printf("wrong input! try again..\n") ; 
		}
	}
}
 
int main(int argc, char** argv) {
    	int sockfd, portno, n;
    	struct sockaddr_in serv_addr;
    	struct hostent *server;
    	struct game myGame ; 
    	char buffer[LINESZ];
    	if (argc < 3){
		printf("usage  hostname port\n");
		return EXIT_SUCCESS;   
	}
    	if(sethandler( SIG_IGN ,SIGPIPE)) ERR("Seting SIGPIPE:");

    	portno = atoi(argv[2]);
    	sockfd = socket(AF_INET, SOCK_STREAM, 0);
   	if (sockfd < 0) ERR("ERROR opening socket");
    	server = gethostbyname(argv[1]);
    	if (server == NULL)  ERR("ERROR, no such host\n");
        	
   	bzero((char *) &serv_addr, sizeof(serv_addr));
    	serv_addr.sin_family = AF_INET;
    	bcopy((char *)server->h_addr, 
    	        (char *)&serv_addr.sin_addr.s_addr,
    	        server->h_length);
    	serv_addr.sin_port = htons(portno);
    	if (connect(sockfd,&serv_addr,sizeof(serv_addr)) < 0)  ERR("ERROR connecting");
	
	// Receive My ID 
        bzero(buffer,LINESZ);
	n = read(sockfd,buffer,LINESZ);
	if (n < 0) ERR("ERROR reading from socket");
	printf("my id is:%s\n",buffer);
	int id = atoi(buffer) ; 

	bool again = true; 
	while(again){
  		char buffer[6] ;
                bzero(buffer,6);
                n = read(sockfd, buffer , 6 );
                if (n < 0) ERR("ERROR reading from socket");
                n = write(sockfd,buffer , 6 );
                if (n <= 0) ERR("ERROR writing to socket");
		initGame(&myGame); 
		
		// After Receiving the game. check if opponent disconnected(finised=-1), make a move and send back the struct  
		for(;;){
			recvGame(sockfd, &myGame ); 		
			if(myGame.finished !=0 ) break ; 
			printGame(stdout, myGame);
			char mov[MOVESZ] ;
			doMove(sockfd, &myGame, id, mov) ;  
			if(myGame.finished !=0) break ;

			// check if the game normally finished.
                        if( getLastLetter(myGame.letters, LETTERSSZ-1) == -1 &&
                            getLastLetter(myGame.p1     , HANDSZ-1 ) == -1 && getLastLetter(myGame.p2 ,HANDSZ-1)== -1 )
                	                myGame.finished = 1;
			sendGame(sockfd ,  myGame ) ;

			// After sending the game, we send the move..	
                        int n = write( sockfd , mov , MOVESZ );
                        if (n <= 0) ERR("ERROR writing to socket");
			if(myGame.finished == 1) break ;
		}
		printGame(stdout, myGame);
		if(myGame.finished ==1 ) { 
			printf("Game Finished\n") ;  
			
		}
		else{
			printf("Game Unresolved!! You won!\n");   	
		}
		char c[2]= "r";
		while(c[0]!='n' && c[0]!='y'){
			printf("Do you want to play again(y/n): "); 
			scanf("%s", c);
		}
		if(c[0]=='y'){
			 n = write(sockfd, "1" , 2  );
       			 if (n <= 0) ERR("ERROR writing to socket");

		}
		else{
		 	again = false ;
                        n = write(sockfd, "0" , 2  );
                        if (n <= 0) ERR("ERROR writing to socket");
		}
	}

		
	if(TEMP_FAILURE_RETRY(close(sockfd))<0)ERR("close");
	return EXIT_SUCCESS;
}
