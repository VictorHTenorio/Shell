#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define TAMANHO_BUFFER_INICIAL 1000000
#define TAMANHO_BLOCO 1000000

typedef struct Node {
    char* arg;           // Armazena um argumento
    int flag;
    int flagRedirecionamento;
    int flagP;
    int flagBackground;
    struct Node* prox;   // Ponteiro para o próximo nó na lista encadeada
} Node;


// Protótipos das funções
Node* inicializarNode(const char* arg, int flag,int redirecionamento, int pipe, int background);
void adicionarNode(Node** head, const char* arg, int flag,int redirecionamento,int pipe,int background);
void liberarLista(Node* head);
void imprimir(Node* head);
void copyLinkedList(Node* head, Node** history);
void executarSequen(char* arg,int flag, int redirecionamento, int pipe, int background);
void executarPar(Node**aux);
void* executarParalelo(void* arg);
void redirecionamentoFuncaoSeq(char* comando, int flagRedirecionamento);
void* executarParaleloRedirecionamento(void* arg);
void redirecionamentoFuncaoPar(Node* head);
void executarParaleloComRedirecionamento(Node* head);
void execPipeSeq(char* comando);
void* execPipePar(void* arg);
void copiarListaNoFinal(Node** destino, Node* origem);

int flagModo = 0;  // 1 - paralelo, 0 - sequencial
int flagFuncionando = 1; //1 - ligado , 0 -desligado
int executei = 0; // 0 - nao executei, 1 -executei, 2 -executei historico 
int flagRedirecionamento = 0;  // 0 - sem redirecionamento, 1 = >, 2 = >>, 3 = <
int flagPipe = 0; // 0 - sem pipe , 1 - com pipe
int flagHistory = 0; // 0 - nao digitou , 1 - digitou
int flagPrimeiraVez = 0;// 0 - primeira vez , 1 - nao é a primeira vez
int flagBatchFile = 0; // 0 - não é batchfile , 1 - é batchfile
int flagBack = 0;// 0 - não é background, 1 - é background

