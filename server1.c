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
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>

#define READ_FILE 3
#define EDIT_FILE 2
#define CREATED_FILES 1
#define CREATE_SUCCESS 0
#define ERROR_NAME_IN_USE -1
#define FILE_NOT_FOUND -2
#define ERROR_EMPTY_DATABASE -3

#define MAX_DEFAULT_SIZE 50
#define MAX_MESSAGE_SIZE 500

struct mensagemUsuario {
	unsigned long time;
	int codigo;
	char user[MAX_DEFAULT_SIZE];
	char subject[MAX_DEFAULT_SIZE];
	char message[MAX_MESSAGE_SIZE];
};

struct mensagemServer {
	struct mensagemUsuario msg;
	struct sockaddr_in client;
};

struct data {
	unsigned long time;
	char user[MAX_DEFAULT_SIZE];
	char subject[MAX_DEFAULT_SIZE];
	char message[MAX_MESSAGE_SIZE];
	struct data *prox;
};

struct dataShare {
	int codigo;
	struct mensagemUsuario msg;
};

int ret;

int inserirNoFim(struct data **, struct mensagemUsuario);
void deletaArquivo(struct data**, char *);
void retornaCriados(struct data **);
struct mensagemUsuario encontrarArquivo(struct data *, struct mensagemUsuario, int op);
void atualizaArquivo(struct data**, struct mensagemUsuario);
void recebeDados();
void* create_shared_memory(size_t);
void enviaAtualizacao(struct mensagemUsuario);
void sincronizaRecebimento(struct data**, struct mensagemUsuario);
void addLog(char*);
void loadSaveState(struct data**);
void saveState(struct data *);
void compartilhaSave(struct data *);
void getTime(char *);

int sock;
void* shmem;
struct sockaddr_in cli_send;
struct mensagemUsuario msg;
struct sockaddr_in server2;
char folder[MAX_DEFAULT_SIZE];
char buf[1024];

pthread_mutex_t lock, lockLog;

