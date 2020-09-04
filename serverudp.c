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
  char ip[50]; //receber o ip do jeito pra ja atribuir
  int porta;   
};

main()
{
	int sock, length;
	struct sockaddr_in name, cli_send;
	char buf[1024];
  struct mensagem msg;

        /* Cria o socket de comunicacao */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock<0) {
	/*
	/- houve erro na abertura do socket
	*/
		perror("opening datagram socket");
		exit(1);
	}
	/* Associa */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(2020);
  
	if (bind(sock,(struct sockaddr *)&name, sizeof name ) < 0) {
		perror("binding datagram socket");
		exit(1);
	}
    /* Imprime o numero da porta */
	length = sizeof(name);
	if (getsockname(sock,(struct sockaddr *)&name, &length) < 0) {
		perror("getting socket name");
		exit(1);
	}
	printf("[Server] Socket port #%d\n", ntohs(name.sin_port));

	int cont=0;
	/* Le */
	while (1)
	{
		if (recvfrom(sock,(char *)&msg,sizeof(msg),0,(struct sockaddr *)&name, &length)<0)
                perror("receiving datagram packet");

    printf("[Server] name.sin_family: %i\n", name.sin_family);
    printf("[Server] name.sin_addr: %s\n", inet_ntoa(name.sin_addr));
    printf("[Server] name.sin_port: %i\n", name.sin_port);
    //printf("[Server] Message:  %s\n", buf);

		/* Associa */
		cli_send.sin_family = name.sin_family;
		cli_send.sin_addr.s_addr = name.sin_addr.s_addr;
		cli_send.sin_port = name.sin_port;

    msg.codigo = cont;
    msg.porta = 67213/*name.sin_port*/;
    strcpy(msg.ip, inet_ntoa(name.sin_addr));

    printf("[Server] Sending: %i\n", cont);
		if (sendto (sock,(char *)&msg,sizeof msg, 0, (struct sockaddr *)&cli_send, sizeof name)<0)
                perror("sending datagram message");

    if(cont > 10)
      break;

    cont++;
	}

  printf("[Server] Acabou, fiz tudo que eu podia =( \n");

	close(sock);
  exit(0);	
}