int main(int argc, char* argv[]) {
    Node* history = NULL;// cabeça historico
    FILE* file = NULL;
    char c;
    
    char args[TAMANHO_BUFFER_INICIAL];

    if(argc==2){
        flagBatchFile = 1;
        file = fopen(argv[1],"r");
        if (file == NULL) {
            fprintf(stderr, "Arquivo nulo.\n");
            exit(EXIT_FAILURE);   
        }
    }else if(argc>2){
        fprintf(stderr, "Muitos argumentos passados\n");
        exit(EXIT_FAILURE);
    }
    while (flagFuncionando==1) {  // Loop infinito
    // Imprime o prompt baseado no modo (sequencial ou paralelo)

        char* entrada=NULL;
        if(argc==1){

            if (flagModo == 1) {
                printf("vht par> ");
            } else {
                printf("vht seq> ");
            }


            int tamanho_buffer = TAMANHO_BUFFER_INICIAL;
            entrada = (char*)malloc(sizeof(char) * tamanho_buffer);

            if (entrada == NULL) {
                fprintf(stderr, "Erro ao alocar memória para a entrada.\n");
                exit(EXIT_FAILURE);
            }

        
            int indice = 0;

            // Lê caracteres da entrada até encontrar uma nova linha ou o final do arquivo
            while ((c = getchar()) != EOF && c != '\n') {
                // Realoca o buffer se necessário
                if (indice >= tamanho_buffer) {
                    tamanho_buffer += TAMANHO_BLOCO;
                    char* temp = (char*)realloc(entrada, tamanho_buffer);

                    if (temp == NULL) {
                        fprintf(stderr, "Erro ao realocar memória para a entrada.\n");
                        free(entrada);
                        exit(EXIT_FAILURE);
                    }
                    entrada = temp;
                }
                entrada[indice] = c;
                indice++;
            }   
            entrada[indice] = '\0';

        }else{
            int tamanho_buffer = TAMANHO_BUFFER_INICIAL;
            entrada = (char*)malloc(sizeof(char) * tamanho_buffer);

            if (entrada == NULL) {
                fprintf(stderr, "Erro ao alocar memória para a entrada.\n");
                exit(EXIT_FAILURE);
            }

           
            int indice = 0;

            // Lê caracteres da entrada até encontrar uma nova linha ou o final do arquivo
            while ((c = fgetc(file)) != EOF && c != '\n') {
                // Realoca o buffer se necessário
                if (indice >= tamanho_buffer) {
                    tamanho_buffer += TAMANHO_BLOCO;
                    char* temp = (char*)realloc(entrada, tamanho_buffer);

                    if (temp == NULL) {
                        fprintf(stderr, "Erro ao realocar memória para a entrada.\n");
                        free(entrada);
                        exit(EXIT_FAILURE);
                    }

                    entrada = temp;
                }

                entrada[indice] = c;
                indice++;
            }

            entrada[indice] = '\0';
        }

        // Checa se chegou ao final da entrada (EOF) e libera a memória
        if (feof(stdin)) {
            free(entrada);
            flagFuncionando = 0;
            break;
            //continue;
        }

        // Cria uma lista de argumentos separados por ponto e vírgula
        Node* head = NULL;
        Node* aux = NULL;
        

        //printf("imprimir historcioSUPERIOR\n");
        //imprimir(history);
       
        if(flagBatchFile==1){
            
            printf("%s\n",entrada);
        }

        char* token = strtok(entrada, ";");//token que divide e minhas entradas

        while (token != NULL) {
            //printf("o token[%d]: %s\n",teste,token);
            //tratamento das entradas---------------------------------------------------------------
            if(flagFuncionando==0){

            }
            if (strstr(token, "|")!=NULL){
                flagPipe = 1;

            }if (strstr(token, "&")!=NULL){
                flagBack= 1;
                                
            }
            if (strstr(token, "exit")!=NULL){// compara se o usuaŕio digitou exit
                flagFuncionando = 0;// desliga o funcionamento e vai faz os comandos e depois sai
                break;               
            }else if(strstr(token, "style parallel")!=NULL){
                flagModo = 1;
                //adicionarNode(&head, token, flagModo);
                token = strtok(NULL, ";");
                //teste++;
            
            }else if(strstr(token, "style sequential")!=NULL){
                flagModo = 0;
                //adicionarNode(&head, token, flagModo);
                token = strtok(NULL, ";");
                //teste++;
            }else if(strstr(token, "!!")!=NULL){
                if (history == NULL) {
                    printf("No commands\n");
                } else if(head == NULL){
                   copyLinkedList(history,&head);
                }else{
                // Copie os comandos do histórico para a lista de comandos
                    copiarListaNoFinal(&head,history);
                }
                token = strtok(NULL, ";");  // Avança para a próxima entrada
                //printf("Copiei o histórico para a lista atual.\n");
                
            }else if(strstr(token,">>")!=NULL){
                flagRedirecionamento = 2;
                adicionarNode(&head, token, flagModo,flagRedirecionamento,flagPipe, flagBack);
                token = strtok(NULL, ";");
                flagRedirecionamento = 0;
                flagPipe = 0;
                flagBack = 0;
            }else if(strstr(token,"<")!=NULL){
                flagRedirecionamento = 3;
                adicionarNode(&head, token, flagModo,flagRedirecionamento, flagPipe, flagBack);
                token = strtok(NULL, ";");
                flagRedirecionamento=0;
                flagPipe = 0;
                flagBack = 0;
                
            }else if(strstr(token,">")!=NULL){
                flagRedirecionamento = 1;
                adicionarNode(&head, token, flagModo,flagRedirecionamento,flagPipe,flagBack);
                token = strtok(NULL, ";");
                flagRedirecionamento=0; 
                flagPipe = 0;
                flagBack = 0;  
            }else{
                adicionarNode(&head, token, flagModo,flagRedirecionamento,flagPipe,flagBack);
                flagRedirecionamento=0;
                flagPipe = 0;
                flagBack = 0;

                token = strtok(NULL, ";");
            }
        }
        //if(flagHistory==1){


        //}
        
        //comecçar a execução___________________________________________________________________________
        aux = head;

        while(aux!=NULL){
            if(aux->flag==0){
                executarSequen(aux->arg,aux->flag,aux->flagRedirecionamento,aux->flagP,aux->flagBackground);
                aux=aux->prox;
                continue;
            }if(aux->flag==1){//--------------------------------------------------PARALELO
                executarPar(&aux);
                continue;
            }
        }
        if(flagPrimeiraVez!=0){
            liberarLista(history);
            history = NULL;
        }
        flagPrimeiraVez = 1; 
        //printf("copiei o historico\n");
        copyLinkedList(head,&history);
        //printf("imprimir head\n");
        //imprimir(head);
        liberarLista(head);
        //printf("imprimir historcio ATUAL DESSA RODADA\n");
        //imprimir(history);
        free(entrada);
        if(argc == 2){
            if(c==EOF){
            flagFuncionando =0;
            }
        }
        
    }
        if (file!=NULL)
        {
            fclose(file);
        }
                          

    return 0;

}

