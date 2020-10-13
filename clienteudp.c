#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DATA "Esta eh a mensagem que quero enviar"

struct conexao {
	int codigo;
};

struct mensagem {
	int codigo;
	char user[50];
	char fileName[50];
	char message[500];
};

//pthread_mutex_t lock; 
//pthread_mutex_lock(&lock); 
//pthread_mutex_unlock(&lock); 

//gcc -w clienteudp.c -o c && gcc -w loadBalancer.c -o l && gcc -w server.c -o s
// ./l &
// ./s &
// ./c &

//pkill s && pkill l && pkill c

void atualizaDados();

int main()
{
	int sock, op;
	char nome[50], folder[50], pid[6];
	struct sockaddr_in nameServer, name;
	struct hostent *hp, *gethostbyname();
  	struct mensagem msg;
	struct conexao con;
	struct stat st = {0};

  	/* Cria o socket de comunicacao */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock<0) {
	/* houve erro na abertura do socket*/
		perror("[Client] Opening datagram socket");
		exit(1);
	}
	/* Associa */
	hp = gethostbyname("localhost"); //chama dns
	if (hp==0) {
		fprintf(stderr, "[Client] Localhost unknown host ");
      	exit(2);
  	}
  	
	/* Já pre define o servidor de acesso */
  	bcopy ((char *)hp->h_addr, (char *)&nameServer.sin_addr, hp->h_length);
	nameServer.sin_family = AF_INET;
	nameServer.sin_port = htons(2020);

	/* Cria o folder onde os arquivos serão armazenados */
	strcpy(folder, "user");
	sprintf(pid, "%d", getpid());
	strcat(folder, pid);
	if (stat(folder, &st) == -1) {
    	mkdir(folder, 0700);
	}

	int loop = 1;

	/* 
	 * Processo pai ira cuidar da interacao com o usuario enquanto
     * o processo filho cuidara da atualizacao dos dados para manter
	 * a consistencia com o servidor.
     *
	 * Antes de criar o filho trava o mutex para que o usuario só possa 
	 * interagir após os dados serem atualziados com o servidor 
	 * (primeira iteracao)
	 */

	/* Envia codiog 10 pedido dos dados */
	con.codigo = 10;
	if (sendto (sock, (char *)&con, sizeof (struct conexao), 0, (struct sockaddr *) &nameServer, sizeof nameServer) < 0)
		perror("[Client] Sending datagram message");
	
	int tam;
	/* Primeiro recebe a quantidade de arquivos */
	if (recvfrom(sock,(char *)&msg,sizeof(struct mensagem),0,(struct sockaddr *)&name, &tam)<0)
		perror("[Client] 1 receiving datagram packet");
	
	if(msg.codigo == 0) {
		printf("[Client] Nenhum dados salvo ainda\n");
	} else {
		printf("[Client] Temso %i arquivos criados", msg.codigo);
		
		for(int i = 0; i < msg.codigo; i++){
			/* Primeiro recebe a quantidade de arquivos */
			if (recvfrom(sock,(char *)&msg,sizeof(struct mensagem),0,(struct sockaddr *)&name, &tam)<0)
				perror("[Client] 2 receiving datagram packet");
			
		}
	}

	printf("Por favor insira seu nome: ");
	scanf("%s", nome);

	if(fork() == 0){
		atualizaDados();
		loop = 0;
	} else {
		while(loop)
		{   
			/* Pergunta pro usuario a opcao desejada */
			printf("\nDigite a opcao desejada:\n");
			printf("1.Ler; \n");
			printf("2.Criar;\n");
			printf("3.Editar;\n");
			printf("4.Excluir;\n");
			printf("5.Arquivos Criados;\n");
			printf("0.Sair.\n\n");
			printf("Opcao: ");

			scanf("%i", &op);
			getchar();

			if(op == 0)
				break;

			msg.codigo = op;
			strcpy(msg.user, &name);

			switch (op) {
				case 1:
					break;
				case 2:
					printf("Insira o nome do arquivo que deseja criar: ");
					scanf("%s", msg.fileName);
					break;
				case 3:
					break;
				case 4:
					break;
				case 5:
					break;
				default:
					continue;
			}

			/* Envia */
			if (sendto (sock, (char *)&msg, sizeof (struct mensagem), 0, (struct sockaddr *) &nameServer, sizeof nameServer) < 0)
				perror("[Client] Sending datagram message");

			if(msg.codigo == 9){
				printf("[Client] Codigo para auto destruicao\n");
				break;
			}

			/* Recebe a resposta */
			int resposta, tam;
			if (recvfrom(sock,(char *)&msg,sizeof(struct mensagem),0,(struct sockaddr *)&name, &tam)<0)
				perror("[Client] receiving datagram packet");
		}
	}

  close(sock);
  exit(0);
}

void atualizaDados(){
	while(1){

	}
}