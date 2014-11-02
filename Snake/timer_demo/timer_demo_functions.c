static void init_default_snake();
static void init_user_snake(Point *p, int length, int snakeID);
static int update_snake(int snakeID);
static void init_tile(Tile *t, int state, Tile **neighbour, int r, int c);
static void update_graphic_string(Tile *t, int state, int colour);
static void update_score(int snakeID);

static void generate_negfood();
static void generate_user_block(Boundary *obst );
static void generate_block();
void cleanup_handler (int signum);
//AI
void shortpath(int snakeID);
//void long_snake(int snakeID);
static void go_east(int snakeID) ;
static void turn_right(int snakeID);
//timer
void timer_intr(void);
static void timer_sleep_init( int msec );
static void pushbutton_handler();
//do func
void inc_func_val(int snakeID);
void do_func(int snakeID);
/************************************************************
 *This function Initialized every tile with defualt value   *
 *			                                    *
 *                                                          *
 ***********************************************************/
static void init_tile(Tile *t, int state, Tile **neighbour, int r, int c) {
	t->p.r = r;
	t->p.c = c;

	t->neighbour[NORTH] = neighbour[NORTH];
	t->neighbour[EAST]  = neighbour[EAST];
	t->neighbour[SOUTH] = neighbour[SOUTH];
	t->neighbour[WEST]  = neighbour[WEST];

	t->state = state;
	t->dist = 1000;

	t->next = NULL;
	t->prev = NULL;

	t->fnext = NULL;
	t->fprev = NULL;

	update_graphic_string(t, state, 0);

	t->string[13] = '\0';
}
/************************************************************
 *This function gives the tile a symbol and color depending *
 *on the state of the tile.                                 *
 *                                                          *
 ***********************************************************/
static void update_graphic_string(Tile *t, int state, int colour) {

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
		case NEGFOOD:
			bg = WHITE;
			fg = t->neg_food_value;
			symbol = 169;
			bold = 0;
			break;
		case SNAKE:
			bg = WHITE;
			fg = colour;
			symbol = 'O';
			bold = 0;
			break;
		case SNAKE_HEAD:
			bg = WHITE;
			fg = colour;
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
 *This function places the contents of sring[0] of tile t   *
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
	Tile *newfood; // temporary variable for linklist

	do {
		food.r = rand() % ROWS;
		food.c = rand() % COLS;
	}while(tile[food.r][food.c].state != EMPTY);

	
	tile[food.r][food.c].food_value = 1 + (rand() % MAX_FOOD_VALUE);
	update_graphic_string(&tile[food.r][food.c], FOOD, 0);
	printGraphic(&tile[food.r][food.c]);
 	food_count++;
	newfood = &tile[food.r][food.c];

	if (food_count == 1)
	{	
		fhead = newfood;		
		ftail = fhead;
		newfood -> fprev = NULL;
		newfood -> fnext = NULL;	
	}
	else if(food_count > 1)
	{
		newfood -> fprev = ftail; 
		newfood -> fnext = NULL; 
		ftail -> fnext = newfood; 
		ftail = newfood; 
	}
	
}

static void generate_negfood()
{
	Point negfood;

	do 
	{
		negfood.r = rand() % ROWS;
		negfood.c = rand() % COLS;
	} while(tile[negfood.r][negfood.c].state != EMPTY);

	tile[negfood.r][negfood.c].neg_food_value = 1 + (rand() % MAX_FOOD_VALUE);
	update_graphic_string(&tile[negfood.r][negfood.c], NEGFOOD, 0);
	printGraphic(&tile[negfood.r][negfood.c]);
 	
	neg_food_count++;
}

/************************************************************
 *This function places snake on random location and makes   *
 *the tail point to body and makes body point to head. The  *
 *is checked to be in EMPTY state before placement          *
 ***********************************************************/
