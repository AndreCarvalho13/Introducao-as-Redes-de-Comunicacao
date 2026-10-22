#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "ws2_32.lib")

// #define SERV_HOST_ADDR "127.0.0.1" coloquei em comentario para que seja preciso passar o IP do servidor por linha de comando ex5
//#define SERV_UDP_PORT 50000 passar a porta do servidor por linha de comando ex5
#define BUFFER_SIZE 1024 // tamanho maximo da mensagem a enviar ex2


int main(int argc, char* argv[]) {
	WSADATA wsa_data;
	SOCKET socket_cliente;
	struct sockaddr_in info_servidor;
	int res;
	int bytes_recebidos; // variavel para guardar o numero de bytes recebidos ex2
	char buffer[BUFFER_SIZE];
	struct sockaddr_in info_local; // Estrutura para guardar os dados do cliente ex3
	int tamanho_info_local;   // Variavel para guardar o tamanho da estrutura sockaddr_in do cliente ex3
	struct sockaddr_in info_origem; // Estrutura para guardar os dados da origem ex6
	int tamanho_info_origem;   // Variavel para guardar o tamanho da estrutura sockaddr_in da origem ex6

	// Variaveis para guardar o IP, porta e mensagem a enviar para o servidor ex5
	char* mensagem = NULL;
	char* ip_servidor = NULL;
	int porta_servidor = 0;

	/*
	// verifica se foi passada a mensagem a enviar
	if (argc != 7) {
		fprintf(stdout, "\nSintaxe:\nexecutavel <IP do servidor> <porta do servidor> <frase a enviar>\n\n");
		exit(EXIT_FAILURE);
	}
	*/
	// guarda os argumentos passados por linha de comando ex5
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i],"-msg") == 0 && i + 1 < argc)
		{
			mensagem = argv[i + 1];
		}
		else if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc)
		{
			ip_servidor = argv[i + 1];
		}
		else if (strcmp(argv[i], "-port") == 0 && i + 1 < argc)
		{
			porta_servidor = atoi(argv[i + 1]);
		}
		
	}

	if (mensagem == NULL || ip_servidor == NULL || porta_servidor == 0) {

		fprintf(stderr,
			"Sintaxe:\n"
			"ex01-cliente.exe -msg \"mensagem\" -ip <IP> -port <porto>\n"
		);

		exit(EXIT_FAILURE);
	}

	// nao e necessario em Berkeley (implementado em Linux/MacOS), mas e obrigatorio no Windows
	res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (res != 0) {
		fprintf(stderr, "\n<CLIENTE> Erro: Falha ao iniciar o winsock (%d)\n", res);
		exit(EXIT_FAILURE);
	}

	// cria socket IPv4 do tipo datagrama
	socket_cliente = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_cliente == INVALID_SOCKET) {
		fprintf(stderr, "\n<CLIENTE> Erro: Falha ao iniciar o socket (%d)\n", WSAGetLastError());
		closesocket(socket_cliente);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	// preenchimento da estrutura com os dados do servidor
	memset(&info_servidor, 0, sizeof(info_servidor));	// preeenche toda a estrutura com 0
	info_servidor.sin_family = AF_INET;					// IPv4
	info_servidor.sin_port = htons(porta_servidor);		// Porto do servidor
	// info_servidor.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);	// IP do servidor
	// considerado nao seguro: nao verifica se o IP esta correto
	// usar antes:
	res = inet_pton(AF_INET, ip_servidor, &info_servidor.sin_addr);
	if (res != 1) {
		fprintf(stderr, "\n<\n<CLIENTE> Erro: Endereco IP invalido (%d)\n", WSAGetLastError());
		closesocket(socket_cliente);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	// envia a mensagem passada por linha de comando para o servidor
	res = sendto(socket_cliente, mensagem, (int)strlen(mensagem), 0, (struct sockaddr*)&info_servidor, sizeof(info_servidor));
	if(res == SOCKET_ERROR) {
		fprintf(stderr, "\n<CLIENTE> Erro: Falha na transmissao (%d)\n", WSAGetLastError());
		closesocket(socket_cliente);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "Mensagem enviada para %s:%d -> %s\n", ip_servidor, porta_servidor, mensagem);

	// preenchimento da estrutura com os dados do cliente ex3
	tamanho_info_local = sizeof(info_local);

	res = getsockname(socket_cliente, (struct sockaddr*)&info_local, &tamanho_info_local);

	if (res == SOCKET_ERROR) {
		fprintf(stderr, "\n<CLIENTE> Erro: Falha ao obter o endereco local (%d)\n", WSAGetLastError());
		closesocket(socket_cliente);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "Endereco local do cliente ->%d\n", ntohs(info_local.sin_port));

	// aguarda pela rececao da resposta do servidor ex2
	bytes_recebidos = recvfrom(socket_cliente, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&info_origem, &tamanho_info_origem);
	if (bytes_recebidos == SOCKET_ERROR) {
		fprintf(stderr, "\n<CLIENTE> Erro: Falha na rececao (%d)\n", WSAGetLastError());
		closesocket(socket_cliente);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	if (info_origem.sin_addr.s_addr != info_servidor.sin_addr.s_addr ||
		info_origem.sin_port != info_servidor.sin_port)
	{
		fprintf(stderr, "Erro: resposta recebida de uma origem diferente do servidor.\n");

		closesocket(socket_cliente);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	buffer[bytes_recebidos] = '\0';	// garante terminacao da mensagem ex2
	
	fprintf(stdout, "Mensagem recebida do servidor -> %s\n", buffer); // resposta do servidor ex2

	closesocket(socket_cliente);
	WSACleanup();
	return(EXIT_SUCCESS);
}