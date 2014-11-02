/************************************************************************
 * snake.c								*
 *									*
 * Author: Lucie Hiltner, Michael Nguyen, Milad Bonakdar, Sarah Elmasry	*
 *									*
 * Revision History:							*
 *	October 7, 2012 - Initial code					*
 *	October 10, 2012 - Milestone 1 integration			*
 *	October 15, 2012 - Milestone 2 progress				*
 *	October 26, 2012 - Milestone 2 integration			*
 *	November 7, 2012 - Milestone 3 integration			*
 *	November 21, 2012 - Final version complete			*                                
 *                                                  		        *                                
 ************************************************************************/

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

/*Must be less than 1 second for this initial setup*/
#define SLEEP_TIME 0.09 * 1000000000 //originally 0.05

#define FOOD 0x0
#define SNAKE 0x1
#define SNAKE_HEAD 0x2
#define OBSTICAL 0x3
#define EMPTY 0x4
#define NEGFOOD 0x5//!!!

enum direction {NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3};

/*If colour output is desired the following escape sequences can be used to change the colour*/
enum colours { BLACK, RED, GREEN, YELLOW, CYAN, WHITE, BLUE, PURPLE};
static const char FG[8][8] ={"\e[0;30m","\e[0;31m","\e[0;32m","\e[0;33m","\e[0;36m","\e[0;37m","\e[0;34m","\e[0;35m"};
static const char BG[8][7] ={"\e[40m","\e[41m","\e[42m","\e[43m","\e[46m","\e[48m","\e[44m","\e[45m"};


#define TRUE 1
#define FALSE 0

#define MAX(a,b) (((a) >= (b)) ? (a) : (b))
#define MIN(a,b) (((a) <= (b)) ? (a) : (b))

#define MIN_ROWS 4
#define MIN_COLS 4

volatile int ROWS = 12;
volatile int COLS = 40;


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

typedef struct {
	Point start;
	Point end;
} Boundary;

struct Tile{
	Point p;
	int state;
	unsigned int food_value;
	unsigned int neg_food_value;//!!!
	char string[14];
	Tile *neighbour[4];
	// Snake linked list
	Tile *next;
	Tile *prev;//!!!
	// Food linked list
	Tile *fnext;
	Tile *fprev;
	//S hort path variable
	int dist;
};

typedef struct {
	Point head;
	Point tail;
	int score;
	int food_move;
	int food_shrink;//!!!
	int snake_running;
	int dir;
	int length;//!!!
	int func_val;

} SnakeCounter;

long int random_seed=0;
Tile **tile;
Tile *fhead = NULL,*ftail = NULL;
SnakeCounter *counter;
Point **snakeCoordinate = NULL;
Boundary *obst = NULL;
//Point food_coord;

FILE *gamelog; 
FILE *optr;
char *filename = NULL;
int *snake_length = NULL;

int num_obstacles;
static int NUM_SNAKES = 0;
int running = 0;// variable for checking to see if the game is in progress if True game is running
int food_count = 0;//number of food tiles on the game screen
int neg_food_count =0;  //number of negative-valued food tiles on the game screen//!!!!!
int block_count = 0;//number of obsticals on the screen

int keyboard_dir = 1;

/******************************************************************************
 *                   variables for handling keyboard logic                    *
 ******************************************************************************/
#define NUM_KEYS 8
#define KEY_WASD 0
#define KEY_Q 1
#define KEY_P 2
#define KEY_ENTER 3
#define KEY_1 4
#define KEY_2 5
#define KEY_3 6
#define KEY_4 7

int key;
volatile int key_received;
volatile int key_wasd_received;
volatile int key_pressed[NUM_KEYS];

#include "functions.c"