int main()
{
	int length;
	struct sockaddr_in name;
	struct dataShare dataShare, *d;
	struct mensagemUsuario auxUsuario;
	struct mensagemServer msgServer;
	struct data *raiz = NULL;
	struct hostent *hp, *gethostbyname();
	struct stat st = {0};

	pthread_mutex_unlock(&lock);
	pthread_mutex_unlock(&lockLog);

	/* Cria o folder onde os arquivos de log serão armazenados */
	strcpy(folder, "Server1Logs/");
	if (stat(folder, &st) == -1) {
    	mkdir(folder, 0700);
	}
	strcat(folder, "log.txt");

	loadSaveState(&raiz);

  	/* Cria o socket de comunicacao */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock<0) {
	/* houve erro na abertura do socket	*/
		perror("[Server1] Opening datagram socket");
		exit(1);
	}
	/* Associa porta */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(6969);
  
	if (bind(sock,(struct sockaddr *)&name, sizeof name ) < 0) {
		perror("[Server1] Binding datagram socket");
		exit(1);
	}
  
  	/* Imprime o numero da porta */
	length = sizeof(name);
	if (getsockname(sock,(struct sockaddr *)&name, &length) < 0) {
		perror("[Server1] Getting socket name");
		exit(1);
	}
	sprintf(buf, "[Server1] Socket port #%d\n", ntohs(name.sin_port)); addLog(buf);

	sprintf(buf, "[Server1 Receiver] Preparando para receber!\n"); addLog(buf);

	hp = gethostbyname("localhost"); //chama dns
	if (hp==0) {
		fprintf(stderr, "[Server1 Receiver] Localhost unknown host ");
      	exit(2);
  	}

	bcopy ((char *)hp->h_addr, (char *)&server2.sin_addr, hp->h_length);
	server2.sin_family = AF_INET;
	server2.sin_port = htons(8765);

	int cont=0;
  
  	dataShare.codigo = -1;
	shmem = create_shared_memory(sizeof(struct dataShare));
  	memcpy(shmem, &dataShare, sizeof(dataShare));

	/* Fork se for o filho vai pra ficar em loop infinito atualizando o pai */
	if(fork() == 0){
		recebeDados();
	}

	/* Caso exista backup, ele ja esta carregado em memoria agora sera compartilhada com os outros servers */
	compartilhaSave(raiz);

  	/* Le */
	sprintf(buf, "[Server1] Starting iteration\n"); addLog(buf);
  	while (1)
	{
		sprintf(buf, "[Server1] Estou pronto! Aguaradndo requests!\n");
		addLog(buf);

		/* Recebe o request */
		if (recvfrom(sock,(char *)&msgServer,sizeof(msgServer),0,(struct sockaddr *)&name, &length) < 0)
			perror("[Server1] Receiving datagram packet");

		sprintf(buf, "[Server1] Verificando se existe algum dado compartilhado\n"); addLog(buf);
		pthread_mutex_lock(&lock);
		d = shmem;
		if(d->codigo == 1){
			sprintf(buf, "[Server1] Existe, atualizando!\n"); addLog(buf);
			sincronizaRecebimento(&raiz, d->msg);
			d->codigo = -1;
			memcpy(shmem, &d, sizeof(d));
		}
		pthread_mutex_unlock(&lock);

		/* Exibe info de quem fez o request */
		//printf("[Server1] name.sin_family: %i\n", name.sin_family);
		//printf("[Server1] name.sin_addr: %s\n", inet_ntoa(name.sin_addr));
		//printf("[Server1] name.sin_port: %i\n", name.sin_port);
		//printf("[Server1] Codigo: %i\n", msg.codigo);
		msg = msgServer.msg;
		sprintf(buf, "[Server1] Codigo recebido: %i\n", msg.codigo); addLog(buf);
		if(msg.codigo == 9){
			sprintf(buf, "[Server1] Codigo para auto destruicao\n"); addLog(buf);
			break;
		}

		//ret = fork();
		//Se for processo pai volta e aguarda as proximas requisicoes
		//if(ret > 0)
		//continue;
		
		//printf("[Server1] Sou filho novo [%i]\n", getpid());

		//Se for = 0 significa que é filho portanto executa o pedido
		/* Associa para resposta */
		cli_send.sin_family =      msgServer.client.sin_family;          //name.sin_family;
		cli_send.sin_addr.s_addr = msgServer.client.sin_addr.s_addr;     //name.sin_addr.s_addr;
		cli_send.sin_port =        msgServer.client.sin_port;            //name.sin_port;

		switch (msg.codigo){
		/* LER */
		case 1:
			sprintf(buf, "[Server1] Ler arquivo\n"); addLog(buf);
			msg = encontrarArquivo(raiz, msg, READ_FILE);
			break;
		/* CRIAR */
		case 2:
			auxUsuario = msg;
			sprintf(buf, "[Server1] Criar arquivo (%s)\n", msg.subject); addLog(buf);
			msg.codigo = inserirNoFim(&raiz, msg);
			enviaAtualizacao(auxUsuario);
			saveState(raiz);
			break;
		/* EDITAR */
		case 3:
			sprintf(buf, "[Server1] Editar arquivo (%s)\n", msg.subject); addLog(buf);
			msg = encontrarArquivo(raiz, msg, EDIT_FILE);
			break;
		/* EXCLUIR */
		case 4:
			auxUsuario = msg;
			sprintf(buf, "[Server1] Excluir arquivo\n"); addLog(buf);
			deletaArquivo(&raiz, msg.subject);
			enviaAtualizacao(auxUsuario);
			saveState(raiz);
			break;
		/* VER CRIADOS */
		case 5:
			sprintf(buf, "[Server1] Ver arquivos\n"); addLog(buf);
			retornaCriados(&raiz);
			continue;
		case 6:
			auxUsuario = msg;
			sprintf(buf, "[Server1] Editar arquivo\n"); addLog(buf);
			atualizaArquivo(&raiz, msg);
			enviaAtualizacao(auxUsuario);
			saveState(raiz);
			continue;
		default:
			sprintf(buf, "[Server1] Opcao nao encontrada\n"); addLog(buf);
			break;
		}

		/* Associa para resposta */
		//cli_send.sin_family =      msgServer.client.sin_family;          //name.sin_family;
		//cli_send.sin_addr.s_addr = msgServer.client.sin_addr.s_addr;     //name.sin_addr.s_addr;
		//cli_send.sin_port =        msgServer.client.sin_port;            //name.sin_port;

		//printf("[Server1] name.sin_family: %i\n", cli_send.sin_family);
		//printf("[Server1] name.sin_addr: %s\n", inet_ntoa(cli_send.sin_addr));
		//printf("[Server1] name.sin_port: %i\n", cli_send.sin_port);
		//msg.porta = 67213/*name.sin_port*/;
		//strcpy(msg.ip, inet_ntoa(name.sin_addr));

		sprintf(buf, "[Server1] Devolvendo resultado\n"); addLog(buf);
		/* Envia o dado */
		if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
			perror("[Server1] Sending datagram message");
	}

	/*if(ret != 0){
		printf("[Server %i] Acabou, fiz tudo que eu podia =( \n", getpid());
		wait();
		printf("[Server1] Todos filhos mortos\n");
		close(sock);
		exit(0);	
	}else{*/
		sprintf(buf, "[Server1] Morri %i\n", getpid()); addLog(buf);
		exit(0);	
	//}
}

