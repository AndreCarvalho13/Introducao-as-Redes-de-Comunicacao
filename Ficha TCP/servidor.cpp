#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define SERV_TCP_PORT 50000
#define BUFFER_SIZE 1024

int writeN(SOCKET sock, char* buffer, int nbytes);
int readLine(SOCKET sock, char* buffer, int tamMax);
DWORD WINAPI Func(LPVOID param);

int main(void)
{
    WSADATA wsa_data;
    SOCKET socket_servidor, socket_ligacao;
    struct sockaddr_in info_servidor;
    SECURITY_ATTRIBUTES sa;
    HANDLE thread;
    int res;

    // Atributos de seguranca da thread
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = FALSE;

    // Obrigatorio no Windows
    res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (res != 0) {
        fprintf(stderr, "\n<SERVIDOR> Falha ao iniciar o winsock (%d)\n", res);
        exit(EXIT_FAILURE);
    }

    // Cria socket IPv4 do tipo stream
    socket_servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_servidor == INVALID_SOCKET) {
        fprintf(stderr, "\n<SERVIDOR> Falha ao criar o socket (%d)\n",
            WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Preenche os dados do servidor
    memset(&info_servidor, 0, sizeof(info_servidor));
    info_servidor.sin_family = AF_INET;
    info_servidor.sin_addr.s_addr = htonl(INADDR_ANY);
    info_servidor.sin_port = htons(SERV_TCP_PORT);

    // Vincula o socket ao porto
    res = bind(socket_servidor, (struct sockaddr*)&info_servidor,
        sizeof(info_servidor));
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "\n<SERVIDOR> Falha na associacao (%d)\n",
            WSAGetLastError());
        closesocket(socket_servidor);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Coloca o socket em modo de escuta
    res = listen(socket_servidor, SOMAXCONN);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "\n<SERVIDOR> Falha na escuta (%d)\n",
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
            fprintf(stderr, "\n<SERVIDOR> Falha no accept (%d)\n",
                WSAGetLastError());
            continue;
        }

        // Reserva um espaco exclusivo para o socket deste cliente
        SOCKET* parametro = (SOCKET*)malloc(sizeof(SOCKET));
        if (parametro == NULL) {
            fprintf(stderr, "\n<SERVIDOR> Falha ao reservar memoria.\n");
            closesocket(socket_ligacao);
            continue;
        }

        *parametro = socket_ligacao;

        // Cria uma thread para atender este cliente
        thread = CreateThread(
            &sa,
            0,
            Func,
            (LPVOID)parametro,
            0,
            NULL
        );

        if (thread == NULL) {
            fprintf(stderr, "\n<SERVIDOR> Falha ao criar a thread (%lu)\n",
                GetLastError());
            free(parametro);
            closesocket(socket_ligacao);
            continue;
        }

        // Liberta o handle, sem terminar a thread
        CloseHandle(thread);
    }
}

// Cada cliente executa esta funcao numa thread diferente
DWORD WINAPI Func(LPVOID param)
{
    SOCKET socket_ligacao = *(SOCKET*)param;
    free(param);

    // Variaveis exclusivas desta thread
    struct sockaddr_in info_cliente;
    int tamanho = sizeof(info_cliente);
    int res, bytes_recebidos, bytes_enviados;
    char buffer[BUFFER_SIZE];
    char ip[INET_ADDRSTRLEN];
    unsigned short porto;

    // Obtem o IP e o porto do cliente ligado
    res = getpeername(socket_ligacao,
        (struct sockaddr*)&info_cliente,
        &tamanho);

    if (res == SOCKET_ERROR) {
        fprintf(stderr, "\n<SERVIDOR> Falha no getpeername (%d)\n",
            WSAGetLastError());
        closesocket(socket_ligacao);
        return 0;
    }

    // Converte o endereco IP para texto
    if (inet_ntop(AF_INET, &info_cliente.sin_addr,
        ip, sizeof(ip)) == NULL) {
        fprintf(stderr, "\n<SERVIDOR> Falha ao converter o IP (%d)\n",
            WSAGetLastError());
        closesocket(socket_ligacao);
        return 0;
    }

    // Converte o porto da ordem de rede para a ordem do computador
    porto = ntohs(info_cliente.sin_port);

    printf("Cliente ligado: %s:%u\n", ip, (unsigned int)porto);

    // Recebe varias mensagens deste cliente
    while (1) {
        bytes_recebidos = readLine(socket_ligacao, buffer, BUFFER_SIZE);

        if (bytes_recebidos == SOCKET_ERROR) {
            fprintf(stderr, "\n<SERVIDOR> Falha na rececao de %s:%u (%d)\n",
                ip, (unsigned int)porto, WSAGetLastError());
            break;
        }

        if (bytes_recebidos == 0) {
            printf("Cliente %s:%u encerrou a ligacao.\n",
                ip, (unsigned int)porto);
            break;
        }

        // Confirma que recebeu uma linha completa
        if (buffer[bytes_recebidos - 1] != '\n') {
            fprintf(stderr, "\n<SERVIDOR> Mensagem demasiado longa de %s:%u.\n",
                ip, (unsigned int)porto);
            break;
        }

        // Termina apenas a ligacao deste cliente
        if (strcmp(buffer, "SAIR\n") == 0) {
            printf("Cliente %s:%u pediu para sair.\n",
                ip, (unsigned int)porto);
            break;
        }

        // Remove o '\n' e desconta-o do tamanho
        bytes_recebidos--;
        buffer[bytes_recebidos] = '\0';

        fprintf(stdout, "Mensagem de %s:%u -> %s\n",
            ip, (unsigned int)porto, buffer);
        fflush(stdout);

        // Prepara a resposta com o tamanho em ASCII
        sprintf_s(buffer, sizeof(buffer), "%d\n", bytes_recebidos);

        bytes_enviados = writeN(socket_ligacao, buffer,
            (int)strlen(buffer));

        if (bytes_enviados == SOCKET_ERROR) {
            fprintf(stderr, "\n<SERVIDOR> Falha na transmissao (%d)\n",
                WSAGetLastError());
            break;
        }

        if (bytes_enviados == 0) {
            printf("Envio interrompido para %s:%u.\n",
                ip, (unsigned int)porto);
            break;
        }

        fprintf(stdout, "Resposta para %s:%u -> %s",
            ip, (unsigned int)porto, buffer);
        fflush(stdout);
    }

    closesocket(socket_ligacao);

    printf("Ligacao com %s:%u terminada.\n",
        ip, (unsigned int)porto);

    return 0;
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
// Guarda o '\n' e termina a string com '\0'
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