int main (int argc, char *argv[])
{

	int PAUSE = 0;
	int LINES = 0;
	int MAX = 100;
	int width, height;
	int NUM_SNAKES;
	int i, j, k;
	int LineNum = 0;
	int quit;

	// Get configuration file name from command line input	
	if ( argc > 1 ){
		filename = argv[1];
	 	//fprintf(gamelog, "Configuration file name is %s\n", filename);
	}

	// Get log file name from command line input	
        if (argc > 2){
	  gamelog = fopen(argv[2], "w"); //open the file with name argv[1]
	  fprintf(gamelog, "\nLog File Created by User: %s\n", argv[2]);
        }
        else{
	  gamelog = fopen("snake.log", "w");
	  fprintf(gamelog,"%s","\nDefault Log file Created: snake.log\n");
        }
        
        if ( argc > 3 ){
	  random_seed = atoi(argv[3]);
	  fprintf(gamelog, "Random Seed Changed to %s\n", argv[3]);
        }
       else{ 
	  random_seed = time(0);
	  fprintf(gamelog, "Default Random Seed is %ld\n", random_seed);
	
	}
	if (argc > 1) {	
		// Open configuration file  
		if ( (optr = fopen(filename,"r")) == NULL){
		  printf("Unable to open file\n");
		  exit(EXIT_FAILURE);
	 	}
	
		fscanf(optr,"%d%d%d%d\n",&width,&height,&num_obstacles,&NUM_SNAKES); //Read the first four numbers in the config file
	
		// Make sure height and width are acceptable
		if ( height >= MIN_ROWS  && width >= MIN_COLS) {
		  ROWS = height;
		  COLS = width;
		}
		else {
		  fprintf(gamelog, "Error: Game field too small\n Min height = 4\n Min width = 4\n");
		  exit(0);
		}
	
		
		LINES = num_obstacles + NUM_SNAKES;
		char array[LINES][MAX]; //arrray to hold each line from fgets()
		char *start, *end;      //pointers used to indicate the location od the pointer for the strtol()
		int obstacle[num_obstacles][4];	//matrix for storage of all the coordinates of each obstacle
		int snake[NUM_SNAKES][MAX]; //matrix for storage of all the coordinates of each snake

		snake_length = (int *)malloc(NUM_SNAKES*sizeof(int));
		obst = (Boundary *)malloc(num_obstacles*sizeof(Boundary)); //obst is an array of bounndary which has two Points (start and end)

		snakeCoordinate = (Point **)malloc(NUM_SNAKES*sizeof(Point *)); //2D array for the coordinates of each snakes
		if ( snakeCoordinate == NULL)
		  exit(0);
		for (i = 0;i < NUM_SNAKES; i++){
		  snakeCoordinate[i] = (Point *)malloc(MAX*sizeof(Point));
		  if (snakeCoordinate[i] == NULL)
		    exit(0);
		}
	
		for (LineNum = 0; LineNum < LINES; LineNum++){    //go through each line and place the line as char in array
		  if ( fgets(array[LineNum],sizeof(array[LineNum]),optr) == NULL)
		    exit(EXIT_FAILURE); 
	 	}
	
		fclose(optr);
	
		// Convert all obstacle coordinates to LONG INT and Put each obstacle into a struct Boundary
		for ( i = 0; i < num_obstacles; i++ ){
		  start = array[i];		
		  for ( k = 0; k < 4; k++ ){	
		    obstacle[i][k] = strtol(start, &end, 10);
		    start = end;
		  }
		  
		  obst[i].start.r = obstacle[i][0];
		  obst[i].start.c = obstacle[i][1];
		  obst[i].end.r = obstacle[i][2];
		  obst[i].end.c = obstacle[i][3];
		  
		  // Make sure obstacle coordinates are within ROWS and COLS
		  if ( obst[i].start.r >= ROWS || obst[i].end.r >= ROWS || obst[i].start.c >= COLS || obst[i].end.c >= COLS ) {
		    fprintf(gamelog, "Error: Obstacle #%d coordinates out of game field range\n", i+1);
		    exit(0);
		  }
		  // Make sure 2nd coordinate of obstacle boundary is greater than 1st coordinate
		  else if ( obst[i].start.r > obst[i].end.r || obst[i].start.c > obst[i].end.c ){
		    fprintf(gamelog, "Error: Obstacle #%d boundaries are invalid\n", i+1);
		    exit(0);
		  }
		  
		}
	
		// Put each user-specified snake into an array of snake coordinates
		for ( i = 0; i < NUM_SNAKES; i++ ){
		  start = array[i+num_obstacles];
		  int ElementNum = 0;
		  for (start = array[i+num_obstacles];; start = end) {
		    snake[i][ElementNum] = strtol(start, &end, 10);
		    if (start == end)
		      break;
		    ElementNum++;
		  }
		  
		  // Snake array length. 
		  snake_length[i] = ElementNum;
		  if ( snake_length[i] < 4 || snake_length[i] > 100  ||  snake_length[i]%2 == 1){
		    fprintf(gamelog, "Error: Snake #%d must have length between 2 and 50\n\tCoordinates must be given in pairs \n", i+1);
		    exit(0);
		  }		
		  
		  for ( k = 0; k < snake_length[i]; k++){
		    if ( k%2 == 0){		
		      if ( snake[i][k] < ROWS )
			snakeCoordinate[i][k/2].r = snake[i][k];
		      else{
			fprintf(gamelog, "Error: Snake #%d has one or more elements out of game field range\n", i+1);
			exit(0);
		      }
		    }
		    else {
		      if ( snake[i][k] < COLS )
			snakeCoordinate[i][k/2].c = snake[i][k];
		      else{
			fprintf(gamelog, "Error: Snake #%d has one or more elements out of game field range\n", i+1);
			exit(0);
		      }
		    }
		  }
		  // Actual snake length ie. number of coordinates
		  snake_length[i] = ElementNum/2;	
		  
		  // Make sure the snake coordinates are all neighbours
		  for ( k = 0; k < snake_length[i]-1; k++ ){
		    if ( !((snakeCoordinate[i][k].r == snakeCoordinate[i][k+1].r && ((snakeCoordinate[i][k].c == snakeCoordinate[i][k+1].c + 1)				    || (snakeCoordinate[i][k].c == snakeCoordinate[i][k+1].c - 1))) || (snakeCoordinate[i][k].c == snakeCoordinate[i][k+1].c 
			 && ((snakeCoordinate[i][k].r == snakeCoordinate[i][k+1].r + 1) || (snakeCoordinate[i][k].r == snakeCoordinate[i][k+1].r - 1)))))                        {
		      fprintf(gamelog, "Error: Snake #%d has one or more invalid snake coordinates\n", i+1);
		      exit(0);
		    }
		    
		    
		  }
		}
	}
	else
		NUM_SNAKES = 0;
	
	// Dynamically allocate playing field
	tile = (Tile **)malloc(ROWS*sizeof(Tile *));
	if ( tile == NULL)
	  exit(0);
	int NumRows;
	for (i = 0;i < ROWS; i++){
	  tile[i] = (Tile *)malloc(COLS*sizeof(Tile));
	  if (tile[i] == NULL)
	    exit(0);
	}
	
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

	// Generate block
	  if (num_obstacles == 0){
	    while(block_count < 10)
	      generate_block();
	  }
	  else {
	    generate_user_block(obst);
	  }

	
	// Allocate snake counter variables and initialize snakes
	if (NUM_SNAKES == 0) {
		fprintf( gamelog, "NUM_SNAKE = 0\n");
	  	counter = (SnakeCounter *)malloc(sizeof(SnakeCounter));
	  	if (counter == NULL)
			exit(0);
		init_default_snake();
	}	
	else {	
		fprintf( gamelog, "NUM_SNAKE != 0\n");
	  	counter = (SnakeCounter *)malloc(NUM_SNAKES*sizeof(SnakeCounter));
		if (counter == NULL)
			exit(0);
		for (i = 0; i < NUM_SNAKES; i++){
		    init_user_snake(snakeCoordinate[i], snake_length[i], i);
		}
	  
	}
	
	// Log the level configuration. If no configuration was provided, print default field 
	if (argc > 1) {	
		fprintf(gamelog, "Level: %s\n\t Width: %d\n\t Height: %d\n\t Number of Obstacles: %d\n\t Number of Snakes %d\n\n", filename
		, COLS, ROWS, num_obstacles, NUM_SNAKES) ;
	}
	else
		fprintf(gamelog, "Level: Default\n\t Width: %d\n\t Height: %d\n\n", COLS, ROWS);
	
		generate_food();
		generate_negfood();
	
	/*run man stty to understand what these options set*/
	system("stty -echo -icanon");//-echo stops the keyboard input from showing up on the terminal
	//Enter line art mode
	ENTER_LINE_MODE;
	HIDE_CURSOR;
	
	printField();
	fflush(stdout);
	
	running = TRUE;
	quit = FALSE;
	
	/*Setup standard in to be asynchronous and owned by this process*/
	fcntl(STDIN_FILENO, F_SETOWN, getpid());
	fcntl(STDIN_FILENO, F_SETFL, O_ASYNC | O_NONBLOCK);
	
	/*setting standard in buffer to zero*/
	//disable stdin buffering
	setbuf(stdin, NULL);
	
	/*Quirk of the terminal causes stdout to misbehave when standard in buffer
	  set to zero.  Setting standard out buffpart1_integrated part1_integrated.c
	  to be larger than contents
	  written before a flush solves this*/
	/*specifies buffer for stdout,since null a buffer is automatically assigned of size 1024 bytes,_IOFBF means full buffering*/
	setvbuf(stdout, NULL, _IOFBF, 1024);
	
	/*Register handler for SIGIO*/
	signal (SIGIO, keyboard_handler);
	
	/*Register the termination signal and the control-c signal to the cleanup
	  handler */
	signal (SIGINT, cleanup_handler);
	signal (SIGTERM, cleanup_handler);
	
	struct timespec sleep_time;
	
	key_received = FALSE;
	key_wasd_received = FALSE;
	
	/*Main game loop:
	  On every iteration inputs should be checked, game logic updated, and screen
	  updated.  Program should then "sleep" until alloted time for a game
	  iteration has passed.*/
	while(running) {
	
	  	//Do input processing here
		if(key_received) 
		{
	    		if(key_pressed[KEY_Q]) 
			{
	      			quit = TRUE;
	      			key_pressed[KEY_Q] = FALSE;
	    		}
	    
	    		else if (key_pressed[KEY_P]) 
			{
				PAUSE = (PAUSE+1)%2;
	      			key_pressed[KEY_P] = FALSE;
	    		}
			// change func_val depending on number of times key is pressed
	    		else if (key_pressed[KEY_1]) 
			{	
				inc_func_val(0, NUM_SNAKES);
				
	      			key_pressed[KEY_1] = FALSE;
	    		}
	    		else if (key_pressed[KEY_2]) 
			{	
				inc_func_val(1, NUM_SNAKES);
				
	      			key_pressed[KEY_2] = FALSE;
	    		}
			else if (key_pressed[KEY_3]) 
			{	
				inc_func_val(2, NUM_SNAKES);
				
	      			key_pressed[KEY_3] = FALSE;
	    		}
			else if (key_pressed[KEY_4]) 
			{	
				inc_func_val(3, NUM_SNAKES);
				
	      			key_pressed[KEY_4] = FALSE;
	    		}
	    
	    		else if (key_pressed[KEY_ENTER]) 
			{
					
	      			key_pressed[KEY_ENTER] = FALSE;
	    		}
	    
	    		else if (key_pressed[KEY_WASD] == NORTH) 
			{
	      			if (keyboard_dir != SOUTH)
					keyboard_dir = NORTH;
	      			key_wasd_received = FALSE;
	    		}
	    
	    		else if (key_pressed[KEY_WASD] == SOUTH) 
			{
		      		if (keyboard_dir != NORTH)
					keyboard_dir = SOUTH;
	      			key_wasd_received = FALSE;
	    		}
	    
	    		else if (key_pressed[KEY_WASD] == WEST) 
			{
	      			if (keyboard_dir != EAST)
					keyboard_dir = WEST;
	      			key_wasd_received = FALSE;
	    		}
	    
	    		else if (key_pressed[KEY_WASD] == EAST) 
			{
	      			if (keyboard_dir != WEST)
					keyboard_dir = EAST;
	      			key_wasd_received = FALSE;
	    		}
	    		
			key_received = FALSE;
	  	}				

		// Update function AI of each snake
		// Display score and function type 	  
		if (running) 
		{
	    		if (PAUSE == 1) {}
			else {
				if ( NUM_SNAKES == 0 ){
	    				do_func(0);
					update_score(0);
					display_func(0);
				}
				else{
					for (i = 0; i < NUM_SNAKES; i++) {
		    				do_func(i);
						update_score(i);
						display_func(i);
					}
				}
			}
	  	}	

		// Generate food
		if((rand() % 100) > 96 && food_count < 100)
		    	generate_food();

		// Generate negative food
		//if((rand() % 100) > 96 && neg_food_count < 6)//!!!
		  //  	generate_negfood();	//!!!
		
		// Update running variable
		if ( NUM_SNAKES == 0 )
			running = counter[0].snake_running;
		else {		
		    	running = FALSE;			
		    	for (i = 0; i < NUM_SNAKES; i++)
		      		running = running || counter[i].snake_running;
		}		
		running = running && !quit;	
		  
		fflush(stdout);
		  
		sleep_time.tv_nsec = SLEEP_TIME;
		sleep_time.tv_sec = 0;
		  
		while(nanosleep(&sleep_time, &sleep_time) == -1) { }
	}//end while

	
	// Log final scores of each snake
	if ( NUM_SNAKES == 0 ) {
		fprintf(gamelog, "Final Score for Snake #%d is: %d\n", 1 , counter[0].score);
		fprintf(gamelog, "Final Length for Snake #%d is: %d\n", 1 , counter[0].length);//!!!!!
	}
	else {	
	  	for ( i = 0; i < NUM_SNAKES; i++){
	    		fprintf(gamelog, "Final Score for Snake #%d is: %d\n", i+1 , counter[i].score);
			fprintf(gamelog, "Final length for Snake #%d is: %d\n", i+1 , counter[i].length);//!!
		}
	}
	
	cleanup_handler(0);
	return 0;
}