void imprimir(Node* head) {
    Node* atual = head;
    while (atual != NULL) {
        printf("%s ;", atual->arg);
        //printf("a flag foi:%d\n", atual->flag);
        atual = atual->prox;
    }
}

Node* inicializarNode(const char* arg,int flag,int redirecionamento, int pipe, int background) {
    // Aloca memória para um novo nó e inicializa seus campos
    Node* novoNode = (Node*)malloc(sizeof(Node));
    if (novoNode == NULL) {
        fprintf(stderr, "Erro ao alocar memória para o novo nó.\n");
        exit(EXIT_FAILURE);
    }

    // Copia o argumento para o nó
    novoNode->arg = strdup(arg);
    novoNode->flag = flag;
    novoNode->flagRedirecionamento = redirecionamento;
    novoNode->flagP = pipe;
    novoNode->flagBackground = background;
    if (novoNode->arg == NULL) {
        fprintf(stderr, "Erro ao alocar memória para o argumento do nó.\n");
        free(novoNode);
        exit(EXIT_FAILURE);
    }

    novoNode->prox = NULL;
    return novoNode;
}

void adicionarNode(Node** head, const char* arg, int flag, int redirecionamento, int pipe, int background) {
    // Adiciona um novo nó à lista
    Node* novoNode = inicializarNode(arg, flag,redirecionamento,pipe,background);

    if (*head == NULL) {
        *head = novoNode;
    } else {
        Node* atual = *head;
        while (atual->prox != NULL) {
            atual = atual->prox;
        }
        atual->prox = novoNode;
    }
}

void liberarLista(Node* head) {
    // Libera a memória alocada para a lista de nós
    Node* atual = head;
    Node* proximo;

    while (atual != NULL) {
        proximo = atual->prox;
        free(atual->arg);
        free(atual);
        atual = proximo;
    }
}

void copyLinkedList(Node* head, Node** history) {
    Node* current = head;
    Node* last = NULL;  // Para rastrear o último nó adicionado a history

    while (current != NULL) {
        Node* new_node = (Node*)malloc(sizeof(Node));
        if (new_node == NULL) {
            fprintf(stderr, "Erro: Falha na alocação de memória.\n");
            exit(EXIT_FAILURE);
        }

        new_node->arg = strdup(current->arg);
        new_node->flag = current->flag;
        new_node->flagP = current->flagP;
        new_node->flagRedirecionamento = current->flagRedirecionamento;
        new_node->prox = NULL;

        if (*history == NULL) {
            *history = new_node;
        } else {
            last->prox = new_node;
        }

        last = new_node;
        current = current->prox;
    }
}

void executarSequen(char* arg, int flag, int redirecionamento, int pipe,int background) {


    pid_t pid = fork();

    if( pid < 0){
        printf("erro");
    }else if(pid == 0){
        if((redirecionamento==1)||(redirecionamento==2)||(redirecionamento==3)){
            redirecionamentoFuncaoSeq(arg,redirecionamento);
        }else if(pipe==1){
            execPipeSeq(arg);
        }else{
        
        //printf("O pid:%d\n",getpid());
        execlp("sh","sh","-c",arg,NULL);
        }
     
    }else{
        wait(NULL);
    }
    
}



void* executarParalelo(void* arg) {
    const char* comando = (const char*)arg;
    system(comando);
    return NULL;
}