static void init_default_snake() {

	Point snakey;

	// Initialize snake variables	
	counter[0].food_move = 0;
	counter[0].food_shrink = 0;
	counter[0].score = 0;
	counter[0].snake_running = TRUE;
	counter[0].dir = EAST;
	counter[0].length = 3;
	counter[0].func_val = 0;

		
	do {
		snakey.r = rand() % ROWS;
		snakey.c = rand() % COLS;
	// Checks if the neighbours are empty before making the snake
	}while( (tile[snakey.r][snakey.c].state != EMPTY) || (tile[snakey.r][snakey.c-1].state != EMPTY) || (tile[snakey.r][snakey.c-2].state != EMPTY) || snakey.c < 1 || snakey.r < 1);

	//Point to head and tail
	tile[snakey.r][snakey.c-2].next = &tile[snakey.r][snakey.c-1];
	tile[snakey.r][snakey.c-1].next = &tile[snakey.r][snakey.c];
	tile[snakey.r][snakey.c].next = NULL;	

	tile[snakey.r][snakey.c].prev = &tile[snakey.r][snakey.c-1]; //head ->mid  !!!!!
	tile[snakey.r][snakey.c-1].prev = &tile[snakey.r][snakey.c-2];	//mid->tail //!!!!!!
	tile[snakey.r][snakey.c-2].prev = NULL;	


	counter[0].head = tile[snakey.r][snakey.c].p;
	counter[0].tail = tile[snakey.r][snakey.c-2].p;
	fprintf(gamelog,"\nDefault Snake Head at (%d,%d)\n",counter[0].head.r,counter[0].head.c);
	fprintf(gamelog,"Default Snake Body at (%d,%d)\n",tile[snakey.r][snakey.c-1].p.r,tile[snakey.r][snakey.c-1].p.c);
	fprintf(gamelog,"Default Snake Tail at (%d,%d)\n\n",counter[0].tail.r,counter[0].tail.c); 
	fflush(gamelog);

	update_graphic_string(&tile[snakey.r][snakey.c], SNAKE_HEAD, GREEN);
	update_graphic_string(&tile[snakey.r][snakey.c-1], SNAKE, GREEN);
	update_graphic_string(&tile[snakey.r][snakey.c-2], SNAKE, GREEN);
	printGraphic(&tile[snakey.r][snakey.c]);
	printGraphic(&tile[snakey.r][snakey.c-1]);
	printGraphic(&tile[snakey.r][snakey.c-2]);

}

/************************************************************
 *This function places snake using coordinate specified by  *
 *the user in the configuration file. The function returns  *
 *the snake direction determined by the neighbors' state    *
 *of the last coordinate.				    *
 ***********************************************************/
static void init_user_snake(Point *p, int length, int snakeID) {

	int i;

	// Initializes snake variables
	counter[snakeID].tail = p[0];
	counter[snakeID].head = p[length-1];
	counter[snakeID].food_move = 0;
	counter[snakeID].score = 0;
	counter[snakeID].snake_running = TRUE;
	counter[snakeID].dir = EAST;
	counter[snakeID].length = length;//!!!!
	counter[snakeID].food_shrink = 0;//!!!
	counter[snakeID].func_val = 0;
	
	// Initializes snake elements
	for ( i=0; i < length; i++){
		if ( tile[p[i].r][p[i].c].state == EMPTY ) {

			if ( i == length-1){
				update_graphic_string(&tile[p[i].r][p[i].c], SNAKE_HEAD, snakeID+2);
				//Head points to NULL
				tile[p[i].r][p[i].c].next = NULL;
				tile[p[i].r][p[i].c].prev = &tile[p[i-1].r][p[i-1].c];
			}
			else if ( i == 0){
				update_graphic_string(&tile[p[i].r][p[i].c], SNAKE, snakeID+2);
				tile[p[i].r][p[i].c].next = &tile[p[i+1].r][p[i+1].c]; 
				//Tail points to NULL
				tile[p[i].r][p[i].c].prev = NULL;
			}
			else{
				tile[p[i].r][p[i].c].next = &tile[p[i+1].r][p[i+1].c];
				tile[p[i].r][p[i].c].prev = &tile[p[i-1].r][p[i-1].c];		
				update_graphic_string(&tile[p[i].r][p[i].c], SNAKE, snakeID+2);
			}	
			printGraphic(&tile[p[i].r][p[i].c]);
		}
		else{
			fprintf(gamelog, "Error: Snake #%d has a coordinate conflict\n", snakeID+1);
			fflush(gamelog);
			cleanup_handler(0);
		}
	}

	// Set snake direction by looking at the last two coordinates
	if ( tile[p[length-2].r][p[length-2].c].next == tile[p[length-2].r][p[length-2].c].neighbour[NORTH] )
		counter[snakeID].dir = NORTH;
	else if ( tile[p[length-2].r][p[length-2].c].next == tile[p[length-2].r][p[length-2].c].neighbour[SOUTH] )
		counter[snakeID].dir = SOUTH;
	else if ( tile[p[length-2].r][p[length-2].c].next == tile[p[length-2].r][p[length-2].c].neighbour[WEST] )
		counter[snakeID].dir = WEST;
	else if ( tile[p[length-2].r][p[length-2].c].next == tile[p[length-2].r][p[length-2].c].neighbour[EAST] )
		counter[snakeID].dir = EAST;
	else {
		fprintf(gamelog, "Error: Snake #%d has a coordinate conflict\n", snakeID+1);
		fflush(gamelog);
		cleanup_handler(0);
	}
	
	// If direction is blocked, choose another direction
	if ( tile[(counter[snakeID].head).r][(counter[snakeID].head).c].neighbour[counter[snakeID].dir]->state != EMPTY ) {
		for ( i = 0; i < 4; i++) {
			if ( (tile[(counter[snakeID].head).r][(counter[snakeID].head).c].neighbour[i])->state == EMPTY ) {
				counter[snakeID].dir == i ; 
				break;
			}
			else{
				if ( i == 3 ){
					fprintf(gamelog, "Error: Snake #%d has a coordinate conflict\n", snakeID+1);
					fflush(gamelog);
					cleanup_handler(0);
				}
			}			
		}
	}
}

