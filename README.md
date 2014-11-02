SnakeRepo
=========

Linux Environment Project

Final code without timer and push button- can be used to run on any linux environment. No push button or timer are implemented in this code.

Snake with Timer and pb/FinalNoMenu- contains code with push button and timer implemented but can only be run on Xilinix board.

Tmier Driver and timer test are added also for viewing


How to run the game

Using the command line below will run the snake game with the default settings:
./snake
The log file name will be named “snake.log” by default. To change the log file name or load specific levels use the following command line below:
./snake <level_name.cfg> <logfile_name> <seed_number>
For example:
./snake  level2.cfg  snakey.log  24

How to change the moving method

The key 1, 2, 3, and 4 changes the moving method for Snake1, Snake2, Snake3, and Snake4 respectively. A moving method can be chosen by repeatedly pressing the number key until the desired moving method is obtained. 
How to control the snake
First ensure that the moving method is either on Keyboard or Push Buttons. For the keyboard the W, A, S, and D keys makes the snake face in the north, west, south, and east respectively. Pressing the BTNU, BTNL, BTNR, and BTND push buttons will make the snake face in the north, west, east, and south direction respectively.

How to pause & quit the game

Pressing the P key will pause the game and pressing it again un-pauses the game. To quit the game press the Q key and the game will exit. Before the game exits the status (e.g. scores, moving method, and reason of death) of the snakes will be saved to the specified log file.
