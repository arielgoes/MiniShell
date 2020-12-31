//Sistemas Operacionais: Mini-Shell; Ariel Góes de Castro (1701450064); Victor Hugo Schneider Lopes (161150749).

//Bibliotecas necessárias
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>


//Alguns buffers e variáveis globais
#define READ_BUFSIZE 1024 //tamanho do buffer de leitura da linha de comando;
#define ARG_BUFSIZE 32 //tamanho do buffer de argumentos;
char *shell_commands[] = {"help", "exit", "cd"}; //Comandos built-in do shell;
unsigned int size_commands = sizeof(shell_commands) / sizeof(char *); //Quantidade de comandos built-in;
int size_args = 0; //Quantidade de argumentos que cada comando possui;



//Declarações das funções criadas;
void shell_loop(char** envp); //Loop principal do shell;
char* shell_read_line(); //funcao que le cada comando digitado pelo usuário;
char** shell_split(char* line); //recebe o comando, e o divide em "tokens";
int shell_apply(char** args, char** envp); //verifica os tokens e chama as funções que irão executá-los;
int shell_launch(int command, char** args); //executa os comandos built-in;
int shell_launch_std(char** args, char** envp); //executa os comandos por meio do execvpe;
int shell_cd(char** args); //utiliza 'chdir' para mudar o diretorio;
int shell_help(); //apresenta uma mensagem de ajuda ao usuário;
int shell_exit(); //fecha o programa;



int main(int argc, char** argv, char** envp){
    shell_loop(envp);
    return 0;
}



void shell_loop(char** envp){
    char *line; //linha de comando escrita pelo usuário
    char **args; //vetor de strings, onde cada string é um comando 'tokenizado';
    int status; //verifica se o shell deve continuar a ser executado;

    //pega o 'hostname';
    char *buf_path = (char*)malloc(100 * sizeof(char));
    buf_path = getcwd(buf_path, 100);

    do{
        buf_path = getcwd(buf_path, 100); //pega o caminho ate a pasta atual
        printf("\033[1;33mMyShell\033[1;36m~%s\033[0m$ >  ", buf_path);
        line = shell_read_line(); //a partir de um buffer, retorna o comando para a string 'line';
        args = shell_split(line); //separa a linha de comando em argumentos;

        //conta a quantidade de argumentos
        for(int i = 0; args[i] != NULL; i++){
            size_args++;
        }

        status = shell_apply(args, envp); //pode chamar "shell_launch" our "shell_launch_std"
        free(line);
        free(args);
        size_args = 0; //'reseta' a variável do tamanho a cada iteração
    }while(status);
}



char* shell_read_line(){
    unsigned int pos = 0;
    char* buffer = (char *)malloc(sizeof(char) * READ_BUFSIZE); //aloca um buffer para captar os comandos digitados pelo usuário;
    int bufsize = READ_BUFSIZE;

    //caso em que a alocação do buffer falha;
    if(buffer == NULL){
        fprintf(stderr, "ERROR! BAD ALLOC! (buffer)\n");
        exit(EXIT_FAILURE);
    }

    char c; //variável auxiliar que verifica os estados de entrada a cada iteração;

    while (true){
        c = getchar();
        if (c == EOF){ //comando padrão de saída UNIX/LINUX: CTRL + D;
            exit(EXIT_SUCCESS);
    }
    else if (c == '\n'){ //caso em que o usuário digitar 'ENTER';
        buffer[pos] = '\0'; //insere o caracter nulo...
        break; //...e sai do loop;
    }
    else{ //quando digitado um caractere válido, ele é atribuído ao buffer;
        buffer[pos] = c;
    }
    pos++;

    //se a posição extrapolar o tamanho do buffer, são realocados mais 1024 bytes;
    if(pos >= READ_BUFSIZE){
        bufsize += READ_BUFSIZE;
        buffer = realloc(buffer, bufsize);

        if(buffer == NULL){
            printf("ERROR! BAD REALLOC!\n");
            exit(EXIT_FAILURE);
        }
    }
}
    //Quando terminado, retorna o buffer contendo a linha de comando;
    return buffer;
}



