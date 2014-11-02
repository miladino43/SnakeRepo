
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

//Libraries for push buttons
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../../user-modules/led_driver/led_ioctl.h"
//--------------------

/*Must be less than 1 second for this initial setup*/
#define SLEEP_TIME 0.08 * 1000000000 //originally 0.05

#define FOOD 0x0
#define SNAKE 0x1
#define SNAKE_HEAD 0x2
#define OBSTICAL 0x3
#define EMPTY 0x4

enum direction {NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3};

/*If colour output is desired the following escape sequences can be used to change the colour*/
enum colours { BLACK, RED, GREEN, YELLOW, BLUE, CYAN, PURPLE, WHITE};
static const char FG[8][8] ={"\e[0;30m","\e[0;31m","\e[0;32m","\e[0;33m","\e[0;34m","\e[0;36m","\e[0;35m","\e[0;37m"};
static const char BG[8][7] ={"\e[40m","\e[41m","\e[42m","\e[43m","\e[44m","\e[46m","\e[45m","\e[48m"};


#define TRUE 1
#define FALSE 0

#define MAX(a,b) (((a) >= (b)) ? (a) : (b))
#define MIN(a,b) (((a) <= (b)) ? (a) : (b))


#define ROWS 22
#define COLS 80

FILE *gamelog; 
/******************************************************************************
 *               macros for manipulating the console and cursor               *
 ******************************************************************************/

#define CLEAR_SCREEN fputs("\e[2J",stdout)

/*Could be optimized with your own function to create a string with the row and 
  column positions and then use fputs to print*/
#define MOVE_CURSOR(r,c) printf("\e[%d;%dH", r+2, c+1)

#define HIDE_CURSOR fputs("\e[?25l",stdout)
#define SHOW_CURSOR fputs("\e[?25h",stdout)

#define SAVE_CURSOR fputs("\e[s",stdout)
#define RESTORE_CURSOR fputs("\e[u",stdout)

#define SET_BG_COLOR(c) fputs(BG[c],stdout)
#define SET_FG_COLOR(c) fputs(FG[c],stdout)

/*Switches to an extended character support mode*/
#define ENTER_LINE_MODE fputs("\e(0",stdout)
#define LEAVE_LINE_MODE fputs("\e(B",stdout)
#define DIAMOND 96

#define MAX_FOOD_VALUE 5

typedef struct Tile Tile;

typedef struct Snake Snake;

typedef struct {
	int c;
	int r;
} Point;

//Stuff for push buttons
struct led_ioctl_data data;
unsigned long value;
int fd;
//--------------------------

struct Tile{
	Point p;
	int state;
	unsigned int food_value;
	char string[14];
	Tile *neighbour[4];
	Tile *next;
};

long int random_seed;
Tile tile[ROWS][COLS];
Point head, tail;


int running = 0;// variable for checking to see if the game is in progress if True game is running
int score = 0;// holds the total score of the current game
int food_count = 0;//number of food tiles on the game screen
int food_move = 0; //for holding food value in oeder to update score
int block_count = 0;//number of obsticals on the screen
int pb_dir;

/******************************************************************************
 *                   variables for handling keyboard logic                    *
 ******************************************************************************/
#define NUM_KEYS 4
#define KEY_WASD 0
#define KEY_Q 1
#define KEY_P 2
#define KEY_ENTER 3


int key;
volatile int key_received;
volatile int key_wasd_received;
volatile int key_pressed[NUM_KEYS];

static void init_snake();
static void init_tile(Tile *t, int state, Tile **neighbour, int r, int c);
static void update_graphic_string(Tile *t, int state);
static void update_score(int food_v);

static void generate_block();
static int go_east(int dir);
static void turn_right();
static void pushbutton_handler(); 

static void init_tile(Tile *t, int state, Tile **neighbour, int r, int c) {
	t->p.r = r;
	t->p.c = c;

	t->neighbour[NORTH] = neighbour[NORTH];
	t->neighbour[EAST]  = neighbour[EAST];
	t->neighbour[SOUTH] = neighbour[SOUTH];
	t->neighbour[WEST]  = neighbour[WEST];

	t->state = state;

	t->next = NULL;

	update_graphic_string(t, state);

	t->string[13] = '\0';
}
/************************************************************
 *This function gives the tile a symbol and color depending *
 *on the state of the tile.                                 *
 *                                                          *
 ***********************************************************/
