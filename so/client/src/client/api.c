#include "api.h"
#include "protocol.h"
#include "debug.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <semaphore.h>






int pacman_connect(char const *req_pipe_path, char const *notif_pipe_path, char const *server_pipe_path) {
  debug("Pacman connect");
  unlink(req_pipe_path);
  unlink(notif_pipe_path);
  if (mkfifo(req_pipe_path, 0666) == -1 && errno != EEXIST) {
    perror("falha no FIFO");
    return 1;
  }
  if (mkfifo(notif_pipe_path, 0666) == -1 && errno != EEXIST) {
      perror("falha no FIFO");
      return 1;
  }
  debug("Abriu, pacman connect\n");
  
  int fd_server=open(server_pipe_path,O_WRONLY);

  char msg[81];
  msg[0] = 1; 
  strncpy(&msg[1],req_pipe_path,40);
  strncpy(&msg[41],notif_pipe_path,40);

  write(fd_server, msg, 81);
  debug("vai abrir notif pipe");
  int fd_noti=open(notif_pipe_path,O_RDONLY);

  char reply[2];
  debug("About to read reply from notif_pipe\n");
  int bytes_read = read(fd_noti,reply,2);
  debug("Read %d bytes, Received reply: %d %d\n", bytes_read, reply[0], reply[1]);
  if (reply[1] != 0) {
      
      close(fd_noti);
      close(fd_server);
      return 1;
  }
  else{
    close(fd_noti);
    close(fd_server);
    return 0;
  }
}

void pacman_play(char command, int fd_req) {
  char msg[2];
  msg[0]=3;
  msg[1]=command;
  if(write(fd_req,msg,2)!=2){
    perror("Erro no pipe requesições, ao escrever");
    return -1;
  }
  return 0;
}

int pacman_disconnect(const char *req_pipe_path, const char *notif_pipe_path) {
  int fd_req = open(req_pipe_path, O_WRONLY);   // abre FIFO de requests
  char msg[1];
  msg[0]=2;                                     // envia OP_CODE=2 (disconnect)
  write(fd_req,msg,1);
  return 0;
}

Board receive_board_update(int notif_fifo) {
  Board new_board;
  memset(&new_board, 0, sizeof(new_board));
  unsigned char op;
  int rc = read(notif_fifo, &op, 1);
  if (rc <= 0) return new_board; // pipe fechado/erro => b.data NULL
  if (op != 4) return new_board; // op_code diferente de 4

  if (read(notif_fifo, &new_board.width,  sizeof(int)) <= 0) return new_board;
  if (read(notif_fifo, &new_board.height, sizeof(int)) <= 0) return new_board;
  if (read(notif_fifo, &new_board.tempo,  sizeof(int)) <= 0) return new_board;
  if (read(notif_fifo, &new_board.victory,sizeof(int)) <= 0) return new_board;
  if (read(notif_fifo, &new_board.game_over,sizeof(int)) <= 0) return new_board;
  if (read(notif_fifo, &new_board.accumulated_points,sizeof(int)) <= 0) return new_board;
  
  size_t n = (size_t)new_board.width * (size_t)new_board.height;

  new_board.data = malloc(n);

  if(read(notif_fifo,new_board.data,n)<=0){
    free(new_board.data);
    return new_board;
  }

  return new_board;   
}