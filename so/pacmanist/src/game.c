#include "board.h"
    #include "display.h"
    #include "parser.h"
    #include <stdlib.h>
    #include <time.h>
    #include <unistd.h>
    #include <string.h>
    #include <fcntl.h>
    #include <sys/wait.h>
    #include <sys/stat.h>  
    #include <signal.h>
    #include <pthread.h>
    #include <errno.h>      

    #include <semaphore.h>
    #define CONTINUE_PLAY 0
    #define NEXT_LEVEL 1
    #define QUIT_GAME 2
    #define LOAD_BACKUP 3
    #define CREATE_BACKUP 4

char levels_directory[256] = "levels";

void screen_refresh(board_t * game_board, int mode) {
    debug("REFRESH\n");
    draw_board(game_board, mode);
    refresh_screen();
    if(game_board->tempo != 0)
        sleep_ms(game_board->tempo);       
}

int play_board(board_t * game_board, char comando, Session *session) {
    pacman_t* pacman = &game_board->pacmans[0];
    command_t* play;
    command_t c; 
    if (pacman->n_moves == 0) { // if is user input
        c.command = comando;
        if(c.command == '\0'){
            session->game_event = CONTINUE_PLAY;
            return CONTINUE_PLAY;
        }
        c.turns = 1;
        play = &c;
    }
    else {
        play = &pacman->moves[pacman->current_move % pacman->n_moves];
    }
    debug("KEY %c\n", play->command);
    if (play->command == 'Q') {
        session->game_event = QUIT_GAME;
        return QUIT_GAME;
    }
    int result = move_pacman(game_board, 0, play);
    if (result == REACHED_PORTAL) {
        // Next level
        session->game_event = NEXT_LEVEL;
        return NEXT_LEVEL;
    }
    if(result == DEAD_PACMAN) {
        session->game_event = DEAD_PACMAN;
        return DEAD_PACMAN;
    }
    if (!game_board->pacmans[0].alive) {
        session->game_event = DEAD_PACMAN;
        return DEAD_PACMAN;
    }      
    session->game_event = CONTINUE_PLAY;
    return CONTINUE_PLAY;  
}


bool find_next_level(const char *current_level, char *next_level, size_t size) {
    int level_num = atoi(current_level);   // "1.lvl" -> 1
    level_num++;

    snprintf(next_level, size, "%d.lvl", level_num);

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", levels_directory,next_level);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return false;   // não existe
    }
    close(fd);
    return true;        // existe
}

//////////////////////////////////////////////////////////////////////////////////////
void* ghost_thread_func(void* arg) {
    ghost_thread* gdata = (ghost_thread*)arg;
    ghost_t* ghost = gdata->ghost;
    board_t* board = gdata->board;
    Session *session = gdata->session;
    while (session->running) {
        if(board->pacmans[0].alive==false){
            session->game_event = DEAD_PACMAN;
            break;
        }
        pthread_mutex_lock(gdata->lock);
        move_ghost(board, gdata->index, &ghost->moves[ghost->current_move % ghost->n_moves]);

        pthread_mutex_unlock(gdata->lock);
        sleep_ms(500);
    }

    free(gdata);
    return NULL;
}

char* prepara_board_servidor(board_t* board){
    size_t buffer_tamanho=(board->width*board->height);
    char* new_board=malloc(buffer_tamanho);
    if (!new_board){
        return NULL;
    }
    int pos=0;
    int height=board->height;
    int width=board->width;
    for(int i=0;i<height;i++){
        for(int j=0; j<width;j++){
            board_pos_t* cell=&board->board[i*width+j];
            char display_elem;
            if(cell->content!=' ' && cell->content !='\0'){
                switch(cell->content){
                    case 'M':
                        display_elem='M';
                        break;
                    case 'P':
                        display_elem='C';
                        break;
                    case 'W':
                        display_elem='#';
                        break;
                    default:
                        display_elem=cell->content;
                        break;
                }
            }
            else if(cell->has_dot){
                display_elem='.';
            }
            else if(cell->has_portal){
                display_elem='@';
            }
            else{
                display_elem=' ';
            }
            
            new_board[pos]=display_elem;
            pos++;
        }

    }
    return new_board;
}



