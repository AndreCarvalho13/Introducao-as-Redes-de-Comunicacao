#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
	WSADATA wsa_data;
	SOCKET socket_cliente;
	struct sockaddr_in info_servidor;
	int res, bytes_recebidos, bytes_enviados;
	char buffer[BUFFER_SIZE];

	// verifica se foi passada a mensagem a enviar
	if (argc != 4) {
		fprintf(stdout, "\nSintaxe:\nexecutavel <ip> <porto> <frase a enviar>\n\n");
		exit(EXIT_FAILURE);
	}

	// nao e necessario em Berkeley (implementado em Linux/MacOS), mas e obrigatorio no Windows
	res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (res != 0) {
		fprintf(stderr, "\n<CLIENTE> Erro: Falha ao iniciar o winsock (%d)\n", res);
		exit(EXIT_FAILURE);
	}

	// cria socket IPv4 do tipo stream
	socket_cliente = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_cliente == INVALID_SOCKET) {
		fprintf(stderr, "\n<CLIENTE> Erro: Falha ao iniciar o socket (%d)\n", WSAGetLastError());
		closesocket(socket_cliente);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	// preenchimento da estrutura com os dados do servidor
	memset(&info_servidor, 0, sizeof(info_servidor));	// preeenche toda a estrutura com 0
	info_servidor.sin_family = AF_INET;					// IPv4
	info_servidor.sin_port = htons(atoi(argv[2]));		// Porto do servidor
	res = inet_pton(AF_INET, argv[1], &info_servidor.sin_addr);	// IP do servidor
	if (res != 1) {
		fprintf(stderr, "\n<\n<CLIENTE> Erro: Endereco IP invalido (%d)\n", WSAGetLastError());
		closesocket(socket_cliente);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	// estabelecer a ligação com o servidor
	res = connect(socket_cliente, (struct sockaddr *) &info_servidor, sizeof(info_servidor));
	if (res == SOCKET_ERROR) {
		fprintf(stderr, "\n<CLIENTE> Erro: Falha na ligacao (%d)\n", WSAGetLastError());
		closesocket(socket_cliente);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	// envia a mensagem passada por linha de comando para o servidor
	bytes_enviados = send(socket_cliente, argv[3], (int)strlen(argv[3]), 0);
	if(bytes_enviados == SOCKET_ERROR) {
		fprintf(stderr, "\n<CLIENTE> Erro: Falha na transmissao (%d)\n", WSAGetLastError());
		closesocket(socket_cliente);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "Mensagem enviada -> %s\n", argv[3]);

	// aguarda pela resposta
	bytes_recebidos = recv(socket_cliente, buffer, sizeof(buffer), 0);
	if (bytes_recebidos == SOCKET_ERROR) {
		fprintf(stderr, "\n<CLIENTE> Erro: Falha na recepcao (%d)\n", WSAGetLastError());
		closesocket(socket_cliente);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	buffer[bytes_recebidos] = '\0';	// adiciona o terminador de string

	fprintf(stdout, "Resposta do servidor -> %s\n", buffer);

	closesocket(socket_cliente);
	WSACleanup();
	return(EXIT_SUCCESS);
}