static void update_graphic_string(Tile *t, int state) {

	int bold;
	int fg, bg;
	char symbol;
	t->state = state;

	switch (t->state) {
		case OBSTICAL:
			bg = YELLOW;
			fg = BLACK;
			symbol = ' ';
			bold = 0;
			break;
		case FOOD:
			bg = WHITE;
			fg = t->food_value;
			symbol = DIAMOND;//0x30 | t->food_value;
			bold = 0;
			break;
		case SNAKE:
			bg = WHITE;
			fg = CYAN;
			symbol = 'O';
			bold = 0;
			break;
		case SNAKE_HEAD:
			bg = WHITE;
			fg = CYAN;
			symbol = '@';
			bold = 1;
			break;
		case EMPTY:
			bg = WHITE;
			fg = BLACK;
			symbol = ' ';
			bold = 0;
			break;
		default:
			bg = WHITE;
			fg = YELLOW;
			symbol = 'E';
			bold = 0;
	}

	memcpy(&t->string[0], &FG[fg], 7);
	memcpy(&t->string[7], &BG[bg], 5);

	if(bold)
		t->string[2] = '1';

	t->string[12] = symbol;
}
/************************************************************
 *This function places the contents of string[0] of tile t   *
 *on the point located at tiles p.r and p.c                 *
 ***********************************************************/

static void printGraphic(Tile* t) {
	MOVE_CURSOR(t->p.r, t->p.c);
	fputs((const char*)&t->string[0], stdout);
}

/************************************************************
 *This function makes the blue header and footer on the     *
 *game screen                                               *
 ***********************************************************/
static void printField() {
	int i;
	int j;

	SET_BG_COLOR(BLUE);
	MOVE_CURSOR(-1,0);
	for (j=0; j < COLS; j++) {
		putchar(' ');
	}

	for (i=0; i < ROWS; i++) {
		for (j=0; j < COLS; j++) {
			printGraphic(&tile[i][j]);
		}
	}

	SET_BG_COLOR(BLUE);
	MOVE_CURSOR(ROWS,0);
	for (j=0; j < COLS; j++) {
		putchar(' ');
	}
}
/************************************************************
 *This function produces food on a random location using    *
 *rand().using update_graphic it produces diammond shape    *
 *and using printgraphic places it on screen                *
 ***********************************************************/
static void generate_food() {
	Point food;

	do {
		food.r = rand() % ROWS;
		food.c = rand() % COLS;
	}while(tile[food.r][food.c].state != EMPTY);

	tile[food.r][food.c].food_value = 1 + (rand() % MAX_FOOD_VALUE);
	update_graphic_string(&tile[food.r][food.c], FOOD);
	printGraphic(&tile[food.r][food.c]);
 	food_count++;

}
/************************************************************
 *This function places snake on random location and makes   *
 *the tail point to body and makes body point to head. The  *
 *is checked to be in EMPTY state before placement          *
 ***********************************************************/
static void init_snake() {
	Point snakey;
	
	do {
		snakey.r = rand() % ROWS;
		snakey.c = rand() % COLS;
	// Checks if the neighbours are empty before making the snake
	}while(tile[snakey.r][snakey.c].state != EMPTY && tile[snakey.r][snakey.c-1].state != EMPTY && tile[snakey.r][snakey.c-2].state != EMPTY && 		snakey.c > 1);

	//Point to head and tail
	//tailpoint -> bodypoint 
	tile[snakey.r][snakey.c-2].next = &tile[snakey.r][snakey.c-1];
	//bodypoint -> headpoint 
	tile[snakey.r][snakey.c-1].next = &tile[snakey.r][snakey.c];	

	head = tile[snakey.r][snakey.c].p;
	tail = tile[snakey.r][snakey.c-2].p;

	update_graphic_string(&tile[snakey.r][snakey.c], SNAKE_HEAD);
	update_graphic_string(&tile[snakey.r][snakey.c-1], SNAKE);
	update_graphic_string(&tile[snakey.r][snakey.c-2], SNAKE);
	printGraphic(&tile[snakey.r][snakey.c]);
	printGraphic(&tile[snakey.r][snakey.c-1]);
	printGraphic(&tile[snakey.r][snakey.c-2]);

}

/******************************************************************
AI
*******************************************************************/

//This part of the code implements the "TURN RIGHT" AI
	

static void turn_right()
{
	int i;
	int dir;
	
	for (i = 0; i < 3; i++)
	{
		int butt = (tile[head.r][head.c].neighbour[dir])->state;
		if(butt == EMPTY || butt == FOOD)
			break;
			dir = dir+1;
			dir = dir%4;
	}
}	
	


