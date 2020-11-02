/*
 * Bruno Guilherme Spirlandeli Marini    - 17037607
 * Marcos Aurelio Tavares de Sousa Filho - 17042284 
 *
 * OPCIONAIS FUNCIONANDO: 
 * CONSISTÊNCIA, INTERFACE, BALANCEAMENTO DE CARGA, CHECKPOINT E LOG, 
 * DIRETÓRIO, FALHAS NO DIRETÓRIO, TOLERÂNCIA A FALHAS 
 *
 * DESCRICAO:
 * Atravez de um processo cliente (clienteudp.c) e possivel fazer requests 
 * para criar/editar/excluir/ler arquivos do servidor, o request será
 * realizado ao balanceador de carga (LoadBalancer.c) que por sua vez escolhera
 * um servidor para direcionar o pedido. Os servidores possuem mecanismos
 * internos para manter a consistencia dos dados entre eles, ou seja, um request.
 * feito para o servidor A será compartilhada com o servidor B de forma 
 * transparente para o usuario. Os servidores possuem mecanismos de backup, 
 * armazenando localmente os arquivos e podendo recarregalos caso aconteça
 * algum problema. O usuário por sua vez será informado quando nao for possivel
 * enviar/receber um dado. Para facilitar o debug logs sao gerados pelos 
 * servidores e salvos em um arquivo log.txt contendo a data e a informacao.
 *
 * DADOS:
 * Um arquivo contem as seguintes informacoes:
 * time: Aramazena a hora que o arquivo foi alterado pela ultima vez
 * user: Armazena o nome do ultimo usuario que alterou o arquivo (MAX: 50)
 * subject: Armazena o nome do arquivo (MAX 50)(Nao pode ser alterado)
 * message: Armazena a mensagem (MAX 500)
 *
 * Valor do Projeto: 11 pontos
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>

#define MAX_MESSAGE_SIZE 500
#define MAX_DEFAULT_SIZE 50

struct mensagemUsuario {
	unsigned long time;
	int codigo;
	char user[MAX_DEFAULT_SIZE];
	char fileName[MAX_DEFAULT_SIZE];
	char message[MAX_MESSAGE_SIZE];
};

struct mensagemServer {
	struct mensagemUsuario msg;
	struct sockaddr_in client;
};

int retFork;
char folder[MAX_DEFAULT_SIZE];
struct timeval tv;
ssize_t ret;

pthread_mutex_t lockServer; 
pthread_mutex_t lockLog;

void addLog(char*);
void getTime(char *);

int main()
{
	int sock, length, serverNum = 1;
	struct sockaddr_in name, cli_send;
	struct sockaddr_in server1, server2;
	struct sockaddr_in serverToSend;
	struct hostent *hp, *gethostbyname();
	char buf[1024], aux[MAX_MESSAGE_SIZE];
	struct mensagemUsuario msg;
	struct mensagemServer msgServer;
	struct stat st = {0};

	pthread_mutex_unlock(&lockServer);
	pthread_mutex_unlock(&lockLog);

	/* Cria o folder onde os arquivos de log serão armazenados */
	strcpy(folder, "LoadBalancerLogs/");
	if (stat(folder, &st) == -1) {
    	mkdir(folder, 0700);
	}
	strcat(folder, "log.txt");

	/* Cria o socket de comunicacao */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock<0) {
	/* houve erro na abertura do socket	*/
		perror("[LoadBalancer] Opening datagram socket");
		exit(1);
	}
	/* Associa porta */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(2020);

	/* Define timeout pra poder lidar com o caso do load balancer down */
	tv.tv_sec = 0;
	tv.tv_usec = 200000;
	
    if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) < 0)
        perror("setsockopt failed\n");

	if (bind(sock,(struct sockaddr *)&name, sizeof name ) < 0) {
		perror("[LoadBalancer] Binding datagram socket");
		exit(1);
	}

	/* Imprime o numero da porta */
	length = sizeof(name);
	if (getsockname(sock,(struct sockaddr *)&name, &length) < 0) {
		perror("[LoadBalancer] Getting socket name");
		exit(1);
	}

	sprintf(buf, "[LoadBalancer] Socket port #%d\n", ntohs(name.sin_port));
	//printf("%s", buf);
	addLog(buf);

	/* Associa o servidor (porta pre definida) */
	hp = gethostbyname("localhost"); //chama dns
	if (hp==0) {
		fprintf(stderr, "[Client] Localhost unknown host ");
      	exit(2);
  	}
  	
	/* Já pre define o servidor de acesso */
  	bcopy ((char *)hp->h_addr, (char *)&server1.sin_addr, hp->h_length);
	server1.sin_family = AF_INET;
	server1.sin_port = htons(6969);

	bcopy ((char *)hp->h_addr, (char *)&server2.sin_addr, hp->h_length);
	server2.sin_family = AF_INET;
	server2.sin_port = htons(6548);

	int cont=0;
  
  	/* Le */
	sprintf(buf, "[LoadBalancer] Starting iteration\n");
	//printf("%s", buf);
	addLog(buf);

  	while (1)
	{
		/* Recebe o request */
		if (recvfrom(sock,(char *)&msg,sizeof(msg),0,(struct sockaddr *)&name, &length) < 0)
			perror("[LoadBalancer] Receiving datagram packet");

		pthread_mutex_lock(&lockServer);
		
		sprintf(buf, "[LoadBalancer] Enviando para o server: %i\n", serverNum);
		//printf("%s", buf);
		addLog(buf);

		if(serverNum == 1){
			serverToSend = server1;
			serverNum = 2;
		}else if(serverNum == 2){
			serverToSend = server2;
			serverNum = 1;
		}
		pthread_mutex_unlock(&lockServer);

		/* Exibe info de quem fez o request */
		/*//printf("[LoadBalancer] name.sin_family: %i\n", name.sin_family);
		sprintf(buf, "[LoadBalancer] name.sin_family: %i\n", name.sin_family);
		
		//printf("[LoadBalancer] name.sin_addr: %s\n", inet_ntoa(name.sin_addr));
		sprintf(aux, "[LoadBalancer] name.sin_family: %i\n", name.sin_family);
		strcat(buf, aux);

		//printf("[LoadBalancer] name.sin_port: %i\n", name.sin_port);
		sprintf(aux, "[LoadBalancer] name.sin_family: %i\n", name.sin_family);
		strcat(buf, aux);

		//printf("[LoadBalancer] Codigo: %i\n", msg.codigo);
		sprintf(aux, "[LoadBalancer] name.sin_family: %i\n", name.sin_family);
		strcat(buf, aux);
		addLog(buf);*/

		/* Mandar codigo para matar o server tambem */
		if(msg.codigo == 9){
			//printf("[LoadBalancer] Codigo para auto destruicao\n");
			sprintf(buf, "[LoadBalancer] Codigo para auto destruicao\n");
			addLog(buf);

			msgServer.msg = msg;
			if (sendto (sock,(char *)&msgServer,sizeof msgServer, 0, (struct sockaddr *)&server1, sizeof name) < 0)
				perror("[LoadBalancer] Sending datagram message");
			if (sendto (sock,(char *)&msgServer,sizeof msgServer, 0, (struct sockaddr *)&server2, sizeof name) < 0)
				perror("[LoadBalancer] Sending datagram message");
			break;
		}

		retFork = fork();
		//Se for processo pai volta e aguarda as proximas requisicoes
		if(retFork > 0)
			continue;
		
		//printf("[LoadBalancer] Sou filho novo [%i]\n", getpid());
		sprintf(buf, "[LoadBalancer] Sou filho novo [%i]\n", getpid());
		addLog(buf);

		/* Associa para resposta */
		cli_send.sin_family = name.sin_family;
		cli_send.sin_addr.s_addr = name.sin_addr.s_addr;
		cli_send.sin_port = name.sin_port;

		//msg.codigo = cont;
		//msg.porta = 67213/*name.sin_port*/;
		//strcpy(msg.ip, inet_ntoa(name.sin_addr));
		
		msgServer.msg = msg;
		msgServer.client = cli_send;
		
		//printf("[LoadBalancer] Encaminhando request para servidor\n");
		sprintf(buf, "[LoadBalancer] Encaminhando request para servidor\n");
		addLog(buf);

		/* Envia o dado para o server */
		ret = sendto (sock,(char *)&msgServer,sizeof msgServer, 0, (struct sockaddr *)&serverToSend, sizeof name);
		if (ret == -1){
			sprintf(buf, "[LoadBalancer] Erro, Servidor [%i] inativo!\n", serverNum);
			addLog(buf);

			if(serverNum == 1)
				system("./s1 &");
			else
				system("./s2 &");
			//perror("[LoadBalancer] Sending datagram message");
		}
		/* REMOVERRRRRR */
		exit(0);
	}

	if(retFork != 0){
		//printf("[Server %i] Acabou, fiz tudo que eu podia =( \n", getpid());
		sprintf(buf, "[Server %i] Acabou, fiz tudo que eu podia =( \n", getpid());
		addLog(buf);

		wait();
		
		//printf("[LoadBalancer] Todos filhos mortos\n");
		sprintf(buf, "[LoadBalancer] Todos filhos mortos\n");
		addLog(buf);

		close(sock);
		exit(0);	
	}else{
		//printf("[LoadBalancer] Morri %i\n", getpid());
		sprintf(buf, "[LoadBalancer] Morri %i\n", getpid());
		addLog(buf);

		exit(0);	
	}
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