#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define READ_FILE 3
#define EDIT_FILE 2
#define CREATED_FILES 1
#define CREATE_SUCCESS 0
#define ERROR_NAME_IN_USE -1
#define FILE_NOT_FOUND -2
#define ERROR_EMPTY_DATABASE -3

struct mensagemUsuario {
	int time;
	int codigo;
	char user[50];
	char subject[50];
	char message[500];
};

struct mensagemServer {
	struct mensagemUsuario msg;
	struct sockaddr_in client;
};

struct data {
	int time;
	char user[50];
	char subject[50];
	char message[500];
	struct data *prox;
};

int ret;
int paiPid;

int inserirNoFim(struct data **, char *);
void deletaArquivo(struct data**, char *);
void retornaCriados(struct data **);
struct mensagemUsuario encontrarArquivo(struct data *, char *, int);
void atualizaArquivo(struct data**, struct mensagemUsuario);

int sock;
struct sockaddr_in cli_send;
struct mensagemUsuario msg;

int main()
{
	int length;
	struct sockaddr_in name;
	char buf[1024];
	struct mensagemServer msgServer;
	struct data *raiz = NULL;

  /* Cria o socket de comunicacao */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock<0) {
	/* houve erro na abertura do socket	*/
		perror("[Server] Opening datagram socket");
		exit(1);
	}
	/* Associa porta */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(6969);
  
	if (bind(sock,(struct sockaddr *)&name, sizeof name ) < 0) {
		perror("[Server] Binding datagram socket");
		exit(1);
	}
  
  	/* Imprime o numero da porta */
	length = sizeof(name);
	if (getsockname(sock,(struct sockaddr *)&name, &length) < 0) {
		perror("[Server] Getting socket name");
		exit(1);
	}
	printf("[Server] Socket port #%d\n", ntohs(name.sin_port));

	int cont=0;
  	paiPid = getpid();
  
  	/* Le */
	printf("[Server %i] Starting iteration\n", getpid());
  	while (1)
	{
		/* Recebe o request */
		if (recvfrom(sock,(char *)&msgServer,sizeof(msgServer),0,(struct sockaddr *)&name, &length) < 0)
			perror("[Server] Receiving datagram packet");

		/* Exibe info de quem fez o request */
		//printf("[Server] name.sin_family: %i\n", name.sin_family);
		//printf("[Server] name.sin_addr: %s\n", inet_ntoa(name.sin_addr));
		//printf("[Server] name.sin_port: %i\n", name.sin_port);
		//printf("[Server] Codigo: %i\n", msg.codigo);
		msg = msgServer.msg;
		printf("[Server] Codigo recebido: %i\n", msg.codigo);
		if(msg.codigo == 9){
			printf("[Server] Codigo para auto destruicao\n");
			break;
		}

		//ret = fork();
		//Se for processo pai volta e aguarda as proximas requisicoes
		//if(ret > 0)
		//continue;
		
		//printf("[Server] Sou filho novo [%i]\n", getpid());

		//Se for = 0 significa que é filho portanto executa o pedido
		/* Associa para resposta */
		cli_send.sin_family =      msgServer.client.sin_family;          //name.sin_family;
		cli_send.sin_addr.s_addr = msgServer.client.sin_addr.s_addr;     //name.sin_addr.s_addr;
		cli_send.sin_port =        msgServer.client.sin_port;            //name.sin_port;

		switch (msg.codigo){
		/* LER */
		case 1:
			printf("[Server] Ler arquivo\n");
			msg = encontrarArquivo(raiz, msg.subject, READ_FILE);
			break;
		/* CRIAR */
		case 2:
			printf("[Server] Criar arquivo (%s)\n", msg.subject);
			msg.codigo = inserirNoFim(&raiz, msg.subject);
			break;
		/* EDITAR */
		case 3:
			printf("[Server] Editar arquivo (%s)\n", msg.subject);
			msg = encontrarArquivo(raiz, msg.subject, EDIT_FILE);
			break;
		/* EXCLUIR */
		case 4:
			printf("[Server] Excluir arquivo\n");
			deletaArquivo(&raiz, msg.subject);
			break;
		/* VER CRIADOS */
		case 5:
			printf("[Server] Ver arquivos\n");
			retornaCriados(&raiz);
			continue;
		case 6:
			printf("[Server] Editar arquivo\n");
			atualizaArquivo(&raiz, msg);
			continue;
		default:
			printf("[Server] Opcao nao encontrada\n");
			break;
		}

		/* Associa para resposta */
		//cli_send.sin_family =      msgServer.client.sin_family;          //name.sin_family;
		//cli_send.sin_addr.s_addr = msgServer.client.sin_addr.s_addr;     //name.sin_addr.s_addr;
		//cli_send.sin_port =        msgServer.client.sin_port;            //name.sin_port;

		//printf("[Server] name.sin_family: %i\n", cli_send.sin_family);
		//printf("[Server] name.sin_addr: %s\n", inet_ntoa(cli_send.sin_addr));
		//printf("[Server] name.sin_port: %i\n", cli_send.sin_port);
		//msg.porta = 67213/*name.sin_port*/;
		//strcpy(msg.ip, inet_ntoa(name.sin_addr));

		printf("[Server] Devolvendo resultado\n");
		/* Envia o dado */
		if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
			perror("[Server] Sending datagram message");
	}

	/*if(ret != 0){
		printf("[Server %i] Acabou, fiz tudo que eu podia =( \n", getpid());
		wait();
		printf("[Server] Todos filhos mortos\n");
		close(sock);
		exit(0);	
	}else{*/
		printf("[Server] Morri %i\n", getpid());
		exit(0);	
	//}
}

