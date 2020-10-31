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
	unsigned long time;
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
	unsigned long time;
	char user[50];
	char subject[50];
	char message[500];
	struct data *prox;
};

struct dataShare {
	int codigo;
	struct mensagemUsuario msg;
};

int ret;
int paiPid;

int inserirNoFim(struct data **, struct mensagemUsuario);
void deletaArquivo(struct data**, char *);
void retornaCriados(struct data **);
struct mensagemUsuario encontrarArquivo(struct data *, struct mensagemUsuario, int op);
void atualizaArquivo(struct data**, struct mensagemUsuario);
void* create_shared_memory(size_t);
void recebeDados();
void enviaAtualizacao(struct mensagemUsuario);
void sincronizaRecebimento(struct data**, struct mensagemUsuario);

int sock;
void* shmem;
struct sockaddr_in cli_send;
struct mensagemUsuario msg;
struct sockaddr_in server1;

pthread_mutex_t lock;

int main()
{
	int length;
	struct sockaddr_in name;
	char buf[1024];
	struct mensagemServer msgServer;
	struct mensagemUsuario auxUsuario;
	struct dataShare dataShare, *d;
	struct data *raiz = NULL;
	struct hostent *hp, *gethostbyname();	

	pthread_mutex_unlock(&lock);

  	/* Cria o socket de comunicacao */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock<0) {
	/* houve erro na abertura do socket	*/
		perror("[Server2] Opening datagram socket");
		exit(1);
	}
	/* Associa porta */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(6548);
  
	if (bind(sock,(struct sockaddr *)&name, sizeof name ) < 0) {
		perror("[Server2] Binding datagram socket");
		exit(1);
	}
  
  	/* Imprime o numero da porta */
	length = sizeof(name);
	if (getsockname(sock,(struct sockaddr *)&name, &length) < 0) {
		perror("[Server2] Getting socket name");
		exit(1);
	}
	printf("[Server2] Socket port #%d\n", ntohs(name.sin_port));

	/* Preparando endereco server 1 */
	printf("[Server2] Preparando para receber!\n");

	hp = gethostbyname("localhost"); //chama dns
	if (hp==0) {
		fprintf(stderr, "[Server2 Receiver] Localhost unknown host ");
      	exit(2);
  	}

	bcopy ((char *)hp->h_addr, (char *)&server1.sin_addr, hp->h_length);
	server1.sin_family = AF_INET;
	server1.sin_port = htons(9876); 

	int cont=0;
  	paiPid = getpid();
  
  	dataShare.codigo = -1;
	shmem = create_shared_memory(sizeof(struct dataShare));
  	memcpy(shmem, &dataShare, sizeof(dataShare));

	/* Fork se for o filho vai pra ficar em loop infinito atualizando o pai */
	if(fork() == 0){
		recebeDados();
	}

  	/* Le */
	printf("[Server2] Starting iteration\n", getpid());
  	while (1)
	{
		/* Recebe o request */
		if (recvfrom(sock,(char *)&msgServer,sizeof(msgServer),0,(struct sockaddr *)&name, &length) < 0)
			perror("[Server2] Receiving datagram packet");

		printf("[Server2] Verificando se existe algum dado compartilhado\n");
		pthread_mutex_lock(&lock);
		d = shmem;
		if(d->codigo == 1){
			printf("[Server2] Existe, atualizando!\n");
			sincronizaRecebimento(&raiz, d->msg);
			d->codigo = -1;
			memcpy(shmem, &d, sizeof(d));
		}
		pthread_mutex_unlock(&lock);

		/* Exibe info de quem fez o request */
		//printf("[Server2] name.sin_family: %i\n", name.sin_family);
		//printf("[Server2] name.sin_addr: %s\n", inet_ntoa(name.sin_addr));
		//printf("[Server2] name.sin_port: %i\n", name.sin_port);
		//printf("[Server2] Codigo: %i\n", msg.codigo);
		msg = msgServer.msg;
		printf("[Server2] Codigo recebido: %i\n", msg.codigo);
		if(msg.codigo == 9){
			printf("[Server2] Codigo para auto destruicao\n");
			break;
		}

		//ret = fork();
		//Se for processo pai volta e aguarda as proximas requisicoes
		//if(ret > 0)
		//continue;
		
		//printf("[Server2] Sou filho novo [%i]\n", getpid());

		//Se for = 0 significa que é filho portanto executa o pedido
		/* Associa para resposta */
		cli_send.sin_family =      msgServer.client.sin_family;          //name.sin_family;
		cli_send.sin_addr.s_addr = msgServer.client.sin_addr.s_addr;     //name.sin_addr.s_addr;
		cli_send.sin_port =        msgServer.client.sin_port;            //name.sin_port;

		switch (msg.codigo){
		/* LER */
		case 1:
			printf("[Server2] Ler arquivo\n");
			msg = encontrarArquivo(raiz, msg, READ_FILE);
			break;
		/* CRIAR */
		case 2:
			auxUsuario = msg;
			printf("[Server2] Criar arquivo (%s)\n", msg.subject);
			msg.codigo = inserirNoFim(&raiz, msg);
			enviaAtualizacao(auxUsuario);
			break;
		/* EDITAR */
		case 3:
			auxUsuario = msg;
			printf("[Server2] Editar arquivo (%s)\n", msg.subject);
			msg = encontrarArquivo(raiz, msg, EDIT_FILE);
			enviaAtualizacao(auxUsuario);
			break;
		/* EXCLUIR */
		case 4:
			auxUsuario = msg;
			printf("[Server2] Excluir arquivo\n");
			deletaArquivo(&raiz, msg.subject);
			enviaAtualizacao(auxUsuario);
			break;
		/* VER CRIADOS */
		case 5:
			printf("[Server2] Ver arquivos\n");
			retornaCriados(&raiz);
			continue;
		case 6:
			auxUsuario = msg;
			printf("[Server2] Editar arquivo\n");
			atualizaArquivo(&raiz, msg);
			enviaAtualizacao(auxUsuario);
			continue;
		default:
			printf("[Server2] Opcao nao encontrada\n");
			break;
		}

		/* Associa para resposta */
		//cli_send.sin_family =      msgServer.client.sin_family;          //name.sin_family;
		//cli_send.sin_addr.s_addr = msgServer.client.sin_addr.s_addr;     //name.sin_addr.s_addr;
		//cli_send.sin_port =        msgServer.client.sin_port;            //name.sin_port;

		//printf("[Server2] name.sin_family: %i\n", cli_send.sin_family);
		//printf("[Server2] name.sin_addr: %s\n", inet_ntoa(cli_send.sin_addr));
		//printf("[Server2] name.sin_port: %i\n", cli_send.sin_port);
		//msg.porta = 67213/*name.sin_port*/;
		//strcpy(msg.ip, inet_ntoa(name.sin_addr));

		printf("[Server2] Devolvendo resultado\n");
		/* Envia o dado */
		if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
			perror("[Server2] Sending datagram message");
	}

	/*if(ret != 0){
		printf("[Server %i] Acabou, fiz tudo que eu podia =( \n", getpid());
		wait();
		printf("[Server2] Todos filhos mortos\n");
		close(sock);
		exit(0);	
	}else{*/
		printf("[Server2] Morri %i\n", getpid());
		exit(0);	
	//}
}