/******************************************************************* 
*Snake Updating Function.                                          *
*	input: int dir                                             *
*	output: none                                               *
*This function moves the snake head tile and the tail tile.        *


*If the snake eats food,The tail remains in the same position      *
* and the score is updated. If the snake hit itself or an          *
* obstacle, the game ends and a "GAME OVER" message is displayed.  *
*******************************************************************/
static int update_snake(int dir, int go_east) {

	Point origHead = head;
	Point origTail = tail;	
	char gameOver[10] = "GAME OVER!";
	int i;
	int butt;
//This part of the code implements the "GO EAST" AI


//This part of the code implements the "GREEDY" AI
/*	if(go_east == 1) {

		int r = head.r;
		for (i = 0; i<ROWS-r; i++){
			
			if (tile[head.r+i][head.c].state == FOOD)
				{					
				dir = SOUTH;
				//if (tile[head.r][head.c].next == OBSTICAL)
				//	break;
				}	
		}
*/

	//move SNAKE_HEAD according to direction
	head = (tile[head.r][head.c].neighbour[dir])->p;

	//if snake eats food, store food value and update score	
	if (tile[head.r][head.c].state == FOOD) {
		food_move += tile[head.r][head.c].food_value;
		food_count--;
		update_score(food_move);
	}
	//if snake hits obstical or itself, display GAME OVER and end game
	else if (tile[head.r][head.c].state == SNAKE | tile[head.r][head.c].state == SNAKE_HEAD | tile[head.r][head.c].state == OBSTICAL) {
	fprintf(gamelog, "Death from Block!");
	MOVE_CURSOR(9,34);
	SET_FG_COLOR(BLUE);
	for (i=0; i<10 ; i++)
		putchar(gameOver[i]);
	running = FALSE;
	}
	
	//grow if food_move is zero, move tail
	if (food_move == 0) {
	  tail = (tile[origTail.r][origTail.c].next)->p;
	  tile[origTail.r][origTail.c].next = NULL;
				}	
	//if food_move is zero, don't move tail, decrement food_move
	else 
		food_move--;

	//update pointer to snake head
	tile[origHead.r][origHead.c].next = &tile[head.r][head.c];
	
	//update and display head, old tail and new tail tiles
	update_graphic_string(&tile[origHead.r][origHead.c], SNAKE);
	update_graphic_string(&tile[head.r][head.c], SNAKE_HEAD);
	update_graphic_string(&tile[origTail.r][origTail.c], EMPTY);
	printGraphic(&tile[origHead.r][origHead.c]);
	printGraphic(&tile[head.r][head.c]);
	printGraphic(&tile[origTail.r][origTail.c]);

	return 0;
}

/************************************************************
 *This function Produces OBSTICALS on the screen and places *
 *Each Randomly on the screen.Two OBSTICAL Sizes available  *
 *                                                          *
 ***********************************************************/
static void generate_block() {
        
        Point block;

                
                block.r = rand() % ROWS;
                block.c = rand() % COLS;

        int random_size = rand() % 2;

        switch (random_size) {
                case 0:
                        update_graphic_string(&tile[block.r][block.c], OBSTICAL);
                        printGraphic(&tile[block.r][block.c]);
                        block_count++;
                        break;
                case 1:
                        update_graphic_string(&tile[block.r][block.c], OBSTICAL);
                        printGraphic(&tile[block.r][block.c]);
                        
                        update_graphic_string(tile[block.r][block.c].neighbour[NORTH], OBSTICAL);
                        printGraphic(tile[block.r][block.c].neighbour[NORTH]);

                        update_graphic_string(tile[block.r][block.c].neighbour[EAST], OBSTICAL);
                        printGraphic(tile[block.r][block.c].neighbour[EAST]);
                        
                        Point newblock = (tile[block.r][block.c].neighbour[NORTH]) ->p;
                        update_graphic_string(tile[newblock.r][newblock.c].neighbour[EAST], OBSTICAL);     
                        printGraphic(tile[newblock.r][newblock.c].neighbour[EAST]);

                        block_count += 4;
                        break;
                default:
                        break;
        }        
// function that will check for clusters with holes and fix the problem
}         