void* notifier_thread_func(void* arg){
    
    ui_thread_data *data = (ui_thread_data*)arg;
    Session *session = data->session;

    int fd_notif = data->fd_notif;

    
    while (session->running) {
        unsigned char op = 4;

        int width, height, tempo;

        int victory = 0;
        int points;
        char *board_chars;

        pthread_mutex_lock(data->lock);
        width  = data->board->width;
        height = data->board->height;
        tempo  = data->board->tempo;
        points = data->board->pacmans[0].points;
        board_chars = prepara_board_servidor(data->board);
        pthread_mutex_unlock(data->lock);

        if (!board_chars) break;

        size_t n = (size_t)width * (size_t)height;

        debug("Notifier writing board update: op=%d, width=%d\n", op, width);
        if (write(fd_notif, &op, 1) != 1) { free(board_chars); break; }
        write(fd_notif, &width,  sizeof(int));
        write(fd_notif, &height, sizeof(int));
        write(fd_notif, &tempo,  sizeof(int));
        write(fd_notif, &victory,sizeof(int));

        write(fd_notif, &session->game_over,sizeof(int));
        write(fd_notif, &points, sizeof(int));
        write(fd_notif, board_chars, n);

        free(board_chars);

        sleep_ms(10);
    }

    return NULL;
}


