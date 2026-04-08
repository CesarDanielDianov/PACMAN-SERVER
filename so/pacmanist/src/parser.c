#include "board.h"
#include "display.h"
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>     // open, O_RDONLY
#include <unistd.h>    // read, close
#include <string.h>

#include <stdbool.h>





void parse_tabuleiro(char *line,board_t *board){
    if (board->width <= 0 || board->height <= 0 || board->board == NULL) {
        perror("Board error");
        return;
    }
    for(int j=0; j< board ->height && *line != '\0'; j++){
        for(int i=0; i< board ->width && line[i] != '\0' && line[i] != '\n'; i++){
            if(line[i] == 'X'){
                board ->board[j*board ->width + i].content = 'W';
            }
            else if (line[i] == 'o'){
                board ->board[j*board ->width + i].content = ' ';
                board ->board[j*board ->width + i].has_dot = 1;
            }
            else if  (line[i]=='@'){
                board ->board[j*board ->width + i].content = ' ';
                board ->board[j*board ->width + i].has_portal = 1;
            }
            else{
                perror("Board error");
                return;
            }

        }
        char *end = strchr(line, '\n');
        if (!end) break;   
        line = end + 1;

    }
}

void monster_parser(char *monster_file, board_t *board) {

    char path[512];
    snprintf(path, sizeof(path), "monsters/%s", monster_file);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open monster file");
        return;
    }

    char buffer[1024];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read monster file");
        close(fd);
        return;
    }
    buffer[bytes_read] = '\0';
    close(fd);

    // 1) escolher um slot livre para este ghost
    ghost_t *ghost = &board->ghosts[board->n_ghosts];

    // 2) inicializar estado do ghost
    ghost->n_moves      = 0;
    ghost->current_move = 0;
    ghost->waiting      = 0;
    ghost->charged      = 0;

    char *line = buffer;

    /* -------- PASSO -------- */
    if (strncmp(line, "PASSO", 5) == 0) {
        int passo;
        sscanf(line + 6, "%d", &passo);
        ghost->passo = passo;

        char *nl = strchr(line, '\n');
        if (!nl) {
            // ficheiro mal formado, nada mais a fazer
            return;
        }
        line = nl + 1;
    } else {
        // ficheiro não começa com PASSO -> erro simples
        perror("monster file: missing PASSO");
        return;
    }

    /* -------- POS -------- */
    if (strncmp(line, "POS", 3) == 0) {
        int x, y;
        sscanf(line + 4, "%d %d", &x, &y);
        ghost->pos_x = x;
        ghost->pos_y = y;
        board->board[x + y * board->width].content = 'M';
        char *nl = strchr(line, '\n');
        if (!nl) {
            return;
        }
        line = nl + 1;
    } else {
        perror("monster file: missing POS");
        return;
    }

    /* -------- COMANDOS (resto do ficheiro) -------- */
    char command;

    while (*line != '\0') {

        // saltar linhas vazias
        if (*line == '\n') {
            line++;
            continue;
        }

        if (*line == 'T') {
            int turns;
            // espera por T <n>
            sscanf(line + 2, "%d", &turns);

            ghost->moves[ghost->n_moves].command    = 'T';
            ghost->moves[ghost->n_moves].turns      = turns;
            ghost->moves[ghost->n_moves].turns_left = turns;
            ghost->n_moves++;
        } else {
            // comando simples: A / D / W / S / R / C
            sscanf(line, "%c", &command);

            ghost->moves[ghost->n_moves].command    = command;
            ghost->moves[ghost->n_moves].turns      = 1;
            ghost->moves[ghost->n_moves].turns_left = 0;
            ghost->n_moves++;
        }

        // ir para a próxima linha de comandos
        char *nl = strchr(line, '\n');
        if (!nl) {
            break;      // última linha do ficheiro
        }
        line = nl + 1;
    }

    // 3) registar ESTE ghost no board (uma única vez!)
    board->ghosts[board->n_ghosts] = *ghost;
    board->n_ghosts++;
    
}