void shortpath(int snakeID)
{
	
	Point start = counter[snakeID].head;

	Tile *neighb;
	Tile *current_food = fhead;
	Tile *closest_snack;
	Tile *current_tile;

	int new_dir;
	int distance;
 

	
	int i,j,k;

	for (i = 0; i < ROWS; i++){
		for (j = 0; j < COLS; j++){
			tile[i][j].dist = 1000;
		}
	}

	if ( food_count != 0 ){		

		tile[start.r][start.c].dist = 0;
		for (i = 0; i < ROWS; i++){
			for (j = 0; j < COLS; j++){
				for ( k = 0; k<4; k++ ){
						neighb = tile[(start.r+i)%ROWS][(start.c+j)%COLS].neighbour[k];
						distance = tile[(start.r+i)%ROWS][(start.c+j)%COLS].dist;
						if ((neighb->state == FOOD || neighb->state == EMPTY) && ( (distance+1) <= neighb->dist ) ) 
							(tile[(start.r+i)%ROWS][(start.c+j)%COLS].neighbour[k])->dist = distance+1;	 
				}
			}
			for (j = COLS; j>0; j--){
				for ( k = 0; k<4; k++ ){
						neighb = tile[(start.r+i)%ROWS][(start.c+j)%COLS].neighbour[k];
						distance = tile[(start.r+i)%ROWS][(start.c+j)%COLS].dist;
						if ((neighb->state == FOOD || neighb->state == EMPTY) && (distance+1) <= neighb->dist ) 
							(tile[(start.r+i)%ROWS][(start.c+j)%COLS].neighbour[k])->dist = distance+1;	 
				}
			}

		}

		for (i = ROWS; i > 0; i--){
			for (j = 0; j < COLS; j++){
				for ( k = 0; k<4; k++ ){
						neighb = tile[(start.r+i)%ROWS][(start.c+j)%COLS].neighbour[k];
						distance = tile[(start.r+i)%ROWS][(start.c+j)%COLS].dist;
						if ((neighb->state == FOOD || neighb->state == EMPTY) && (distance+1) <= neighb->dist ) 
							(tile[(start.r+i)%ROWS][(start.c+j)%COLS].neighbour[k])->dist = distance+1;	 
				}
			}
			for (j = COLS; j>0; j--){
				for ( k = 0; k<4; k++ ){
						neighb = tile[(start.r+i)%ROWS][(start.c+j)%COLS].neighbour[k];
						distance = tile[(start.r+i)%ROWS][(start.c+j)%COLS].dist;
						if ((neighb->state == FOOD || neighb->state == EMPTY) && (distance+1) <= neighb->dist ) 
							(tile[(start.r+i)%ROWS][(start.c+j)%COLS].neighbour[k])->dist = distance+1;	 
				}
			}

		}


		// distance to first food tile
		distance = current_food->dist;
		closest_snack = current_food;
	 
		while (current_food != NULL)
		{	 
			i = 0;			
			for ( k = 0; k < 4; k++){
				if ( current_food->neighbour[k]->state == EMPTY || current_food->neighbour[k]->state == FOOD )
					i++;
			}
			if ( i > 1 ){
				if ( current_food->dist < distance ){
					distance = current_food->dist;
					closest_snack = current_food;
				}
			}
		 	current_food = (current_food -> fnext);

		}

	 	current_tile = closest_snack;


		if ( distance < 1000 ){
			while ( distance > 1 ){
				for ( k = 0; k < 4; k++){
					if ( current_tile->neighbour[k]->dist == distance-1 )
						current_tile = current_tile->neighbour[k];	
				}	
				distance--;	
			}

			for ( k = 0; k < 4; k++){
				if ( current_tile->neighbour[k]->dist == 0 ){
					new_dir = (k+2)%4;
					if ( (counter[snakeID].dir%2 == 0 && new_dir%2 != 0) || (counter[snakeID].dir%2 != 0 && new_dir%2 == 0) )
						counter[snakeID].dir = new_dir;
					else 
						turn_right(snakeID); 
				}
			}
		}
		else
			turn_right(snakeID);	
	}
	else
		go_east(snakeID);
	
}