void* executarParaleloRedirecionamento(void* arg) {
    Node* node = (Node*) arg;
    char* comando = node->arg;
    char* parte1;
    char* parte2;
    FILE* file1;
    //AQUIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
    if (node->flagRedirecionamento == 1) {
        parte1 = strtok(comando, ">");
        parte2 = strtok(NULL, ">");
    if (parte2 != NULL) {
        // Remover espaços em branco extras antes do nome do arquivo
        while (*parte2 == ' ' || *parte2 == '\t') {
            parte2++;
        }
        // Remove espaços em branco no final do nome do arquivo
        size_t len = strlen(parte2);
        while (len > 0 && (parte2[len - 1] == ' ' || parte2[len - 1] == '\t')) {
            parte2[--len] = '\0';
        }
    }

    //printf("Comando: %s\n", parte1);
    //printf("Arquivo de redirecionamento: %s\n", parte2);

    

    // Abre o arquivo para redirecionamento
    file1 = fopen(parte2, "w+");
    if (file1 == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo para redirecionamento\n");
        exit(EXIT_FAILURE);
    }

    // Redirecionar stdout para o arquivo
    dup2(fileno(file1), STDOUT_FILENO);
    system(parte1);
    fclose(file1);
    freopen("/dev/tty", "a", stdout);
    }
    //aqui2222222222222222222222222222222222222222222222222222222222222222222222222222
    else if(node->flagRedirecionamento == 2){
        parte1 = strtok(comando, ">>");
        parte2 = strtok(NULL, ">>");
        if (parte2 != NULL) {
            // Remover espaços em branco extras antes do nome do arquivo
            while (*parte2 == ' ' || *parte2 == '\t') {
                parte2++;
            }
            // Remove espaços em branco no final do nome do arquivo
            size_t len = strlen(parte2);
            while (len > 0 && (parte2[len - 1] == ' ' || parte2[len - 1] == '\t')) {
                parte2[--len] = '\0';
            }
        }

        //printf("Comando: %s\n", parte1);
        //printf("Arquivo de redirecionamento: %s\n", parte2);

        // Abre o arquivo para redirecionamento
        file1 = fopen(parte2, "a+");
        if (file1 == NULL) {
            fprintf(stderr, "Erro ao abrir o arquivo para redirecionamento\n");
            exit(EXIT_FAILURE);
            
        }

        // Redirecionar stdout para o arquivo
        dup2(fileno(file1), STDOUT_FILENO);
        system(parte1);
        fclose(file1);
        freopen("/dev/tty", "a", stdout);
        
    }
    //aqui 333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333
    else if(node->flagRedirecionamento == 3){
        parte1 = strtok(comando, "<");
        parte2 = strtok(NULL, "<");
        if (parte2 != NULL) {
            // Remover espaços em branco extras antes do nome do arquivo
            while (*parte2 == ' ' || *parte2 == '\t') {
                parte2++;
            }
            // Remove espaços em branco no final do nome do arquivo
            size_t len = strlen(parte2);
            while (len > 0 && (parte2[len - 1] == ' ' || parte2[len - 1] == '\t')) {
                parte2[--len] = '\0';
            }
        }

        //printf("Comando: %s\n", parte1);
        //printf("Arquivo de redirecionamento: %s\n", parte2);

        // Abre o arquivo para redirecionamento
        file1 = fopen(parte2, "r");
        if (file1 == NULL) {
            fprintf(stderr, "Erro ao abrir o arquivo para redirecionamento\n");
            exit(EXIT_FAILURE);
        }
       
       // Redireciona stdin para o arquivo
        freopen(parte2, "r", stdin);
        
        // Executa o comando
        system(parte1);
        
        // Redireciona stdin de volta para o terminal
        freopen("/dev/tty", "r", stdin);
        
        // Fecha o arquivo
        fclose(file1);
    }
    return NULL;

}


void executarPar(Node**aux) {
    Node* passador;
    passador = *aux;
    int cont = 0;
    while ((passador!=NULL)&&(passador->flag!=0)){
        cont++;
        passador=passador->prox;
        //printf("cont:%d\n",cont);
    }
    
    
    pthread_t thread[cont];

    for(int i = 0;i<cont;i++){
    // Cria uma thread para executar o comando em paralelo usando system
        if(((*aux)->flagRedirecionamento==1)||((*aux)->flagRedirecionamento==2)||((*aux)->flagRedirecionamento==3)){
            int result = pthread_create(&thread[i], NULL, executarParaleloRedirecionamento, &((*aux)->arg));
            
            if (result != 0){
                fprintf(stderr, "Erro ao criar a thread para execução em paralelo.\n");
                exit(EXIT_FAILURE);
            // Lidere com o erro de criação de thread conforme necessário
            }
            *aux=(*aux)->prox;

        }else if((*aux)->flagP==1){
            int result = pthread_create(&thread[i], NULL, execPipePar, &((*aux)->arg));
            if (result != 0){
                fprintf(stderr, "Erro ao criar a thread para execução em paralelo.\n");
                exit(EXIT_FAILURE);
            // Lidere com o erro de criação de thread conforme necessário
            }
            *aux=(*aux)->prox;
            
        }else{
            int result = pthread_create(&thread[i], NULL, executarParalelo, (*aux)->arg);
            if (result != 0){
            fprintf(stderr, "Erro ao criar a thread para execução em paralelo.\n");
            exit(EXIT_FAILURE);
            // Lidere com o erro de criação de thread conforme necessário
            }
            *aux=(*aux)->prox;
        } 
    }
        
    // Aguarde a thread concluir antes de continuar
    for(int i = 0; i< cont;i++){
        if (pthread_join(thread[i], NULL) != 0) {
            fprintf(stderr, "Erro ao aguardar a thread.\n");
            // Lidere com o erro de join da thread conforme necessário
        }
    }
}