int inserirNoFim(struct data **raiz, struct mensagemUsuario msg){
    int ret;
	struct data *aux = *raiz;
	if(*raiz != NULL){
		printf("[Server2] File: %s Name: %s\n", aux->subject, msg.subject);
		ret = strcmp(aux->subject, msg.subject);
		if(ret == 0)
			return ERROR_NAME_IN_USE;
		while(aux->prox != NULL){
			aux = aux->prox;
			printf("[Server2] File: %s Name: %s\n", aux->subject, msg.subject);
			ret = strcmp(aux->subject, msg.subject);
			if(ret == 0)
				return ERROR_NAME_IN_USE;
		}
	}
	
	struct data *novo;
    novo = (struct data *) malloc(sizeof(struct data));
    if(novo == NULL) exit(0);
    strcpy(novo->subject, msg.subject);
	strcpy(novo->user, msg.user);
	novo->time = msg.time;
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

	printf("[Server2] Retorna Criados\n");

	if(*raiz == NULL){
		printf("[Server2] Raiz nula!\n");
		count = 0;
	}else{
		aux = *raiz;
		count = 1;
		while(aux->prox != NULL){
			printf("[Server2] Raiz não nula!\n");
			aux = aux->prox;
			count++;
		}
	}

	printf("[Server2] Existe %i arquivos criados\n", count);

	msg.codigo = CREATED_FILES;
	if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
		perror("[Server2] Sending datagram message");	

	printf("[Server2] READ_FILES enviado, enviando quantia\n");
	msg.codigo = count;
	if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
		perror("[Server2] Sending datagram message");

	aux = *raiz;

	for(int i = 0; i < count; i++){
		msg.codigo = i;
		strcpy(msg.subject, aux->subject);
		if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
			perror("[Server2] Sending datagram message");
		aux = aux -> prox;
	}
}