int inserirNoFim(struct data **raiz, struct mensagemUsuario msg){
    int ret;
	struct data *aux = *raiz;
	if(*raiz != NULL){
		sprintf(buf, "[Server1] File: %s Name: %s\n", aux->subject, msg.subject); addLog(buf);
		ret = strcmp(aux->subject, msg.subject);
		if(ret == 0)
			return ERROR_NAME_IN_USE;
		while(aux->prox != NULL){
			aux = aux->prox;
			sprintf(buf, "[Server1] File: %s Name: %s\n", aux->subject, msg.subject); addLog(buf);
			ret = strcmp(aux->subject, msg.subject);
			if(ret == 0)
				return ERROR_NAME_IN_USE;
		}
	}
	
	struct data *novo;
    novo = (struct data *) malloc(sizeof(struct data));
    if(novo == NULL) exit(0);
    strcpy(novo->subject, msg.subject);
	strcpy(novo->message, msg.message);
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

	sprintf(buf, "[Server1] Retorna Criados\n"); addLog(buf);

	if(*raiz == NULL){
		sprintf(buf, "[Server1] Raiz nula!\n"); addLog(buf);
		count = 0;
	}else{
		aux = *raiz;
		count = 1;
		while(aux->prox != NULL){
			sprintf(buf, "[Server1] Raiz não nula!\n"); addLog(buf);
			aux = aux->prox;
			count++;
		}
	}

	sprintf(buf, "[Server1] Existe %i arquivos criados\n", count); addLog(buf);

	msg.codigo = CREATED_FILES;
	if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
		perror("[Server1] Sending datagram message");	

	sprintf(buf, "[Server1] READ_FILES enviado, enviando quantia\n"); addLog(buf);
	msg.codigo = count;
	if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
		perror("[Server1] Sending datagram message");

	aux = *raiz;

	for(int i = 0; i < count; i++){
		msg.codigo = i;
		strcpy(msg.subject, aux->subject);
		if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
			perror("[Server1] Sending datagram message");
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
		sprintf(buf, "[Server1] Raiz nula, recriando!\n"); addLog(buf);
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
					sprintf(buf, "[Server1] Encontrado, atualizando! %s %s \n", aux->user, msg.user); 
					addLog(buf);
					return;
				}else{
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

		sprintf(buf, "[Server1] Nao existe mais, adicionado no fim!\n"); addLog(buf);
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
		perror("[Server1 Receiver] Opening datagram socket");
		exit(1);
	}
	/* Associa porta */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(9876);
  
	if (bind(sockReceiver,(struct sockaddr *)&name, sizeof name ) < 0) {
		perror("[Server1 Receiver] Binding datagram socket");
		exit(1);
	}
  
  	/* Imprime o numero da porta */
	socklen_t length = sizeof(name);
	if (getsockname(sockReceiver,(struct sockaddr *)&name, &length) < 0) {
		perror("[Server1 Receiver] Getting socket name");
		exit(1);
	}
	sprintf(buf, "[Server1 Receiver] Receiver port #%d\n", ntohs(name.sin_port)); addLog(buf);

	while(1){
		/* Recebe o request */
		if (recvfrom(sockReceiver,(char *)&dataShare,sizeof(dataShare),0,(struct sockaddr *)&name, &length) < 0)
			perror("[Server1 Receiver] Receiving datagram packet");

		sprintf(buf, "[Server1 Receiver] Novo arquivo recebido\n"); addLog(buf);

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
	
	if (sendto (sock,(char *)&data,sizeof data, 0, (struct sockaddr *)&server2, sizeof server2) < 0)
			perror("[Server1] Sending datagram message");
	sprintf(buf, "[Server2] Alteracao enviada!\n"); addLog(buf);
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
	saveState(*raiz);
}

void addLog(char*log){
	FILE *fp;
	char message[1000] = "\0";
	
	getTime(message);
	strcat(message, log);

	pthread_mutex_lock(&lockLog);
	fp = fopen(folder, "a+");
	fprintf(fp, "%s", message);
	fclose(fp);
	pthread_mutex_unlock(&lockLog);
}

void getTime(char* message){
	struct tm tm;
	char buffer[255];
	char f[255];

	sprintf(buffer,"%lu", time(NULL));
    
	memset(&tm, 0, sizeof(struct tm));
    strptime(buffer, "%s", &tm);
    strftime(f, sizeof(f), "%d-%b-%Y %H:%M ", &tm);
	strcpy(message, f);
}

void loadSaveState(struct data**raiz){
	FILE *fp;
	struct data *aux, *novo;
	struct mensagemUsuario input;
	fp = fopen("Server1Logs/data.dat", "r");
	if (fp == NULL) 
		return;

	while(fread(&input, sizeof(struct mensagemUsuario), 1, fp)){
		atualizaArquivo(raiz, input);
	} 

	sprintf(buf, "[Server1] Backup carregado com sucesso!\n");
	addLog(buf);

	fclose(fp);
}

void saveState(struct data *raiz){
	FILE *fp;
	struct mensagemUsuario aux;
	fp = fopen("Server1Logs/data.dat", "w");
	while(raiz != NULL){
		strcpy(aux.subject, raiz->subject);
		strcpy(aux.message, raiz->message);
		strcpy(aux.user, raiz->user);
		aux.time = raiz->time;
		aux.codigo = 6;

		fwrite(&aux, sizeof(struct mensagemUsuario), 1, fp);
		raiz = raiz->prox;
	}

	sprintf(buf, "[Server1] Save state completo!\n");
	addLog(buf);
	fclose(fp);
}

void compartilhaSave(struct data *raiz){
	struct mensagemUsuario msg;
	while(raiz != NULL){
		strcpy(msg.user, raiz->user);
		strcpy(msg.subject, raiz->subject);
		strcpy(msg.message, raiz->message);
		msg.time = raiz->time;
		msg.codigo = 6;
		
		enviaAtualizacao(msg);
		raiz = raiz->prox;
	}
}