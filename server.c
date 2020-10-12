#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

struct mensagem {
  int codigo;
  char message[500];
  char ip[50]; //receber o ip do jeito pra ja atribuir
  int porta;   
};

struct mensagemServer {
	int codigo;
	struct sockaddr_in client;
};

int ret;
int paiPid;

int main()
{
	int sock, length;
	struct sockaddr_in name, cli_send;
	char buf[1024];
  	struct mensagem msg;
	struct mensagemServer msgServer;

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

		if(msgServer.codigo == 9){
		printf("[Server] Codigo para auto destruicao\n");
		break;
		}

		ret = fork();
		//Se for processo pai volta e aguarda as proximas requisicoes
		if(ret > 0)
		continue;
		
		printf("[Server] Sou filho novo [%i]\n", getpid());

		//Se for = 0 significa que é filho portanto executa o pedido
		switch (msgServer.codigo){
		/* LER */
		case 1:
			printf("[Server] Ler arquivo\n");
			break;
		/* CRIAR */
		case 2:
			printf("[Server] Criar arquivo\n");
			break;
		/* EDITAR */
		case 3:
			printf("[Server] Editar arquivo\n");
			break;
		/* EXCLUIR */
		case 4:
			printf("[Server] Excluir arquivo\n");
			break;
		/* VER CRIADOS */
		case 5:
			printf("[Server] Ver arquivos\n");
			break;
		/* Atualizar Client */
		case 10:
			printf("[Server] Atualizar Client\n");
			break;
		default:
			printf("[Server] Opcao nao encontrada\n");
			break;
		}

		/* Associa para resposta */
		cli_send.sin_family =      msgServer.client.sin_family;          //name.sin_family;
		cli_send.sin_addr.s_addr = msgServer.client.sin_addr.s_addr;     //name.sin_addr.s_addr;
		cli_send.sin_port =        msgServer.client.sin_port;            //name.sin_port;

		printf("[Server] name.sin_family: %i\n", cli_send.sin_family);
		printf("[Server] name.sin_addr: %s\n", inet_ntoa(cli_send.sin_addr));
		printf("[Server] name.sin_port: %i\n", cli_send.sin_port);

		msg.codigo = 0;
		//msg.porta = 67213/*name.sin_port*/;
		//strcpy(msg.ip, inet_ntoa(name.sin_addr));

		printf("[Server] Devolvendo resultado\n");
		/* Envia o dado */
		if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof cli_send) < 0)
			perror("[Server] Sending datagram message");

		/* Break temporario para para o processo filho */
		exit(0);
	}

	if(ret != 0){
		printf("[Server %i] Acabou, fiz tudo que eu podia =( \n", getpid());
		wait();
		printf("[Server] Todos filhos mortos\n");
		close(sock);
		exit(0);	
	}else{
		printf("[Server] Morri %i\n", getpid());
		exit(0);	
	}
}