void redirecionamentoFuncaoSeq(char* comando, int flagRedirecionamento) {
    char* parte1 = NULL;
    char* parte2 = NULL;
    FILE* file1;
    

    if (flagRedirecionamento == 1) {
        parte1 = strtok(comando, ">");
        parte2 = strtok(NULL, ">");
        if (parte2 != NULL) {
            // Remover espaços em branco extras antes do nome do arquivo
            while (*parte2 == ' ' || *parte2 == '\t') {
                parte2++;
            }
            // Remove espaços em branco no final do nome do arquivo
            size_t len = strlen(parte2);
            while (len > 0 && (parte2[len - 1] == ' ' || parte2[len - 1] == '\t')) {
                parte2[--len] = '\0';
            }
        }

        //printf("Comando: %s\n", parte1);
        //printf("Arquivo de redirecionamento: %s\n", parte2);

        // Abre o arquivo para redirecionamento
        file1 = fopen(parte2, "w+");
        if (file1 == NULL) {
            fprintf(stderr, "Erro ao abrir o arquivo para redirecionamento\n");
            exit(EXIT_FAILURE);
        }

        // Redirecionar stdout para o arquivo
        dup2(fileno(file1), STDOUT_FILENO);
        //printf("voce escreveu:%s\n",parte1);
        execlp("sh","sh","-c",parte1,NULL);
        fclose(file1);

    }else if(flagRedirecionamento == 2){
        parte1 = strtok(comando, ">>");
        parte2 = strtok(NULL, ">>");
        if (parte2 != NULL) {
            // Remover espaços em branco extras antes do nome do arquivo
            while (*parte2 == ' ' || *parte2 == '\t') {
                parte2++;
            }
            // Remove espaços em branco no final do nome do arquivo
            size_t len = strlen(parte2);
            while (len > 0 && (parte2[len - 1] == ' ' || parte2[len - 1] == '\t')) {
                parte2[--len] = '\0';
            }
        }

        //printf("Comando: %s\n", parte1);
        //printf("Arquivo de redirecionamento: %s\n", parte2);

        // Abre o arquivo para redirecionamento
        file1 = fopen(parte2, "a+");
        if (file1 == NULL) {
            fprintf(stderr, "Erro ao abrir o arquivo para redirecionamento\n");
            exit(EXIT_FAILURE);
        }

        // Redirecionar stdout para o arquivo
        dup2(fileno(file1), STDOUT_FILENO);
        //printf("voce escreveu:%s\n",parte1);
        execlp("sh","sh","-c",parte1,NULL);
        fclose(file1);

    }else if(flagRedirecionamento == 3){
        parte1 = strtok(comando, "<");
        parte2 = strtok(NULL, "<");
        if (parte2 != NULL) {
            // Remover espaços em branco extras antes do nome do arquivo
            while (*parte2 == ' ' || *parte2 == '\t') {
                parte2++;
            }
            // Remove espaços em branco no final do nome do arquivo
            size_t len = strlen(parte2);
            while (len > 0 && (parte2[len - 1] == ' ' || parte2[len - 1] == '\t')) {
                parte2[--len] = '\0';
            }
        }

        //printf("Comando: %s\n", parte1);
        //printf("Arquivo de redirecionamento: %s\n", parte2);

        // Abre o arquivo para redirecionamento
        file1 = fopen(parte2, "r");
        if (file1 == NULL) {
            fprintf(stderr, "Erro ao abrir o arquivo para redirecionamento\n");
            exit(EXIT_FAILURE);
        }
        // Redirecionar stdin para o arquivo
        dup2(fileno(file1), STDIN_FILENO);
        //printf("voce escreveu:%s\n",parte1);
        execlp("sh","sh","-c",parte1,NULL);
        fclose(file1);
    }

}

