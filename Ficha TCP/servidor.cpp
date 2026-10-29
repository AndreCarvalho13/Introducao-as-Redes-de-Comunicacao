#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define SERV_TCP_PORT 50000
#define BUFFER_SIZE 1024

int writeN(SOCKET sock, char* buffer, int nbytes);
int readLine(SOCKET sock, char* buffer, int tamMax);

int main(void)
{
    WSADATA wsa_data;
    SOCKET socket_servidor, socket_ligacao;
    struct sockaddr_in info_servidor;
    int res, bytes_recebidos, bytes_enviados;
    char buffer[BUFFER_SIZE];

    // Obrigatorio no Windows
    res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (res != 0) {
        fprintf(stderr, "\n<SERVIDOR> Erro: Falha ao iniciar o winsock (%d)\n", res);
        exit(EXIT_FAILURE);
    }

    // Cria socket IPv4 do tipo stream
    socket_servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_servidor == INVALID_SOCKET) {
        fprintf(stderr, "\n<SERVIDOR> Erro: Falha ao iniciar o socket (%d)\n",
            WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Preenchimento da estrutura com os dados do servidor
    memset(&info_servidor, 0, sizeof(info_servidor));
    info_servidor.sin_family = AF_INET;
    info_servidor.sin_addr.s_addr = htonl(INADDR_ANY);
    info_servidor.sin_port = htons(SERV_TCP_PORT);

    // Vincula o socket ao porto
    res = bind(socket_servidor, (struct sockaddr*)&info_servidor,
        sizeof(info_servidor));
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "\n<SERVIDOR> Erro: Falha na associacao (%d)\n",
            WSAGetLastError());
        closesocket(socket_servidor);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Coloca o socket em modo de escuta
    res = listen(socket_servidor, SOMAXCONN);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "\n<SERVIDOR> Erro: Falha na escuta (%d)\n",
            WSAGetLastError());
        closesocket(socket_servidor);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    printf("Servidor TCP a escuta no porto %d...\n", SERV_TCP_PORT);

    while (1) {
        // Aceita a ligacao de um cliente
        socket_ligacao = accept(socket_servidor, NULL, NULL);
        if (socket_ligacao == INVALID_SOCKET) {
            fprintf(stderr, "\n<SERVIDOR> Erro: Falha na ligacao (%d)\n",
                WSAGetLastError());
            closesocket(socket_servidor);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        printf("Cliente ligado.\n");

        // Recebe varias mensagens do mesmo cliente
        while (1) {
            bytes_recebidos = readLine(socket_ligacao, buffer, BUFFER_SIZE);

            if (bytes_recebidos == SOCKET_ERROR) {
                fprintf(stderr, "\n<SERVIDOR> Erro: Falha na rececao (%d)\n",
                    WSAGetLastError());
                break;
            }

            if (bytes_recebidos == 0) {
                printf("O cliente encerrou a ligacao.\n");
                break;
            }

            // Confirma que recebeu uma linha completa
            if (buffer[bytes_recebidos - 1] != '\n') {
                fprintf(stderr, "\n<SERVIDOR> Mensagem demasiado longa.\n");
                break;
            }

            // Termina a ligacao deste cliente se receber SAIR
            if (strcmp(buffer, "SAIR\n") == 0) {
                printf("O cliente pediu para sair.\n");
                break;
            }

            // Remove o '\n' e desconta-o do tamanho
            bytes_recebidos--;
            buffer[bytes_recebidos] = '\0';

            fprintf(stdout, "Mensagem recebida -> %s\n", buffer);
            fflush(stdout);

            // Envia o tamanho da mensagem em ASCII
            sprintf_s(buffer, sizeof(buffer), "%d\n", bytes_recebidos);

            bytes_enviados = writeN(socket_ligacao, buffer,
                (int)strlen(buffer));

            if (bytes_enviados == SOCKET_ERROR) {
                fprintf(stderr, "\n<SERVIDOR> Erro: Falha na transmissao (%d)\n",
                    WSAGetLastError());
                break;
            }

            if (bytes_enviados == 0) {
                printf("Envio interrompido.\n");
                break;
            }

            fprintf(stdout, "Resposta -> %s", buffer);
            fflush(stdout);
        }

        closesocket(socket_ligacao);
        printf("Ligacao terminada. A aguardar outro cliente...\n");
    }
}
// Garante o envio de todos os bytes
int writeN(SOCKET sock, char* buffer, int nbytes)
{
    int nLeft = nbytes;
    int nWritten;
    int totalWritten = 0;

    while (nLeft > 0) {
        nWritten = send(sock, buffer, nLeft, 0);
        if (nWritten == 0 || nWritten == SOCKET_ERROR)
            return nWritten;

        nLeft -= nWritten;
        buffer += nWritten;
        totalWritten += nWritten;
    }

    return totalWritten;
}

// Le ate '\n' ou ate encher o buffer; ignora '\r'
int readLine(SOCKET sock, char* buffer, int tamMax)
{
    char c;
    int nRead, i = 0;

    if (tamMax < 2) {
        WSASetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }

    buffer[0] = '\0';

    while (i < tamMax - 1) {
        nRead = recv(sock, &c, sizeof(c), 0);
        if (nRead == SOCKET_ERROR || nRead == 0)
            return nRead;

        if (c == '\r')
            continue;

        buffer[i++] = c;
        buffer[i] = '\0';

        if (c == '\n')
            break;
    }

    return i;
}