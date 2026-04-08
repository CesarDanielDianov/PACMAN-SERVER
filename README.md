# -PACMAN-SERVER-Sistemas Operativos-
  Jogo bi-dimensional inspirado no Pacman desenvolvido em C no âmbito da disciplina de Sistemas Operativos.
  O projeto explora paralelização com múltiplas tarefas, comunicação entre processos, sincronização de threads, e interação com o sistema     de ficheiros POSIX.

#FUNCIONALIDADES PRINCIPAIS:
  Servidor de jogos que suporta múltiplos clientes em sessões paralelas via named pipes.
  Implementação de API cliente: pacman_connect, pacman_disconnect, pacman_play, receive_board_updates.
  Comunicação estruturada entre cliente e servidor com mensagens codificadas (OP_CODE e dados fixos).
  Movimentos do Pacman e monstros geridos por threads separadas, com uma thread exclusiva para ncurses.
  Suporte a múltiplas sessões concorrentes com sincronização via mutexes e semáforos.
  Construído a partir da primeira parte do projeto, versão single-player do PacmanIST com manipulação de ficheiros e             reencarnação do Pacman.

#EXECUTAR (terminal linux):
    Assumindo que se está no directório que inclui os directórios 'pacmanist' e 'client',deve-se compilar e executar priemiro o servidor,da seguinte   forma (ja dentro do diretorio 'pacmanist'):
      (COMPILAÇAO: make )
      (EXECUÇAO: ./bin/Pacmanist levels  3 /tmp/pacman_server)
        *'3' é o numero maximo de users que se podem conectar ao msm tempo ao servidor,caso ja esteje cheio,os users a mais serão postos 
      numa lista de espera 
    
  De seguida,com novas instancias de terminal ,entrar no diretorio 'client', compilar e executar da seguinte forma :
      (COMPILAÇAO: make)
      (EXECUÇAO: ./bin/client 3 /tmp/pacman_server)
         *'3' neste caso é o identificador do user(cada user tem id unico)

#FERRAMENTAS USADAS:
    Linguagem C,
    POSIX API (fork, named pipes, file descriptors, signals),
    Threads (pthread library),
    Biblioteca ncurses para interface do jogo,
    Sistema Linux.
