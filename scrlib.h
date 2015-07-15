#ifndef SCRLIB_H
#define SCRLIB_H
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))

#define HERR(source) (fprintf(stderr,"%s(%d) at %s:%d\n",source,h_errno,__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))

//#define READ_SOCKET(socket, buffer, length) (!((n = read(sockfd,daGame->board[i],6) )<0)?: ERR("ERROR reading from socket"))

#define SHMSZ 27
#define LETTERSSZ 26
#define BUFTOREAD 22 
#define HANDSZ 6 // including '\0'
#define LINESZ 222 
#define MOVESZ 5
#define TIMEOUT_SEC 60
#define TIMEOUT_USEC 0
#define CLIENTS_NUM 100
#define KEY_SHM 2000
#define KEY_GAME 6000
#define KEY_OF_KEY_GAME 5000
#define BOARD_ROWS 5
#define BOARD_COLS 5
#define STEPS_NUM (BOARD_COLS * BOARD_ROWS)

struct game{
        char letters[LETTERSSZ]; 
        char board[BOARD_ROWS][BOARD_COLS+1]; 
        int turn ; 
        int op ;
	char p1[HANDSZ];
	char p2[HANDSZ];
	int finished ;
	int score1 ; 
	int score2 ;  
	int ip2;
	char steps[STEPS_NUM][MOVESZ] ;
 
};

int sethandler( void (*f)(int), int sigNo);
int getLastLetter(char *letters, int n);
char getNewCard(struct game *g);
void recvGame( int sockfd, struct game* daGame ) ;
void sendGame( int sd , struct game daGame) ;
void printGame(FILE *f, struct game g ) ; 
void initHands( struct game *g);
void initGame( struct game *g);

#endif 
