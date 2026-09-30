#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "ws2_32.lib")

#define SERV_UDP_PORT 50000

#define BUFFER_SIZE 1024


int main(void)
{
    WSADATA wsa_data;
    SOCKET socket_servidor;
    struct sockaddr_in info_servidor;
    int res, bytes_recebidos;
    char buffer[BUFFER_SIZE];

    // nao e necessario em Berkeley (implementado em Linux/MacOS), mas e obrigatorio no Windows
    res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if(res != 0) {
        fprintf(stderr, "\n<SERVIDOR> Erro: Falha ao iniciar o winsock (%d)\n", res);
        exit(EXIT_FAILURE);
    }

    // cria socket IPv4 do tipo datagrama
    socket_servidor = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_servidor == INVALID_SOCKET) {
        fprintf(stderr, "\n<SERVIDOR> Erro: Falha ao iniciar o socket (%d)\n", WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // preenchimento da estrutura com os dados do servidor
    memset(&info_servidor, 0, sizeof(info_servidor));
	info_servidor.sin_family = AF_INET;                     // IPv4
	info_servidor.sin_addr.s_addr = htonl(INADDR_ANY);      // Aceita ligacoes de qualquer endereço IP
	info_servidor.sin_port = htons(SERV_UDP_PORT);          // Porto do servidor

    // vincula o socket ao porto
    res = bind(socket_servidor, (struct sockaddr*)&info_servidor, sizeof(info_servidor));
    if(res == SOCKET_ERROR) {
        fprintf(stderr, "\n<SERVIDOR> Erro: Falha na associacao (%d)\n", WSAGetLastError());
        closesocket(socket_servidor);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    printf("Servidor UDP a escuta no porto %d...\n", SERV_UDP_PORT);

    while (1) {
        // aguarda pela rececao
        bytes_recebidos = recvfrom(socket_servidor, buffer, BUFFER_SIZE - 1, 0, NULL, NULL);

        if (bytes_recebidos == SOCKET_ERROR || bytes_recebidos > BUFFER_SIZE) {
            fprintf(stderr, "\n<SERVIDOR> Erro: Falha na rececao (%d)\n", WSAGetLastError());
            closesocket(socket_servidor);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        buffer[bytes_recebidos] = '\0';     // garante terminacao da mensagem
        fprintf(stdout, "Mensagem recebida -> %s\n", buffer);
        fflush(stdout);
    }

    closesocket(socket_servidor);
    WSACleanup();
    return(EXIT_SUCCESS);
}