void pacman_parser(char *pacman_file, board_t *board){

    char path[512];
    snprintf(path, sizeof(path), "pacmans/%s", pacman_file);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open pacman file");
        return;
    }

    char buffer[1024];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read pacman file");
        close(fd);
        return;
    }
    buffer[bytes_read] = '\0';
    close(fd);


    pacman_t *pac = &board->pacmans[0];   



    pac->pos_x = 1;
    pac->pos_y = 1;
    pac->alive = 1;
    pac->points = 0;
    pac->passo = 0;
    pac->n_moves = 0;      // controlado pelo jogador
    pac->current_move = 0;
    pac->waiting = 0;
    char *line = buffer;

    /* -------- PASSO -------- */
    if (strncmp(line, "PASSO", 5) == 0) {
        int passo;
        sscanf(line + 6, "%d", &passo);
        pac->passo = passo;

        char *nl = strchr(line, '\n');
        if (!nl) {
            // ficheiro mal formado, nada mais a fazer
            return;
        }
        line = nl + 1;
    } else {
        // ficheiro não começa com PASSO -> erro simples
        perror("pacman file: missing PASSO");
        return;
    }

    /* -------- POS -------- */
    if (strncmp(line, "POS", 3) == 0) {
        int x, y;
        sscanf(line + 4, "%d %d", &x, &y);
        pac->pos_x = x;
        pac->pos_y = y;
        board->board[x + y * board->width].content = 'P';
        char *nl = strchr(line, '\n');
        if (!nl) {
            return;
        }
        line = nl + 1;
    } else {
        perror("pacman file: missing POS");
        return;
    }
    board->n_pacmans = 1;
    char command;
    while (*line != '\0') {

        // saltar linhas vazias
        if (*line == '\n') {
            line++;
            continue;
        }

        if (*line == 'T') {
            int turns;
            // espera por T <n>
            sscanf(line + 2, "%d", &turns);

            pac->moves[pac->n_moves].command    = 'T';
            pac->moves[pac->n_moves].turns      = turns;
            pac->moves[pac->n_moves].turns_left = turns;
            pac->n_moves++;
        } else {
            // comando simples: A / D / W / S / R / C
            sscanf(line, "%c", &command);

            pac->moves[pac->n_moves].command    = command;
            pac->moves[pac->n_moves].turns      = 1;
            pac->moves[pac->n_moves].turns_left = 0;
            pac->n_moves++;
        }

        // ir para a próxima linha de comandos
        char *nl = strchr(line, '\n');
        if (!nl) {
            break;      // última linha do ficheiro
        }
        line = nl + 1;
    }
}



bool lvl_line_parser(char *line, board_t *board) {
    if (strncmp(line, "DIM", 3) == 0) {

        int width, height;
        sscanf(line + 4, "%d %d", &width, &height);

        board->width = width;
        board->height = height;
        if (board->board == NULL) {
            board->board = malloc(width * height * sizeof(board_pos_t));
            if (!board->board) {
                perror("malloc board");
                exit(1);
            }
        }

        memset(board->board, 0, width * height * sizeof(board_pos_t));

        return false;
    }
    else if (strncmp(line, "TEMPO", 5) == 0) {
        int tempo;
        sscanf(line + 6, "%d", &tempo);
        board->tempo = tempo;
        return false;
    }

    // PAC -----
    else if (strncmp(line, "PAC", 3) == 0) {
        if (board->pacmans == NULL) {
            board->pacmans = malloc(sizeof(pacman_t));
            if (!board->pacmans) {
                perror("malloc pacmans");
                exit(1);
            }
            board->n_pacmans = 1;
        }
        char pacman_file[256];
        sscanf(line + 4, "%s", pacman_file);
        pacman_parser(pacman_file, board);
        return false;
    }
    // MON -----
    else if (strncmp(line, "MON", 3) == 0) {

        if (board->ghosts == NULL) {
            board->ghosts = malloc(MAX_GHOSTS * sizeof(ghost_t));
            if (!board->ghosts) {
                perror("malloc ghosts");
                exit(1);
            }
            board->n_ghosts = 0;
        }
    
        line += 4; 
        while (*line != '\0' && *line != '\n') {
            char *end = strchr(line, ' ');
            if (end == NULL) {
                monster_parser(line, board);
                break;
            }
            *end = '\0';
            monster_parser(line, board);
            line = end + 1;
        }
        return false;
    }
    

    else {
        return true;
    }
}






int lvl_parser(char *level_file, board_t *board,char levels_directory[]) {
    strcpy(board->level_name, level_file);

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", levels_directory, level_file);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    if (board->ghosts == NULL) {
        board->ghosts = malloc(MAX_GHOSTS * sizeof(ghost_t));
        if (!board->ghosts) {
            perror("malloc ghosts");
            close(fd);
            return -1;
        }
    }
    board->n_ghosts = 0; 

    char buffer[1024];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        return -1;
    }
    buffer[bytes_read] = '\0';

    char *line = buffer;
    bool Acabou = false;
    while (!Acabou) {
        char *end = strchr(line, '\n');
        if (!end) break;

        *end = '\0';
        Acabou = lvl_line_parser(line, board);

        if (Acabou) {
            *end = '\n';      
            line = line;      
            break;
        }
        
        line = end + 1;       // só avança enquanto estamos no header
        
    }
    parse_tabuleiro(line, board);
    // ...
    close(fd);
    return 0;
}


