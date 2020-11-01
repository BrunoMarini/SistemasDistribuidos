#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>


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

int ret;
char folder[MAX_DEFAULT_SIZE];

pthread_mutex_t lockServer; 
pthread_mutex_t lockLog;

void addLog(char*);

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
		//printf("[LoadBalancer] name.sin_family: %i\n", name.sin_family);
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
		addLog(buf);

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

		ret = fork();
		//Se for processo pai volta e aguarda as proximas requisicoes
		if(ret > 0)
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
		if (sendto (sock,(char *)&msgServer,sizeof msgServer, 0, (struct sockaddr *)&serverToSend, sizeof name) < 0)
			perror("[LoadBalancer] Sending datagram message");

		/* REMOVERRRRRR */
		exit(0);
	}

	if(ret != 0){
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
	pthread_mutex_lock(&lockLog);
	fp = fopen(folder, "a+");
	fprintf(fp, "%s", log);
	fclose(fp);
	pthread_mutex_unlock(&lockLog);
}