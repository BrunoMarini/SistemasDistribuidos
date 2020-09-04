#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>

#define DATA "Esta eh a mensagem que quero enviar"

struct mensagem {
  int codigo;
  char ip[50];
  int porta;
};

//gcc clienteudp.c -o c && gcc serverudp.c -o s && ./s &
main()
{
	int sock;
	struct sockaddr_in name;
	struct hostent *hp, *gethostbyname();
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
        hp = gethostbyname("localhost"); //chama dns
        if (hp==0) {
            fprintf(stderr, "localhost unknown host ");
            exit(2);
        }
        bcopy ((char *)hp->h_addr, (char *)&name.sin_addr, hp->h_length);
	name.sin_family = AF_INET;
	name.sin_port = htons(2020);

  msg.codigo=1;
  
	/* Envia */
	if (sendto (sock,(char *)&msg,sizeof (struct mensagem), 0, (struct sockaddr *)&name,
                    sizeof name)<0)
                perror("sending datagram message");

int resposta, tam;
if (recvfrom(sock,(char *)&msg,sizeof(struct mensagem),0,(struct sockaddr *)&name, &tam)<0)
                perror("[Client] receiving datagram packet");

printf("[Client] Porta recebida:  %i\n", msg.porta);
printf("[Client] IP recebido :    %s\n", msg.ip);
printf("[Client] Codigo recebido: %i\n", msg.codigo);
        close(sock);
        exit(0);
}