/*******************************************************************
* Score updating and displaying function.                          *
*	input: int food_v                                          *
*	output: none                                               *
*This function takes the food_v and adds it to the existing score, *
*and then displays the updated score in the upper left corner in   *
*white.                                                            *
********************************************************************/
static void update_score(int food_v) {
	char scoreStr[10] = "SCORE: ";
	char sc_rep[4] = "";
	char outputStr[15] = "";
	short sc = 0;//short int to store score in order to put into char array
	int i = 0;	

	score = score + food_v;
	sc = (short)score;
	snprintf( sc_rep, 4, "%d", sc ); 
		
	//Update score
	for (i=0; i < 4; i++)	
		scoreStr[i+7] = sc_rep[i];

	//Display score	
	MOVE_CURSOR(-1,5);
		
	memcpy(&outputStr[0], &FG[WHITE], 7);
	memcpy(&outputStr[7], &BG[BLUE], 5);
	outputStr[2] = '1';
	
	for (i=0; i < 10; i++) {
		
		
		outputStr[12] = scoreStr[i];

		fputs((const char*)&outputStr, stdout);
	}

}

/* Standard-in interrupt handler function.
	As the game updates at a variable rate that could be quite slow there
	could be many key presses between game logic updates.  By processing key
	presses asynchronously we can be assured that we never miss a key press and
	that we always detect the 'quit' key.  You can think of this as a hard
	requirement where not responding to the quit signal would be considered a
	failure.

	WASD keys are grouped together as we only care what the latest one was that
	was pressed as their affect is mutually exclusive as the snake can only move
	in one direction at a time.
*/
void keyboard_handler (int signum) {
	key = getchar();
	if(key != EOF) {
		key_received = TRUE;

		//Buffer latest direction and control keys
		switch (key) {
			case 'w':
				key_pressed[KEY_WASD] = NORTH;
				key_wasd_received = TRUE;
				break;
			case 's':
				key_pressed[KEY_WASD] = SOUTH;
				key_wasd_received = TRUE;
				break;
			case 'a':
				key_pressed[KEY_WASD] = WEST;
				key_wasd_received = TRUE;
				break;
			case 'd':
				key_pressed[KEY_WASD] = EAST;
				key_wasd_received = TRUE;
				break;
			case 'q':
			case 'Q':
			  fprintf(gamelog, "%s", "\nUser Quit\n");
				key_pressed[KEY_Q] = TRUE;
				break;
			case 'p':
			case 'P':
			  
				key_pressed[KEY_P] = TRUE;
				break;
			case '\n':
				key_pressed[KEY_ENTER] = TRUE;
				break;
			default:
				break;
		}
	}
}

static void pushbutton_handler() {

	int key_2;	

	fd = open("/dev/pb_driver",O_RDWR); 
	value = 0xFFFFFFFF;
	data.offset = 4;
	data.data = value;
	ioctl(fd, LED_WRITE_REG, &data);	

	data.offset = 0;
	ioctl(fd, LED_READ_REG, &data);
	//printf("The register is reading: %d \n",data.data);

	key_2 = data.data;
	
		if (key_2 == 16)
			pb_dir = NORTH;
		if (key_2 == 4)
			pb_dir = SOUTH;
		if (key_2 == 8)
			pb_dir = WEST;
		if (key_2 == 2) 
			pb_dir = EAST;
}


/* Cleanup function to be called, if possible, whenever program quits.  If this
	function is not called the terminal might not respond to inputs correctly or
	the screen could be garbled due to not leaving line mode and, (at least on
	the petalinux system) this would require a restart to fix.
*/
void cleanup_handler (int signum) {
	SET_BG_COLOR(BLACK);
	SET_FG_COLOR(WHITE);
	//Exit line art mode
	LEAVE_LINE_MODE;
	MOVE_CURSOR(ROWS+1,0);
	SHOW_CURSOR;
	fclose(gamelog); 
	rewind(stdout);
	ftruncate(1,0);//set size of stdout to 0 bytes
	system("stty echo icanon");
	LEAVE_LINE_MODE;
	exit(0);
}