void execPipeSeq(char* comando){
    char* parte1 = NULL;//comando
    char* parte2 = NULL;//modificador
    
    parte1 = strtok(comando, "|");
    parte2 = strtok(NULL, "|");

    if (parte1==NULL) {
        fprintf(stderr, "Syntax error. Near unexpected token `|'\n");
        return;
    }
    if (parte2==NULL) {
        fprintf(stderr, "Syntax error. Esperava-se algo depois do `|'\n");
        return;
    }
    printf("Comando: %s\n", parte1);
    printf("Modificador: %s\n", parte2);  

    

    int fd[2];  // Array para os descritores do pipe

    if (pipe(fd) == -1) {
        perror("Erro ao criar o pipe");
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Erro ao criar o processo filho");
        return;
        } else if (pid == 0) {  // Código do processo filho
        close(fd[0]);  // Fecha a leitura do pipe, pois não será usada

        // Redireciona a saída padrão para o descritor de escrita do pipe
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);  // Fecha o descritor de escrita do pipe

        // Executa o primeiro comando
        execlp("sh", "sh", "-c", parte1, NULL);

        // Se o execlp falhar, imprime uma mensagem de erro
        //perror("Erro ao executar o comando antes do pipe");
        //exit(EXIT_FAILURE);
        } else {  // Código do processo pai
        close(fd[1]);  // Fecha a escrita do pipe, pois não será usada

        // Redireciona a entrada padrão para o descritor de leitura do pipe
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);  // Fecha o descritor de leitura do pipe

        // Executa o segundo comando
        execlp("sh", "sh", "-c", parte2, NULL);

        // Se o execlp falhar, imprime uma mensagem de erro
        //fprintf(stderr, "Erro ao abrir o arquivo para redirecionamento\n");
        //exit(EXIT_FAILURE);
    } 

}

void* execPipePar(void* arg) {
    Node* node = (Node*)arg;
    char* comando = node->arg;

    // Divide o comando em duas partes usando o pipe como delimitador
    char* parte1 = strtok(comando, "|");
    char* parte2 = strtok(NULL, "|");

    // Remove espaços extras antes e depois dos comandos
    char* comando1 = strtok(parte1, " ");
    char* comando2 = strtok(parte2, " ");

    if (comando1 == NULL || comando2 == NULL) {
        fprintf(stderr, "Syntax error. Near unexpected token `|'\n");
        return NULL;
    }

    int fd[2]; // Descritores de arquivo do pipe
    if (pipe(fd) == -1) {
        fprintf(stderr, "pipe\n");
        exit(EXIT_FAILURE);
    }

    int pid = fork(); // Cria um novo processo

    if (pid == -1) {
        fprintf(stderr, "fork\n");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Processo filho
        close(fd[0]); // Fecha a extremidade de leitura do pipe

        // Redireciona a saída para o pipe
        dup2(fd[1], STDOUT_FILENO);

        // Fecha a extremidade de escrita do pipe
        close(fd[1]);

        // Executa o primeiro comando usando system
        system(comando1);
        
        
    } else { // Processo pai
        close(fd[1]); // Fecha a extremidade de escrita do pipe

        // Redireciona a entrada para o pipe
        dup2(fd[0], STDIN_FILENO);

        // Fecha a extremidade de leitura do pipe
        close(fd[0]);

        // Executa o segundo comando usando system
        
        system(comando2);
        
        freopen("/dev/tty", "r", stdin);
    }

    return NULL;
    
}

// Função para copiar uma lista no final de outra
void copiarListaNoFinal(Node** destino, Node* origem) {
    if (*destino == NULL) {
        *destino = origem;
        return;
    }

    // Encontrar o último nó da lista de destino
    Node* ultimoDestino = *destino;
    while (ultimoDestino->prox != NULL)
        ultimoDestino = ultimoDestino->prox;

    // Adicionar cada nó da lista de origem ao final da lista de destino
    while (origem != NULL) {
        Node* novoNode = (Node*)malloc(sizeof(Node));
        if (novoNode == NULL) {
            fprintf(stderr, "Erro: falha ao alocar memória para o nó.\n");
            exit(EXIT_FAILURE);
            
        }

        novoNode->arg = strdup(origem->arg);
        novoNode->flag = origem->flag;
        novoNode->flagRedirecionamento = origem->flagRedirecionamento;
        novoNode->flagP = origem->flagP;
        novoNode->prox = NULL;

        ultimoDestino->prox = novoNode;
        ultimoDestino = ultimoDestino->prox;

        origem = origem->prox;
    }
}