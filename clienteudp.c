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
#include <time.h>

#define MAX_MESSAGE_SIZE 500
#define MAX_DEFAULT_SIZE 50

#define READ_FILE 3
#define EDIT_FILE 2
#define CREATED_FILES 1
#define CREATE_SUCCESS 0
#define ERROR_NAME_IN_USE -1
#define FILE_NOT_FOUND -2
#define ERROR_EMPTY_DATABASE -3

struct conexao {
	int codigo;
};

struct mensagem {
	unsigned long time;
	int codigo;
	char user[MAX_DEFAULT_SIZE];
	char subject[MAX_DEFAULT_SIZE];
	char message[MAX_MESSAGE_SIZE];
};

//pthread_mutex_t lock; 
//pthread_mutex_lock(&lock); 
//pthread_mutex_unlock(&lock); 

//gcc -w clienteudp.c -o c && gcc -w loadBalancer.c -o l && gcc -w server.c -o s
// ./l &
// ./s &
// ./c &

//pkill s && pkill l && pkill c

void editFile(struct mensagem);
void printArquivos();
void parseResponse(struct mensagem);
void showFile(struct mensagem);
void printEpoch(char*);

struct sockaddr_in name, nameServer;
int sock;
char folder[MAX_DEFAULT_SIZE], nome[MAX_DEFAULT_SIZE];

int main()
{
	int op;
	char c;
	char pid[6], aux[MAX_DEFAULT_SIZE];
	struct hostent *hp, *gethostbyname();
  	struct mensagem msg;
	struct conexao con;
	struct stat st = {0};

	system("./s1 &");
	system("./s2 &");
	system("./l &");
	sleep(2);
	
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

	/* Envia codiog 10 pedido dos dados 
	con.codigo = 10;
	if (sendto (sock, (char *)&con, sizeof (struct conexao), 0, (struct sockaddr *) &nameServer, sizeof nameServer) < 0)
		perror("[Client] Sending datagram message");
	
	int tam;
	/* Primeiro recebe a quantidade de arquivos 
	if (recvfrom(sock,(char *)&msg,sizeof(struct mensagem),0,(struct sockaddr *)&name, &tam)<0)
		perror("[Client] 1 receiving datagram packet");
	
	if(msg.codigo == 0) {
		printf("[Client] Nenhum dados salvo ainda\n");
	} else {
		printf("[Client] Temso %i arquivos criados", msg.codigo);
		
		for(int i = 0; i < msg.codigo; i++){
			/* Primeiro recebe a quantidade de arquivos 
			if (recvfrom(sock,(char *)&msg,sizeof(struct mensagem),0,(struct sockaddr *)&name, &tam)<0)
				perror("[Client] 2 receiving datagram packet");
			
		}
	}*/

	printf("Por favor insira seu nome: ");
	scanf("%s", nome);

	/*if(fork() == 0){
		atualizaDados();
		loop = 0;
	} else {*/
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
			strcpy(msg.user, nome);

			switch (op) {
				case 1:
					printf("Insira o nome do arquivo que deseja ler: ");
					scanf("%s", msg.subject);
					break;
				case 2:
					printf("Insira o nome do arquivo que deseja criar: ");
					scanf("%s", msg.subject);
					strcpy(msg.message, "\0");
					break;
				case 3:
					printf("Insira o nome do arquivo que deseja editar: ");
					scanf("%s", msg.subject);
					printf("Preparando para pedir %s \n", msg.subject);
					break;
				case 4:
					printf("Insira o nome do arquivo que deseja excluir: ");
					scanf("%s", msg.subject);
					getchar();
					printf("Tem certeza que deseja excluir o arquivo %s [Y/n]?", msg.subject);
					scanf("%c", &c);
					if(c == 'n')
						continue;
					break;
				case 5:
					break;
				case 9: break;
				default:
					continue;
			}
			
			msg.time = (unsigned long)time(NULL);
			printf("[Client] Atribuindo tempo request! [%lu]\n", msg.time);

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

			parseResponse(msg);
		}
	//}
	strcpy(aux, "rm -rf ");
	strcat(aux, folder);
	printf("[Client] Apagando pasta e terminando! Adeus\n");
	system(aux);
	close(sock);
	exit(0);
}