//This function implements the longest snake
/*
void long_snake(int snakeID){
	int state_status;
	int i;
	Point chead;

	chead = counter[snakeID].head;
	
	if (global_i == 0){
		counter[snakeID].dir = NORTH;
		for (i = 0; i < 3; i++) {
			state_status = (tile[chead.r][chead.c].neighbour[counter[snakeID].dir])->state;
			if((state_status == EMPTY || state_status == FOOD) && (global_r < ROWS)){
				global_r++;
				break;
			}
			counter[snakeID].dir = (counter[snakeID].dir+1)%4;
			global_i = 1;
			global_r = 0;
		}
	}
	else 	{
		counter[snakeID].dir = SOUTH;
		for (i = 0; i < 3; i++) {
			state_status = (tile[chead.r][chead.c].neighbour[counter[snakeID].dir])->state;
			if((state_status == EMPTY || state_status == FOOD) && (global_r < ROWS)){
				global_r++;
				break;
			}
			counter[snakeID].dir = (counter[snakeID].dir-1)%4;
			global_i = 0;
			global_r = 0;
			}
	}	
}
*/

//This part of the code implements the "GO EAST" AI
static void go_east(int snakeID) 
{
	int state_status;
	int i;
	Point chead;
	chead = counter[snakeID].head;
	counter[snakeID].dir = EAST;
	
	for (i = 0; i < 3; i++)
	{
		state_status = (tile[chead.r][chead.c].neighbour[counter[snakeID].dir])->state;
		if(state_status == EMPTY || state_status == FOOD)
			break;
		counter[snakeID].dir = counter[snakeID].dir+1;
		counter[snakeID].dir = counter[snakeID].dir%4;
	}
}

//This part of the code implements the "TURN RIGHT" AI
static void turn_right(int snakeID)
{
 int state_status;
 int i;
 Point chead;
 chead = counter[snakeID].head;
 
 for (i = 0; i < 3; i++)
 {
 	 state_status = (tile[chead.r][chead.c].neighbour[counter[snakeID].dir])->state;
 	 if(state_status == EMPTY || state_status == FOOD)
 	 	 break;
 	 counter[snakeID].dir = counter[snakeID].dir+1;
 	 counter[snakeID].dir = counter[snakeID].dir%4;
 }
}

//change ai type
void inc_func_val(int snakeID) {
	counter[snakeID].func_val = (counter[snakeID].func_val + 1)%4;
	fprintf(gamelog, "The moving method for SNAKE#%d is: %d\n",snakeID , counter[snakeID].func_val);
}

