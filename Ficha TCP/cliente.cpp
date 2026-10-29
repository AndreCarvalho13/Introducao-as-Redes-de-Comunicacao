#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

int writeN(SOCKET sock, char* buffer, int nbytes);
int readLine(SOCKET sock, char* buffer, int tamMax);

int main(int argc, char* argv[])
{
    WSADATA wsa_data;
    SOCKET socket_cliente;
    struct sockaddr_in info_servidor;
    int res, bytes_recebidos, bytes_enviados;
    char buffer[BUFFER_SIZE];
    struct addrinfo hints;
	struct addrinfo* result;

    // Verifica se foi passada a mensagem a enviar
    if (argc != 4) {
        fprintf(stdout,
            "\nSintaxe:\nexecutavel <ip> <porto> \"frase a enviar\"\n\n");
        exit(EXIT_FAILURE);
    }

    // Reserva espaco para o '\n' e o '\0' no buffer do servidor
    if (strlen(argv[3]) > BUFFER_SIZE - 2 ||
        strchr(argv[3], '\n') != NULL ||
        strchr(argv[3], '\r') != NULL) {
        fprintf(stderr,
            "\n<CLIENTE> Use uma frase ate %d bytes, sem quebras de linha.\n",
            BUFFER_SIZE - 2);
        exit(EXIT_FAILURE);
    }

    // Obrigatorio no Windows
    res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (res != 0) {
        fprintf(stderr, "\n<CLIENTE> Erro: Falha ao iniciar o winsock (%d)\n", res);
        exit(EXIT_FAILURE);
    }

    // Cria socket IPv4 do tipo stream
    socket_cliente = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_cliente == INVALID_SOCKET) {
        fprintf(stderr, "\n<CLIENTE> Erro: Falha ao iniciar o socket (%d)\n",
            WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Preenchimento da estrutura com os dados do servidor
    memset(&info_servidor, 0, sizeof(info_servidor));
    info_servidor.sin_family = AF_INET;
    info_servidor.sin_port = htons(atoi(argv[2]));
    // Resolve o nome do servidor para um endereco IPv4
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    res = getaddrinfo(argv[1], NULL, &hints, &result);
    if (res != 0) {
        fprintf(stderr, "\n<CLIENTE> Erro: Nao foi possivel resolver o servidor (%d)\n",
            res);
        closesocket(socket_cliente);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Copia o primeiro endereco IPv4 encontrado
    info_servidor.sin_addr =
        ((struct sockaddr_in*)result->ai_addr)->sin_addr;

    // Liberta a memoria reservada por getaddrinfo
    freeaddrinfo(result);

    // Estabelece a ligacao com o servidor
    res = connect(socket_cliente, (struct sockaddr*)&info_servidor,
        sizeof(info_servidor));
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "\n<CLIENTE> Erro: Falha na ligacao (%d)\n",
            WSAGetLastError());
        closesocket(socket_cliente);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Prepara a frase com o delimitador de fim de linha
    sprintf_s(buffer, sizeof(buffer), "%s\n", argv[3]);

    // Envia a mensagem completa, incluindo o '\n'
    bytes_enviados = writeN(socket_cliente, buffer, (int)strlen(buffer));
    if (bytes_enviados == SOCKET_ERROR) {
        fprintf(stderr, "\n<CLIENTE> Erro: Falha na transmissao (%d)\n",
            WSAGetLastError());
        closesocket(socket_cliente);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    if (bytes_enviados == 0) {
        fprintf(stderr, "\n<CLIENTE> Erro: Envio interrompido.\n");
        closesocket(socket_cliente);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Mensagem enviada -> %s\n", argv[3]);

    // Aguarda pela resposta
    bytes_recebidos = readLine(socket_cliente, buffer, BUFFER_SIZE);
    if (bytes_recebidos == SOCKET_ERROR) {
        fprintf(stderr, "\n<CLIENTE> Erro: Falha na recepcao (%d)\n",
            WSAGetLastError());
        closesocket(socket_cliente);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    if (bytes_recebidos == 0) {
        fprintf(stderr, "\n<CLIENTE> Servidor encerrou a ligacao.\n");
        closesocket(socket_cliente);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    if (buffer[bytes_recebidos - 1] != '\n') {
        fprintf(stderr, "\n<CLIENTE> Erro: Resposta demasiado longa.\n");
        closesocket(socket_cliente);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Remove o '\n'; readLine ja acrescentou o '\0'
    buffer[bytes_recebidos - 1] = '\0';
    fprintf(stdout, "Resposta do servidor -> %s\n", buffer);

    closesocket(socket_cliente);
    WSACleanup();
    return EXIT_SUCCESS;
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