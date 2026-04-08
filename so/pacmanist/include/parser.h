#ifndef PARSER_H
#define PARSER_H

#include "board.h"
#include <stdbool.h>
void pacman_parser(char *pacman_file, board_t *board);
int  lvl_parser(char *level_file, board_t *board, char levels_directory[]);
bool lvl_line_parser(char *line, board_t *board);
void parse_tabuleiro(char *line, board_t *board);
void monster_parser(char *monster_file, board_t *board);

#endif