//do the func based on func val
void do_func(int snakeID) {
	if (counter[snakeID].func_val == 0)
		shortpath(snakeID);
	if (counter[snakeID].func_val == 1)
		go_east(snakeID);
	if (counter[snakeID].func_val == 2)
		//keyboard input
		if ( (counter[snakeID].dir%2 == 0 && keyboard_dir%2 != 0) || (counter[snakeID].dir%2 != 0 && keyboard_dir%2 == 0) )		
			counter[snakeID].dir = keyboard_dir;
	if (counter[snakeID].func_val == 3){
		//pushbutton input
		if ( (counter[snakeID].dir%2 == 0 && pb_dir%2 != 0) || (counter[snakeID].dir%2 != 0 && pb_dir%2 == 0) )		
			counter[snakeID].dir = pb_dir;
	}
	update_snake(snakeID);
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
static int update_snake(int snakeID) {

	Point origHead;
	Point origTail;	
	Point newHead;
	
	int grow, shrink, length;//!!!!
	//int i;

	origHead = counter[snakeID].head;
	origTail = counter[snakeID].tail;
	grow = counter[snakeID].food_move;	
	shrink = counter[snakeID].food_shrink;	//!!!
	length = counter[snakeID].length;		//!!!	
		


	if (counter[snakeID].snake_running == TRUE) {

		newHead = (tile[(counter[snakeID].head).r][(counter[snakeID].head).c].neighbour[counter[snakeID].dir])->p;

		//only move food or empty		
		if ( (tile[newHead.r][newHead.c].state == EMPTY) || (tile[newHead.r][newHead.c].state == FOOD) || (tile[newHead.r][newHead.c].state == NEGFOOD) ) {

			//move SNAKE_HEAD according to direction
			counter[snakeID].head = newHead;
			//if snake eats food, store food value and update score	
			if (tile[newHead.r][newHead.c].state == FOOD) {
				grow += tile[newHead.r][newHead.c].food_value;
				food_count--;
				counter[snakeID].score += tile[newHead.r][newHead.c].food_value;
				//update_score(tile[newHead.r][newHead.c].food_value, snakeID);		
				length += tile[newHead.r][newHead.c].food_value;//!!!

				//update food linked list	
				if (newHead.r == fhead->p.r && newHead.c == fhead->p.c ){	
					if (food_count != 0){	
						fhead = tile[newHead.r][newHead.c].fnext;
						tile[newHead.r][newHead.c].fnext = NULL;
						fhead->fprev = NULL;
					}
					else {
						fhead = NULL;
						ftail = NULL;
					}				
				}
				else if (newHead.r == ftail->p.r && newHead.c == ftail->p.c ){
					if (food_count != 0){	
						ftail = tile[newHead.r][newHead.c].fprev;
						tile[newHead.r][newHead.c].fprev = NULL;								
						ftail->fnext = NULL;
					}
					else {
						fhead = NULL;
						ftail = NULL;
					}				
				}
				else {
					(tile[newHead.r][newHead.c].fprev)->fnext = tile[newHead.r][newHead.c].fnext;
					(tile[newHead.r][newHead.c].fnext)->fprev = tile[newHead.r][newHead.c].fprev;
				}

			}

//if snake eats negative-value food, decrease the negative food count //!!!
			else if (tile[newHead.r][newHead.c].state == NEGFOOD) 
			{
				shrink += tile[newHead.r][newHead.c].neg_food_value;
				neg_food_count--;
			}


			//grow if food_move is zero, move tail

			if (counter[snakeID].food_move == 0) {
	
				counter[snakeID].tail = (tile[origTail.r][origTail.c].next)->p;
				tile[origTail.r][origTail.c].next = NULL;
				update_graphic_string(&tile[origTail.r][origTail.c], EMPTY, 0);
	
			}	
			//if food_move is zero, don't move tail, decrement food_move
			else 
				grow--;

			//update pointer to snake head
			tile[origHead.r][origHead.c].next = &tile[newHead.r][newHead.c];
			tile[newHead.r][newHead.c].prev = &tile[origHead.r][origHead.c];

			//update and display head, old tail and new tail tiles
			update_graphic_string(&tile[origHead.r][origHead.c], SNAKE, snakeID+2);
			update_graphic_string(&tile[newHead.r][newHead.c], SNAKE_HEAD, snakeID+2);
			printGraphic(&tile[origHead.r][origHead.c]);
			printGraphic(&tile[newHead.r][newHead.c]);
			printGraphic(&tile[origTail.r][origTail.c]);

			while (shrink>0)//!!!!!!!!
			{		
				if (length > 4)
				{	
					update_graphic_string(&tile[newHead.r][newHead.c], EMPTY, 0);	
					printGraphic(&tile[newHead.r][newHead.c]);
			
					newHead = tile[newHead.r][newHead.c].prev -> p;
					counter[snakeID].head = newHead;	
	
					update_graphic_string(&tile[newHead.r][newHead.c], SNAKE_HEAD, snakeID+2);
					printGraphic(&tile[newHead.r][newHead.c]);			
			
					length--;
					shrink--;
				}
			
				else
				{		
					shrink = 0;
					break;
				}	
			}	 

		}//end if

		//if snake hits obstical, kill snake and update log file with cause of death
		else if ( tile[newHead.r][newHead.c].state == OBSTICAL ) {
			fprintf(gamelog, "Snake #%d: Death from obstacle collision!\n", snakeID+1);
			fflush(gamelog);
			counter[snakeID].snake_running = FALSE;
		}
		//if snake hits itself or another snake, kill snake and update log file with cause of death
		else if ( (tile[newHead.r][newHead.c].state == SNAKE) || (tile[newHead.r][newHead.c].state == SNAKE_HEAD) ) {
			fprintf(gamelog, "Snake #%d: Death from snake collision!\n", snakeID+1);
			fflush(gamelog);
			counter[snakeID].snake_running = FALSE;
		}

	}

	counter[snakeID].food_move = grow;
	counter[snakeID].food_shrink = shrink;	
	counter[snakeID].length = length;	
	

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
                        update_graphic_string(&tile[block.r][block.c], OBSTICAL, 0);
                        printGraphic(&tile[block.r][block.c]);
                        block_count++;
                        break;
                case 1:
                        update_graphic_string(&tile[block.r][block.c], OBSTICAL, 0);
                        printGraphic(&tile[block.r][block.c]);
                        
                        update_graphic_string(tile[block.r][block.c].neighbour[NORTH], OBSTICAL, 0);
                        printGraphic(tile[block.r][block.c].neighbour[NORTH]);

                        update_graphic_string(tile[block.r][block.c].neighbour[EAST], OBSTICAL, 0);
                        printGraphic(tile[block.r][block.c].neighbour[EAST]);
                        
                        Point newblock = (tile[block.r][block.c].neighbour[NORTH]) ->p;
                        update_graphic_string(tile[newblock.r][newblock.c].neighbour[EAST], OBSTICAL, 0);     
                        printGraphic(tile[newblock.r][newblock.c].neighbour[EAST]);

                        block_count += 4;
                        break;
                default:
                        break;
        }        
}   
/****************************************************************
 *Function to generate obsicle if some are defined in the cfg   * 
 * File. If none the generate_block() function is called        *
 *                                                              *
 *                                                              *
 ***************************************************************/    