void parseResponse(struct mensagem msg){
	printf("[Client] Codigo recebido: %i\n", msg.codigo);
	switch(msg.codigo){
		case ERROR_EMPTY_DATABASE:
			printf("\nERRO! Não existe arquivos cadastrados!\n");
			break;
		case CREATE_SUCCESS:
			printf("\nOperação realizada com sucesso!\n");
			break;
		case ERROR_NAME_IN_USE:
			printf("\nERRO! O nome já está em uso!\n");
			break;
		case CREATED_FILES:
			printArquivos();
			break;
		case FILE_NOT_FOUND:
			printf("\nERRO! O nome que nao foi possivel encontrar o arquivo!\n");
			break;
		case EDIT_FILE:
			editFile(msg);
			break;
		case READ_FILE:
			showFile(msg);
			break;
	}
}

void printArquivos(){
	int resposta, tam;
	struct mensagem msg;
	if (recvfrom(sock,(char *)&msg,sizeof(struct mensagem),0,(struct sockaddr *)&name, &tam)<0)
		perror("[Client] receiving datagram packet");
	resposta = msg.codigo;
	if(resposta == 0)
		printf("\nNenhum arquivo criado!\n");
	else
		printf("\nArquivos criado:\n");
	for(int i = 0; i < resposta; i++){
		if (recvfrom(sock,(char *)&msg,sizeof(struct mensagem),0,(struct sockaddr *)&name, &tam)<0)
			perror("[Client] receiving datagram packet");

		printf("%i. %s\n", i + 1, msg.subject);
	}
}

void editFile(struct mensagem msg){
	FILE *fp = NULL;
	char path[70];
	strcpy(path, folder);
	strcat(path, "/");
	strcat(path, msg.subject);
	strcat(path, ".txt");
	printf("[Client] File name: %s \n", path);
	fp = fopen(path, "w");
	fprintf(fp, "%s", msg.message);
	fclose(fp);
	
	while ( getchar() != '\n' );

	printf("\nArquivo %s encontrado!\n", msg.subject);
	printf("\nAgora so editar la e me avisar quando terminar =)\n");
	printf("Pressione qualquer tecla para continuar...");
	getchar();
	//FALTA ENVIAR O ATUALZIADO
	fp = fopen(path, "r");
	//fread(msg.message, 1, (long)ftell(fp), fp);
	char c = getc(fp);
	int i = 0;
   	while (c != EOF) {
		msg.message[i] = c;
		c = getc(fp);
		i++;
   	}

	msg.time = time(NULL);
	msg.codigo = 6;
	strcpy(msg.user, nome);

	if (sendto (sock, (char *)&msg, sizeof (struct mensagem), 0, (struct sockaddr *) &nameServer, sizeof nameServer) < 0)
				perror("[Client] Sending datagram message");
}

void showFile(struct mensagem msg){
	char buffer[30];
	sprintf(buffer,"%lu", msg.time);

	printf("\nExibindo arquivo: %s \n", msg.subject);
	if(strlen(msg.message) == 0)
		printf("\n** Mensagem vazia **\n");
	else
		printf("\n%s\n", msg.message);
	printf("\nCriado por: %s \n", msg.user);
	printEpoch(buffer);	
}

void printEpoch(char* time){
	struct tm tm;
    char buf[255];
	
    memset(&tm, 0, sizeof(struct tm));
    strptime(time, "%s", &tm);
    strftime(buf, sizeof(buf), "Em: %d %b %Y as %H:%M (GMT 00:00)", &tm);
    puts(buf);
    
	return;
}