int main (int argc, char **argv)
{

	int i = 0;
	int j = 0;
	int dir = EAST;
	int go_east = 0;

	

        if (argc == 1)
                {
                gamelog = fopen("snake.log", "w");
                fprintf(gamelog,"%s","\nDefault Log file Created: snake.log\n");
                }
        else 
                {
                gamelog = fopen(argv[1], "w"); //open the file with name argv[1]
                fprintf(gamelog, "\nLog File Created by User: %s\n", argv[1]);
                }
        
        if (argc == 3)
                {
                random_seed = atoi(argv[2]);
                fprintf(gamelog, "Random Seed Changed to %s\n", argv[2]);
                
                }
        else 
                random_seed = time(0);

        //fprintf(gamelog, "\nargc = %d\n", argc);




Tile *neighbour[4] = {NULL, NULL, NULL, NULL};
	
	/* Initialize neighbour list for torus */
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			neighbour[NORTH] = &tile[(i-1) < 0 ? ROWS-1 : i-1][j];
			neighbour[EAST]  = &tile[i][(j+1) > COLS-1 ? 0 : j+1];
			neighbour[SOUTH] = &tile[(i+1) > ROWS-1 ? 0 : i+1][j];
			neighbour[WEST]  = &tile[i][(j-1) < 0 ? COLS-1 : j-1];

			init_tile(&tile[i][j], EMPTY, neighbour, i, j);
		}
	}



	

	CLEAR_SCREEN;

	SET_BG_COLOR(WHITE);
	SET_FG_COLOR(BLACK);

	srand(random_seed);
	init_snake();
	generate_food();

	/*run man stty to understand what these options set*/
	system("stty -echo -icanon");//-echo stops the keyboard input from showing up on the terminal
	//Enter line art mode
	ENTER_LINE_MODE;
	HIDE_CURSOR;

	printField();
	fflush(stdout);

	running = TRUE;
	score = 0;

	/*Setup standard in to be asynchronous and owned by this process*/
	fcntl(STDIN_FILENO, F_SETOWN, getpid());
	fcntl(STDIN_FILENO, F_SETFL, O_ASYNC | O_NONBLOCK);

	/*setting standard in buffer to zero*/
	//disable stdin buffering
	setbuf(stdin, NULL);

	/*Quirk of the terminal causes stdout to misbehave when standard in buffer
	  set to zero.  Setting standard out buffpart1_integrated part1_integrated.c
er to be larger than contents
	  written before a flush solves this*/
	/*specifies buffer for stdout,since null a buffer is automatically assigned of size 1024 bytes,_IOFBF means full buffering*/
	setvbuf(stdout, NULL, _IOFBF, 1024);

	/*Register handler for SIGIO*/
	signal (SIGIO, keyboard_handler);

	/*Register the termination signal and the control-c signal to the cleanup
	  handler */
	signal (SIGINT, cleanup_handler);
	signal (SIGTERM, cleanup_handler);
	//signal(SIGABRT,cleanup_handler);	

	struct timespec sleep_time;

	key_received = FALSE;
	key_wasd_received = FALSE;


	/*Main game loop:
	  On every iteration inputs should be checked, game logic updated, and screen
	  updated.  Program should then "sleep" until alloted time for a game
	  iteration has passed.*/
	while(running) {
	  	 
		/* Do input processing here */
		if(key_received) {
			if(key_pressed[KEY_Q]) {
			  
				running = FALSE;
				key_pressed[KEY_Q] = FALSE;
			}
			else if (key_pressed[KEY_P]) {
			  // system("read -n1 -r -p \"Press any key to coninue Game\"");
			  
			  sleep(100);
				key_pressed[KEY_P] = FALSE;
			}

			else if (key_pressed[KEY_ENTER]) {
				go_east = 1;
				key_pressed[KEY_ENTER] = FALSE;
			}

			else if (key_pressed[KEY_WASD] == NORTH) {
				if (dir != SOUTH)
					dir = NORTH;
				key_wasd_received = FALSE;
			}

			else if (key_pressed[KEY_WASD] == SOUTH) {
				if (dir != NORTH)
					dir = SOUTH;
				key_wasd_received = FALSE;
			}

			else if (key_pressed[KEY_WASD] == WEST) {
				if (dir != EAST)
					dir = WEST;
				key_wasd_received = FALSE;
			}

			else if (key_pressed[KEY_WASD] == EAST) {
				if (dir != WEST)
					dir = EAST;
				key_wasd_received = FALSE;
			}

			key_received = FALSE;
		}		

		if (running) {
			pushbutton_handler();
			dir = pb_dir;
			update_snake(dir, go_east);
		}

		if(block_count < 50)
		  generate_block();
		
		if((rand() % 100) > 96 && food_count < 15)
		  generate_food();
	
		fflush(stdout);

		sleep_time.tv_nsec = SLEEP_TIME;
		sleep_time.tv_sec = 0;

		while(nanosleep(&sleep_time, &sleep_time) == -1) { }
	}

	fprintf(gamelog, "Final Score: %d\n", score);
	
	cleanup_handler(0);
	return 0;
}