static void generate_user_block(Boundary *obst){
  int obsticle_number;
  Point initial;
  
  for(obsticle_number = 0; obsticle_number < num_obstacles; obsticle_number++){
    for (initial.r = obst[obsticle_number].start.r; initial.r  <= obst[obsticle_number].end.r;initial.r++){
      for(initial.c = obst[obsticle_number].start.c; initial.c <= obst[obsticle_number].end.c;initial.c++){
	update_graphic_string(&tile[initial.r][initial.c], OBSTICAL,0);
	printGraphic(&tile[initial.r][initial.c]);
	}
    }
  }
}
/*******************************************************************
* Score updating and displaying function.                          *
*	input: int food_v                                          *
*	output: none                                               *
*This function takes the food_v and adds it to the existing score, *
*and then displays the updated score in the upper left corner in   *
*white.                                                            *
********************************************************************/
static void update_score(int snakeID) {
	char scoreStr[7] = "SCORE: ";
	char sc_rep[4] = "";
	char outputStr[14] = "";
	short sc = 0;//short int to store score in order to put into char array
	int i = 0;	

	sc_rep[3] = '\0';
	outputStr[13] = '\0';;

	//Display "SCORE:"	
	MOVE_CURSOR(-1,-1);
	
	memcpy(&outputStr[0], &FG[WHITE], 7);
	memcpy(&outputStr[7], &BG[BLUE], 5);
	outputStr[2] = '1';

	for (i=0; i < 7; i++) {
		outputStr[12] = scoreStr[i];
		fputs( (const char*)&outputStr, stdout);
	}

	//Display numerical score for specific snake
	sc = (short)counter[snakeID].score;
	snprintf( sc_rep, 4, "%d", sc ); 

	MOVE_CURSOR(-1,-1+(12*snakeID)+10);
		
	memcpy(&outputStr[0], &FG[snakeID+2], 7);
	memcpy(&outputStr[7], &BG[BLUE], 5);
	outputStr[2] = '1';
	
	for (i=0; i < 3; i++) {
		outputStr[12] = sc_rep[i];
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


	int mask;
	
	ioctl(timer_fd, TIMER_READ_REG, &TCSR0);	
	mask = TCSR0.data & 0x100;
	
	if (mask == 0x100)		
		timer_intr();
	else{
		key = getchar();
		if(key != EOF) 
		{
			key_received = TRUE;

			//Buffer latest direction and control keys
			switch (key) 
			{
				case 'w':
				case 'W':
					key_pressed[KEY_WASD] = NORTH;
					key_wasd_received = TRUE;
					break;
				case 's':
				case 'S':
					key_pressed[KEY_WASD] = SOUTH;
					key_wasd_received = TRUE;
					break;
				case 'a':
				case 'A':
					key_pressed[KEY_WASD] = WEST;
					key_wasd_received = TRUE;
					break;
				case 'd':
				case 'D':
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
				case '1':
					key_pressed[KEY_1] = TRUE;
					break;
				case '2':
					key_pressed[KEY_2] = TRUE;
					break;
				case '3':
					key_pressed[KEY_3] = TRUE;
					break;
				case '4':
					key_pressed[KEY_4] = TRUE;
					break;
				case '\n':
					key_pressed[KEY_ENTER] = TRUE;
					break;
				default:
					break;
			}
		}


	}
}

/* Cleanup function to be called, if possible, whenever program quits.  If this
	function is not called the terminal might not respond to inputs correctly or
	the screen could be garbled due to not leaving line mode and, (at least on
	the petalinux system) this would require a restart to fix.
*/
void cleanup_handler (int signum) {

	fprintf(gamelog, "CLEANUP!\n");

	TCSR0.data = 0x00;
	ioctl(timer_fd, TIMER_WRITE_REG, &TCSR0);

	int i;
	for (i = 0;i < ROWS; i++){
		free(tile[i]);
	}
	for (i = 0;i < NUM_SNAKES; i++){
		free(snakeCoordinate[i]);
	}
	free(snakeCoordinate);
	free(tile);
	free(counter);
	free(obst);
	free(snake_length);

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


static void pushbutton_handler() {

	int key_2;	

	pb_fd = open("/dev/pb_driver",O_RDWR); 
	value = 0xFFFFFFFF;
	data.offset = 4;
	data.data = value;
	ioctl(pb_fd, LED_WRITE_REG, &data);	

	data.offset = 0;
	ioctl(pb_fd, LED_READ_REG, &data);

	key_2 = data.data;
	
		if (key_2 == 16)
		{
			if ( pb_dir != SOUTH )
			pb_dir = NORTH;
		}
		if (key_2 == 4)
		{
			if ( pb_dir != NORTH )
			pb_dir = SOUTH;
		}
		if (key_2 == 8)
		{
			if ( pb_dir != EAST )
			pb_dir = WEST;
		}
		if (key_2 == 2) 
		{
			if ( pb_dir != WEST )
			pb_dir = EAST;
		}

}

void timer_intr(void){

	int mask;
	ioctl(timer_fd, TIMER_READ_REG, &TCSR0);	
	mask = TCSR0.data & 0x100;
	
	if (mask == 0x100){
		
		timer_flag = 1;	
		//fflush(gamelog);
		ioctl(timer_fd, TIMER_WRITE_REG, &TCSR0);
	}
}

static void timer_sleep_init( int msec ){			
		
	fprintf(gamelog,"Initializing Timer\n");
	timer_flag = 0;
	
	// Set load reg with interval=3 seconds 
	TLR0.data=((msec*75000)-2); // 75MHz
	ioctl(timer_fd,TIMER_WRITE_REG,&TLR0); 
	
	// Load the value in TLR0 to timer
	TCSR0.data=0x20;
	ioctl(timer_fd,TIMER_WRITE_REG,&TCSR0);

	// Enable timer0,enable timer to give interrupt, auto-reload, enable external generate signal, and set to count down
	TCSR0.data=0xD6;
	ioctl(timer_fd,TIMER_WRITE_REG,&TCSR0);
	
	//fprintf(gamelog,"in timer_sleep and finished setting up timer...\n");
	//fflush(gamelog);
	
	fprintf(gamelog,"Timer Initialized :)\n");
}