char** shell_split(char* line){
  /*string de delimitadores (separam comandos da linha de comando informada)*/
    const char str_delim[2] = " \t";
    char *token;

    /*pega a primeira substring */
    token = strtok(line, str_delim);

    /*vetor que irá conter os argumentos do comando a ser executado, nesse caso podem haver 15 argumentos*/
    unsigned int argsize = ARG_BUFSIZE;
    char **args = (char**)malloc(sizeof(char*) * ARG_BUFSIZE);

    /*Caso não consiga alocar espaço para o char** args*/
    if(args == NULL){
        fprintf(stderr, "ERROR! BAD ALLOC! (args)\n");
        exit(EXIT_FAILURE);
    }

    int pos = 0;

    // pega o resto das substrings
    while(token != NULL){
    //cada token contem um argumento, que é inserido no vetor de argumentos
        args[pos] = token;
        pos++;

    //caso mais argumentos sejam digitados do que o tamanho disponível, aloca-se mais espaço de memória;
    if(pos >= argsize){
        argsize += ARG_BUFSIZE;
        args = realloc(args, argsize);

        //se a realocação falhar... encerra o fluxo de execução do programa;
        if(!args){
            fprintf(stderr, "ERROR! BAD ALLOC! (args)\n");
            exit(EXIT_FAILURE);
        }
    }

        //renova a 'substring'
        token = strtok(NULL, str_delim);
    }

    //As chamadas de sistema execv, execp ... recebem o vetor de argumentos sempre terminado em NULL;
    args[pos] = '\0'; //pode ser também '\0' ou 'NULL';
    return args; //retorna o vetor de argumentos 'tokenizados';
}



int shell_apply(char** args, char** envp){
    //Verifica se foi passado algum comando, do contrário, continua rodando o loop do shell;
    if(args[0] == NULL){
        return 1;
    }

    //se o comando passado for um dos comandos "built-in", chama-se a função shell_launch;
    for(int i = 0; i < size_commands; i++){
        if(strcmp(args[0], shell_commands[i]) == 0){
        return shell_launch(i, args);
        }
    }

    //se não for uma função built-in, chama-se a função shell_launch_std, que a executará por meio do execvpe;
    return shell_launch_std(args, envp);
}



int shell_launch(int command, char** args){
    if(command == 0){
        return shell_help();
    }
    else if(command == 1){
        return shell_exit();
    }
    else if(command == 2){
        return shell_cd(args);
    }
    else{
        return 0;
    }
}



int shell_help(){
    printf("UNIPAMPA ALEGRETE - Ciencia da Computacao\n");
    printf("Micro-Shell por Victor Lopes e Ariel Góes de Castro\n");
    printf("Primeiro Trabalho da Disciplina de Sistemas Operacionais\n");
    printf("Comandos implementados pelo programa:\n");

    for(int i = 0; i < size_commands; i++){
        printf("%s ", shell_commands[i]);
    }

    printf("\nApenas digite o comando desejado e tecle ENTER\n");
    printf("Para obter informacoes sobre outros programas utilize o comando 'man'\n");

    return 1;
}



int shell_exit(){
    return 0;
}



int shell_cd(char** args){
    if(args[1] == NULL){
        printf("ERROR! NO DIRECTORY INFORMED");
    }
    else{
        if(chdir(args[1]) != 0){
            perror("lsh");
        }
    }
    return 1;
}