struct mensagemUsuario encontrarArquivo(struct data *raiz, struct mensagemUsuario r, int op){
	struct mensagemUsuario msg;
	msg.codigo = FILE_NOT_FOUND;
	if(raiz == NULL){
		msg.codigo = ERROR_EMPTY_DATABASE;
	}else{
		while(1){
			if(strcmp(raiz->subject, r.subject) == 0){
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
		printf("[Server2] Raiz nula, recriando!\n");
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
				if(aux->time < msg.time){
					strcpy(aux->message, msg.message);
					strcpy(aux->user, msg.user);
					aux->time = msg.time;
					printf("[Server2] Encontrado, atualizando! %s %s \n", aux->user, msg.user);
					return;
				}
			}
			aux = aux -> prox;
		}while(aux != NULL);

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

		printf("[Server2] Nao existe mais, adicionando no fim!");
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

void* create_shared_memory(size_t size) {
  int protection = PROT_READ | PROT_WRITE;
  int visibility = MAP_SHARED | MAP_ANONYMOUS;
  return mmap(NULL, size, protection, visibility, -1, 0);
}

void recebeDados(){
	int codigo, sockReceiver;
	struct sockaddr_in name;
	struct dataShare dataShare, *dataShareAux;

	/* Cria o socket de comunicacao */
	sockReceiver = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockReceiver<0) {
	/* houve erro na abertura do socket	*/
		perror("[Server2 Receiver] Opening datagram socket");
		exit(1);
	}
	/* Associa porta */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(8765);
  
	if (bind(sockReceiver,(struct sockaddr *)&name, sizeof name ) < 0) {
		perror("[Server2 Receiver] Binding datagram socket");
		exit(1);
	}
  
  	/* Imprime o numero da porta */
	socklen_t length = sizeof(name);
	if (getsockname(sockReceiver,(struct sockaddr *)&name, &length) < 0) {
		perror("[Server2 Receiver] Getting socket name");
		exit(1);
	}
	printf("[Server2 Receiver] Receiver port #%d\n", ntohs(name.sin_port));

	while(1){
		/* Recebe o request */
		if (recvfrom(sockReceiver,(char *)&dataShare,sizeof(dataShare),0,(struct sockaddr *)&name, &length) < 0)
			perror("[Server2 Receiver] Receiving datagram packet");

		printf("[Server1 Receiver] Novo arquivo recebido\n");

		while(1){
			pthread_mutex_lock(&lock);
			dataShareAux = shmem;
			if(dataShareAux->codigo != 1){
				codigo = dataShareAux->codigo;
				memcpy(shmem, &dataShare, sizeof(dataShare));
				break;
			}
			pthread_mutex_unlock(&lock);
			sleep(1);
		}
		pthread_mutex_unlock(&lock);
		if(codigo == 9)
			exit(0);
	}
}

void enviaAtualizacao(struct mensagemUsuario msg){
	struct dataShare data;
	data.codigo = 1;
	data.msg = msg;
	if (sendto (sock,(char *)&data,sizeof data, 0, (struct sockaddr *)&server1, sizeof server1) < 0)
			perror("[Server2] Sending datagram message");
	printf("[Server2] Alteracao enviada!\n");
}

void sincronizaRecebimento(struct data**raiz, struct mensagemUsuario msg){
	struct data *aux = *raiz;

	switch(msg.codigo){
		/* EXCLUIR */ 
		case 4:
			deletaArquivo(raiz, msg.subject);
			break;
		case 2:
		case 6:
			atualizaArquivo(raiz, msg);
			break;
	}
}