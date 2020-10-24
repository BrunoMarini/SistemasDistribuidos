#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define MAX_MESSAGE_SIZE 500
#define MAX_DEFAULT_SIZE 50

struct mensagemUsuario {
	int time;
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

int main()
{
	int sock, length;
	struct sockaddr_in name, cli_send;
	struct sockaddr_in server1, server2;
	struct hostent *hp, *gethostbyname();
	char buf[1024];
	struct mensagemUsuario msg;
	struct mensagemServer msgServer;

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
	printf("[LoadBalancer] Socket port #%d\n", ntohs(name.sin_port));

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

	int cont=0;
  
  	/* Le */
	printf("[LoadBalancer %i] Starting iteration\n", getpid());
  	while (1)
	{
		/* Recebe o request */
		if (recvfrom(sock,(char *)&msg,sizeof(msg),0,(struct sockaddr *)&name, &length) < 0)
			perror("[LoadBalancer] Receiving datagram packet");

		/* Exibe info de quem fez o request */
		printf("[LoadBalancer] name.sin_family: %i\n", name.sin_family);
		printf("[LoadBalancer] name.sin_addr: %s\n", inet_ntoa(name.sin_addr));
		printf("[LoadBalancer] name.sin_port: %i\n", name.sin_port);
		printf("[LoadBalancer] Codigo: %i\n", msg.codigo);

		/* Mandar codigo para matar o server tambem */
		if(msg.codigo == 9){
			printf("[LoadBalancer] Codigo para auto destruicao\n");
			msgServer.msg = msg;
			if (sendto (sock,(char *)&msgServer,sizeof msgServer, 0, (struct sockaddr *)&server1, sizeof name) < 0)
				perror("[LoadBalancer] Sending datagram message");
			break;
		}

		ret = fork();
		//Se for processo pai volta e aguarda as proximas requisicoes
		if(ret > 0)
			continue;
		
		printf("[LoadBalancer] Sou filho novo [%i]\n", getpid());

		/* Associa para resposta */
		cli_send.sin_family = name.sin_family;
		cli_send.sin_addr.s_addr = name.sin_addr.s_addr;
		cli_send.sin_port = name.sin_port;

		//msg.codigo = cont;
		//msg.porta = 67213/*name.sin_port*/;
		//strcpy(msg.ip, inet_ntoa(name.sin_addr));
		
		msgServer.msg = msg;
		msgServer.client = cli_send;
		
		printf("[LoadBalancer] Encaminhando request para servidor\n");
		/* Envia o dado para o server */
		if (sendto (sock,(char *)&msgServer,sizeof msgServer, 0, (struct sockaddr *)&server1, sizeof name) < 0)
			perror("[LoadBalancer] Sending datagram message");

		/* REMOVERRRRRR */
		exit(0);
	}

	if(ret != 0){
		printf("[Server %i] Acabou, fiz tudo que eu podia =( \n", getpid());
		wait();
		printf("[LoadBalancer] Todos filhos mortos\n");
		close(sock);
		exit(0);	
	}else{
		printf("[LoadBalancer] Morri %i\n", getpid());
		exit(0);	
	}
}