void* pacman_thread_func(void* arg) {

    pacman_thread_data* pdata = (pacman_thread_data*)arg;
    board_t* board = pdata->board;
    Session *session = pdata->session;
    debug("Pacman about to open sessao_pipe\n");
    int fd_req= pdata->fd_req;
    char msg[2];

    while (session->running) {
        ssize_t n = read(fd_req,msg,2);
        if (n == 0) { 
            session->running = 0;
            break;
        }
        char comando=msg[1];

        if(msg[0]==2){
            session->game_event = DEAD_PACMAN;
        }

        pthread_mutex_lock(pdata->lock);
        play_board(board,comando,session);
        pthread_mutex_unlock(pdata->lock);
        sleep_ms(50);
    }


    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////

void gaming_session(Session *session) { 

    srand((unsigned int)time(NULL)); 
    open_debug_file("debug.log"); 
    pthread_mutex_t board_lock = PTHREAD_MUTEX_INITIALIZER; 
    int accumulated_points = 0; 
    bool end_game = false; 
    board_t game_board; 
    memset(&game_board, 0, sizeof(board_t)); 
    char current_level[32] = "1.lvl"; 
    lvl_parser(current_level, &game_board,levels_directory); 
    load_level(&game_board, accumulated_points); 

    session->running = 1;            
    session->game_event = CONTINUE_PLAY; 
    session->game_over = 0;          
    
    int int_req_pipe = open(session->req_pipe_path, O_RDONLY);
    if(int_req_pipe==-1){
        perror("open notif");
        return;
    }

    int int_notif_pipe = open(session->notif_pipe_path, O_WRONLY);
    if(int_notif_pipe==-1){
        perror("open notif");
        return;
    }
    while (!end_game) { 
        
        session->running = 1; 
        
        pthread_t notifier_thr; 
        ui_thread_data ui = {.board = &game_board,.lock = &board_lock,.fd_notif = int_notif_pipe,
            .session = session};
        pthread_create(&notifier_thr, NULL, notifier_thread_func, &ui); 
        
        pthread_t pacman_thr; 
        pacman_thread_data pdata = {.board = &game_board,.lock = &board_lock,.fd_req = int_req_pipe,
            .session = session};
        pthread_create(&pacman_thr, NULL, pacman_thread_func, &pdata); 
        
        pthread_t *ghost_threads = malloc(sizeof(pthread_t) * game_board.n_ghosts); 
        for (int i = 0; i < game_board.n_ghosts; i++) { 
            ghost_thread* g = malloc(sizeof(ghost_thread)); 
            g->ghost = &game_board.ghosts[i]; 
            g->board = &game_board; 
            g->index = i; 
            g->lock = &board_lock; 
            g->session = session;   
            pthread_create(&ghost_threads[i], NULL, ghost_thread_func, g); 
        }
        
        while (true) {
            
            
            if (session->game_event == QUIT_GAME) {
                close_debug_file(); 
                return; 
            }

            if (session->game_event == NEXT_LEVEL) {
                char next_level[32];
                if (!find_next_level(current_level, next_level, sizeof(next_level))) {
                    session->game_over = 1;
                    sleep_ms(50);
                    session->running = 0;

                    for (int i = 0; i < game_board.n_ghosts; i++)
                        pthread_join(ghost_threads[i], NULL);
                    pthread_join(notifier_thr, NULL);
                    pthread_join(pacman_thr, NULL);

                    sleep_ms(game_board.tempo);
                    end_game = true;
                    break;
                }

                session->game_event = CONTINUE_PLAY;
                int saved_points = game_board.pacmans[0].points;

                session->running = 0;
                for (int i = 0; i < game_board.n_ghosts; i++)
                    pthread_join(ghost_threads[i], NULL);
                pthread_join(notifier_thr, NULL);

                unload_level(&game_board);
                strcpy(current_level, next_level);
                strcpy(game_board.level_name, next_level);
                lvl_parser(current_level, &game_board, levels_directory);
                load_level(&game_board, saved_points);

                sleep_ms(game_board.tempo);
                break;
            }
            
            if (session->game_event == DEAD_PACMAN) {
                sleep_ms(game_board.tempo);
                session->game_over = 1;

                for (int i = 0; i < game_board.n_ghosts; i++)
                    pthread_join(ghost_threads[i], NULL);
                pthread_join(notifier_thr, NULL);
                pthread_join(pacman_thr, NULL);

                end_game = true;
                break;
            }
           
            accumulated_points = game_board.pacmans[0].points;
            
        }
    }
    close(int_req_pipe);
    close(int_notif_pipe);
    close_debug_file(); 
    terminal_cleanup();
    return;
}


void* anfitria_thread_function(void* arg) {//NOVAS FUNCTIONS
    server_data* server = (server_data*) arg;


    
    int fd_reg = open(server->register_pipe, O_RDWR);
    if (fd_reg == -1) {
        perror("Erro ao abrir register_pipe");
        pthread_exit(NULL);
    }

    while (true) {
        sem_wait(&server->sem_empty); // espera espaço livre no buffer

        char msg[81];
        read(fd_reg, msg, sizeof(msg)); // lê pedido do cliente

        Session sessao;
        memset(&sessao, 0, sizeof(sessao));
        strncpy(sessao.req_pipe_path, msg + 1, 40);
        sessao.req_pipe_path[40] = '\0';
        strncpy(sessao.notif_pipe_path, msg + 41, 40);
        sessao.notif_pipe_path[40] = '\0';


        
        int fd_reply = open(sessao.notif_pipe_path, O_WRONLY);// Envia resposta para o cliente
        if (fd_reply != -1) {
            char reply[2] = {1, 0};
            write(fd_reply, reply, sizeof(reply));
            close(fd_reply);
        } else {
            perror("Erro ao abrir pipe de notificação para reply");
            continue;
        }

        // Coloca a session no buffer PRODUTOR -CONSUMIDOR
        pthread_mutex_lock(&server->buffer_mutex);
        server->buffer[server->in] = sessao;
        server->in = (server->in + 1) % server->buffer_size;
        pthread_mutex_unlock(&server->buffer_mutex);

        sem_post(&server->sem_full); 

 
    }

    close(fd_reg);
    pthread_exit(NULL);
}


void* thread_gestoras_function(void* arg) {//NOVAS FUNCTIONS
    server_data* server = (server_data*) arg;
    while (true) {
        sem_wait(&server->sem_full); 

        pthread_mutex_lock(&server->buffer_mutex); 
        Session sessao = server->buffer[server->out]; 
        server->out = (server->out + 1) % server->buffer_size; 
        pthread_mutex_unlock(&server->buffer_mutex); 
        sem_post(&server->sem_empty); 
        pthread_mutex_lock(&server->buffer_mutex); 
        server->current_games++;           
        pthread_mutex_unlock(&server->buffer_mutex); 

        gaming_session(&sessao); //Chama a função que executa todo o jogo para esta sessão ainda n feito(ja esta feita :))
 
 
        pthread_mutex_lock(&server->buffer_mutex); 
        if (server->current_games > 0) server->current_games--; 
        pthread_mutex_unlock(&server->buffer_mutex); 

    }

    pthread_exit(NULL);
}



int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage: %s <level_directory> <max_games> <register_pipe>\n", argv[0]);
        return 1;
    }
    signal(SIGPIPE, SIG_IGN);
    server_data server;
    strncpy(levels_directory,argv[1],256);
    size_t len = strlen(levels_directory);
    if (len > 0 && levels_directory[len-1] == '/') {
        levels_directory[len-1] = '\0';
    }
    server.max_games=atoi(argv[2]);
    strncpy(server.register_pipe,argv[3],40);
    server.current_games=0;
    mkfifo(server.register_pipe, 0666);


    server.buffer_size = server.max_games;   
    server.buffer = malloc(sizeof(Session) * server.buffer_size);
    server.in = 0;
    server.out = 0;                   ///BUFFER PRODUTOR-CONSUMIDOR INICIALIZAÇAO
    pthread_mutex_init(&server.buffer_mutex, NULL);
    sem_init(&server.sem_empty, 0, server.buffer_size);
    sem_init(&server.sem_full, 0, 0);


    pthread_t thread_anfitria;
    pthread_create(&thread_anfitria,NULL,anfitria_thread_function,&server); 

    pthread_t *threads = malloc(sizeof(pthread_t) * server.max_games);
    for (int i = 0; i < server.max_games; i++) {
        pthread_create(&threads[i],NULL,thread_gestoras_function,&server);  
    }

    while (1) {
        pause();
    }
    return 0;
}
