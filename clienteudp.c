#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>

#define DATA "Esta eh a mensagem que quero enviar"

struct mensagem {
  int codigo;
  char message[500];
  char ip[50];
  int porta;
};

//gcc -w clienteudp.c -o c && gcc -w serverudp.c -o s && ./s &
//pkill s

int main()
{
	int sock, op;
	struct sockaddr_in nameServer, name;
	struct hostent *hp, *gethostbyname();
  struct mensagem msg;

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

  while(1){
   
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

    printf("[Client] Vou enviar em\n");
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

    printf("[Client] Porta recebida:  %i\n", ntohs(msg.porta));
    printf("[Client] IP recebido :    %s\n", msg.ip);
    printf("[Client] Codigo recebido: %i\n", msg.codigo);
  }
  
  close(sock);
  exit(0);

}

