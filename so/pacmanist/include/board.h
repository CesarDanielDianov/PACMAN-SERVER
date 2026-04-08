#ifndef BOARD_H
#define BOARD_H

#define MAX_MOVES 20
#define MAX_LEVELS 20
#define MAX_FILENAME 256
#define MAX_GHOSTS 25

#include <pthread.h>
#include <semaphore.h>




typedef enum {
    REACHED_PORTAL = 1,
    VALID_MOVE = 0,
    INVALID_MOVE = -1,
    DEAD_PACMAN = -2,
} move_t;

typedef struct {
    char command;
    int turns;
    int turns_left;
} command_t;

typedef struct {
    int pos_x, pos_y; //current position
    int alive; // if is alive
    int points; // how many points have been collected
    int passo; // number of plays to wait before starting
    command_t moves[MAX_MOVES];
    int current_move;
    int n_moves; // number of predefined moves, 0 if controlled by user, >0 if readed from level file
    int waiting;
} pacman_t;

typedef struct {
    int pos_x, pos_y; //current position
    int passo; // number of plays to wait between each move
    command_t moves[MAX_MOVES];
    int n_moves; // number of predefined moves from level file
    int current_move;
    int waiting;
    int charged;
} ghost_t;

typedef struct {
    char content;   // stuff like 'P' for pacman 'M' for monster/ghost and 'W' for wall
    int has_dot;    // whether there is a dot in this position or not
    int has_portal; // whether there is a portal in this position or not
} board_pos_t;

typedef struct {
    int width, height;      // dimensions of the board
    board_pos_t* board;     // actual board, a row-major matrix
    int n_pacmans;          // number of pacmans in the board
    pacman_t* pacmans;      // array containing every pacman in the board to iterate through when processing (Just 1)
    int n_ghosts;           // number of ghosts in the board
    ghost_t* ghosts;        // array containing every ghost in the board to iterate through when processing
    char level_name[256];   //name for the level file to keep track of which will be the next
    char pacman_file[256];  // file with pacman movements
    char ghosts_files[MAX_GHOSTS][256]; // files with monster movements
    int tempo;              // Duration of each play
} board_t;

typedef struct{
    int id;
    int req_pipe;
    int notif_pipe;
    char req_pipe_path[40+ 1];
    char notif_pipe_path[40+ 1];
    int running;        //variaveis que eram globais ,passaram a locais pra cada insatncia de game
    int game_event;     
    int game_over;  
  }Session;

typedef struct {
    int max_games;
    int current_games;
    char register_pipe[40+ 1];
    int fd_register;

    Session *buffer;
    int buffer_size;
    int in;
    int out;

    pthread_mutex_t buffer_mutex;
    sem_t sem_empty;
    sem_t sem_full;
} server_data;




typedef struct{
    ghost_t* ghost;
    board_t* board;
    int index;
    pthread_mutex_t *lock;  
    Session *session;
}ghost_thread;

typedef struct{
    board_t* board;
    pthread_mutex_t *lock;
    int fd_req;
    Session *session;
}pacman_thread_data;

typedef struct {
    board_t *board;
    pthread_mutex_t *lock;
    int fd_notif;
    Session *session;
} ui_thread_data;

/*Makes the current thread sleep for 'int milliseconds' miliseconds*/
void sleep_ms(int milliseconds);

/*Processes a command for Pacman or Ghost(Monster)
*_index - corresponding index in board's pacman_t/ghost_t array
command - command to be processed*/
int move_pacman(board_t* board, int pacman_index, command_t* command);
int move_ghost(board_t* board, int ghost_index, command_t* command);

/*Process the death of a Pacman*/
void kill_pacman(board_t* board, int pacman_index);

/*Adds a pacman to the board*/
int load_pacman(board_t* board, int points);

/*Adds a ghost(monster) to the board*/
int load_ghost(board_t* board);

/*Loads a level into board*/
int load_level(board_t* board, int accumulated_points);

/*Unloads levels loaded by load_level*/
void unload_level(board_t * board);

// DEBUG FILE

/*Opens the debug file*/
void open_debug_file(char *filename);

/*Closes the debug file*/
void close_debug_file();

/*Writes to the open debug file*/
void debug(const char * format, ...);

/*Writes the board and its contents to the open debug file*/
void print_board(board_t* board);

#endif