int shell_launch_std(char** args, char** envp){
    int save_fd_in = dup(0), save_fd_out = dup(1), file_fd, redir = 0; //armazena o 'file descriptor' padrão de entrada e saída;
    int tem_pipe = 0, status, j, num_pipes = 0; //tem_pipe = verifica presença de pelo menos um pipe; status = variável auxiliar;
    int pos_allcommands = 0, pos_current = 0, num_commands = 1;
    pid_t pid; //pid = variável para criação dos processos;
    FILE* file = NULL; //ponteiro para arquivo (caso exista redirecionamento);
    char** args_aux ; //ponteiro para strings que estejam antes OU depois do redirecionamento ('>', '<');

    //Casos em que o usuário desejar redirecionar a entrada/saída do arquivo;
    for(int i = 0; i < size_args; i++){
        if(strcmp(args[i], ">") == 0){
            file = fopen(args[i+1], "w+");
            if(!file){
                perror("file");
                return 1;
            }

            //obtém-se o(s) comando(s) antes do '>';
            args_aux = (char**)malloc(i * sizeof(char*));
            for(int j = 0; j < i; j++){
                args_aux[j] = args[j];
            }

            args = args_aux;
            /*Com a retirada do comando de redirecionamento de saída e do nome do arquivo,
            a quantidade de argumentos deve ser alterada para refletir a atual*/
            size_args = i;

            file_fd = fileno(file); //pega o descritor de arquivos do file;
            redir = 1; //indica que deve-se redirecionar a saída para um arquivo;
            break;
        }
        else if(strcmp(args[i], "<") == 0){
            file = fopen(args[i-1], "w+");
            if(!file){
                perror("file");
                return 1;
            }

            args_aux = (char**)malloc((size_args - (i + 1)) * sizeof(char*));

            //obtém-se o(s) comando(s) depois do '<';
            for(int j = i + 1; j < size_args; j++){
                args_aux[j - (i+1)] = args[j];
            }

            args = args_aux;
            /*Com a retirada do comando de redirecionamento de saída e do nome do arquivo,
            a quantidade de argumentos deve ser alterada para refletir a atual*/
            size_args = size_args - (i + 1);

            file_fd = fileno(file);
            redir = 1; //indica que deve-se redirecionar a saída para um arquivo;
            break;
        }
    }
    //conta quantos pipes o argumento possui
    for(int i = 0; i < size_args; i++){
        if(strcmp(args[i], "|") == 0){
            num_pipes++;
        }
    }

    //Caso em que existe pelo menos um pipe:
    if(num_pipes >= 1){
        char** current_command = NULL; //vetor de strings temporário;
        //vetor com todos os comandos, cada comando pode possuir mais de um argumento; ex.: 'cd ..';
        char*** allcommands = (char***)malloc(sizeof(char**) * (num_pipes + 1));

        for(int i = 0; i < size_args; i++){
            if(strcmp(args[i], "|") == 0){ //Caso em que encontra um pipe, coloca-se o comando antes do pipe em uma posição do vetor de comandos;
                allcommands[pos_allcommands] = current_command;
                pos_allcommands++;
                pos_current = 0;
                current_command = NULL;
                num_commands = 1;
            }
            else{//Enquanto não encontrar um pipe adiciona argumentos ao comando atual
                if(current_command == NULL){//Aloca-se um espaço ao vetor temporário;
                    current_command = (char **)malloc(sizeof(char *) * num_commands);
                }
                else{ //Se já tiver um comando no vetor temporário, aloca-se mais espaço para mais argumentos;
                    num_commands++;
                    current_command = realloc(current_command, sizeof(char *) * num_commands);
                }
                current_command[pos_current] = args[i]; //O argumento atual eh colocado no comando atual;
                pos_current++;
            }
        }
        //O ultimo comando é adicionado ao vetor de comandos;
        allcommands[pos_allcommands] = current_command;

        int size_allcommands = pos_allcommands + 1;
        int size_fd = size_allcommands - 1;
        int pipefd[size_fd][2]; //Vetor de pipes, cada qual com sua respectiva entrada e saída;

        //Cria todos os pipes necessários;
        for(int i = 0; i < size_allcommands; i++){
            pipe(pipefd[i]);
        }
        //Para cada comando eh criado um processo novo que vai executá-lo;
        for(int i = 0; i < size_allcommands; i++){
            pid = fork();
            if(pid < 0){
                perror("fork pipe");
                return 0;
            }
            else if(pid == 0){
                if(i != 0){ //caso genérico;
                    dup2(pipefd[i-1][0], 0); //lê do anterior;
                }else{ //Caso for o primeiro comando... não há necessidade de ler do pipe;
                    dup2(save_fd_in, 0);
                }

                if((i + 1) != size_allcommands){ //Caso genérico;
                    dup2(pipefd[i][1], 1); //Escreve no próximo;
                }else{ //Caso for o ultimo comando, escreve-se na saida padrão, que pode ser o shell ou um arquivo;
                    if(redir == 1){
                        dup2(file_fd, 1);
                    }
                    else{
                        dup2(save_fd_out, 1);
                    }
                }

                //Fecha-se os pipes;
                for(int j = 0; j < size_allcommands - 1; j++){
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }

                //Executa o comando atual;
                if(execvpe(allcommands[i][0], allcommands[i], envp) == -1){
                    perror("execvpe pipe");
                    return 0;
                }
                else{
                    exit(0);
                }
            }
        }

        //Fecha-se os pipes;
        for(int i = 0; i < size_allcommands - 1; i++){
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }
        //Espera o último processo filho terminar sua execução;
        waitpid(pid, &status, 0);
    }
    else{ //Se não houver nenhum pipe;

        if(redir == 1){
            dup2(file_fd, 1);
        }
        else{
            dup2(save_fd_out, 1);
        }
        pid = fork();

        if(pid < 0){
            perror("fork single command");
            return 0;
        }
        else if(pid == 0){
            if(execvpe(args[0], args, envp) == -1){
                perror("execvpe single command");
                return 0;
            }
            exit(EXIT_SUCCESS);
        }

        waitpid(pid, &status, 0);
        dup2(save_fd_out, 1);
    }

    return 1;
}