int inserirNoFim(struct data **raiz, char *subject){
    int ret;
	struct data *aux = *raiz;
	if(*raiz != NULL){
		printf("[Server] File: %s Name: %s\n", aux->subject, subject);
		ret = strcmp(aux->subject, subject);
		if(ret == 0)
			return ERROR_NAME_IN_USE;
		while(aux->prox != NULL){
			aux = aux->prox;
			printf("[Server] File: %s Name: %s\n", aux->subject, subject);
			ret = strcmp(aux->subject, subject);
			if(ret == 0)
				return ERROR_NAME_IN_USE;
		}
	}
	
	struct data *novo;
    novo = (struct data *) malloc(sizeof(struct data));
    if(novo == NULL) exit(0);
    strcpy(novo->subject, subject);
    novo->prox = NULL;
    
    if(*raiz == NULL){
        *raiz = novo;
    }else{
        struct data *aux;
        aux = *raiz;
        while(aux->prox != NULL){
            aux = aux->prox;
        }
        aux->prox = novo;
    }
	return CREATE_SUCCESS;
}

void retornaCriados(struct data **raiz){
	struct data *aux = NULL;
	//struct mensagemUsuario msg;
	int count;

	printf("[Server] Retorna Criados\n");

	if(*raiz == NULL){
		printf("[Server] Raiz nula!\n");
		count = 0;
	}else{
		aux = *raiz;
		count = 1;
		while(aux->prox != NULL){
			printf("[Server] Raiz não nula!\n");
			aux = aux->prox;
			count++;
		}
	}

	printf("[Server] Existe %i arquivos criados\n", count);

	msg.codigo = CREATED_FILES;
	if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
		perror("[Server] Sending datagram message");	

	printf("[Server] READ_FILES enviado, enviando quantia\n");
	msg.codigo = count;
	if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
		perror("[Server] Sending datagram message");

	aux = *raiz;

	for(int i = 0; i < count; i++){
		msg.codigo = i;
		strcpy(msg.subject, aux->subject);
		if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
			perror("[Server] Sending datagram message");
		aux = aux -> prox;
	}
}

struct mensagemUsuario encontrarArquivo(struct data *raiz, char *subject, int op){
	struct mensagemUsuario msg;
	msg.codigo = FILE_NOT_FOUND;
	if(raiz == NULL){
		msg.codigo = ERROR_EMPTY_DATABASE;
	}else{
		while(1){
			if(strcmp(raiz->subject, subject) == 0){
				strcpy(msg.subject, raiz->subject);
				strcpy(msg.message, raiz->message);
				strcpy(msg.user, raiz->user);
				msg.time = raiz->time;
				msg.codigo = op;
				break;
			}
			if(raiz->prox == NULL)
				break;
			raiz = raiz->prox;
		}
	}
	return msg;
}

void atualizaArquivo(struct data** raiz, struct mensagemUsuario msg){
	struct data *aux = *raiz;
	struct data *novo;

	if(*raiz == NULL){
		printf("[Server] Raiz nula, recriando!\n");
		novo = (struct data *) malloc(sizeof(struct data));
		if(novo == NULL) exit(0);
		strcpy(novo->subject, msg.subject);
		strcpy(novo->message, msg.message);
		strcpy(novo->user, msg.user);
		novo->time = msg.time;
		novo->prox = NULL;
		*raiz = novo;
	} else {
		do{
			if(strcmp(aux->subject, msg.subject) == 0){
				strcpy(aux->message, msg.message);
				strcpy(aux->user, msg.user);
				aux->time = msg.time;
				printf("[Server] Encontrado, atualizando! %s %s \n", aux->user, msg.user);
				return;
			}
			aux = aux -> prox;
		}while(aux != NULL);
		
		printf("ta foda\n");

		aux = *raiz;

		while(aux -> prox != NULL) 
			aux = aux -> prox;
		
		novo = (struct data *) malloc(sizeof(struct data));
		if(novo == NULL) exit(0);
		strcpy(novo->subject, msg.subject);
		strcpy(novo->message, msg.message);
		strcpy(novo->user, msg.user);
		novo->time = msg.time;
		novo->prox = NULL;
		aux->prox = novo;

		printf("[Server] Nao existe mais, adicionando no fim!");
	}
}

void deletaArquivo(struct data **raiz, char *subject){
	if(*raiz == NULL)
		return;

	struct data *aux = *raiz;
	struct data *ant;

	if(strcmp((*raiz)->subject, subject)==0){
		*raiz = (*raiz)->prox;
		return;
	}

	ant = aux;
	aux = aux->prox;

	do{
		if(strcmp(aux->subject, subject) == 0){
			ant->prox = aux->prox;
			break;
		}
		ant = aux;
		aux = aux->prox;
	}while(aux != NULL);
}