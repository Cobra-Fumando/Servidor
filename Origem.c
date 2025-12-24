#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

typedef struct {
	int id;
	char name[50];
	int idade;
	char cpf[15];
	float salario;

} Cadastro;

typedef struct {
	int* executado;
	SOCKET* sock;
} servidor;

WSADATA wsaData;
SOCKET sock;


DWORD WINAPI Receber(LPVOID lpParam) {
	servidor* clientSockPtr = (servidor*)lpParam;
	char buffer[1024];
	int bytesReceived;

	while (1) {
		memset(buffer, 0, sizeof(buffer));
		bytesReceived = recv(*clientSockPtr->sock, buffer, sizeof(buffer), 0);

		if (bytesReceived > 0) {
			buffer[bytesReceived] = '\0';
			printf("Mensagem recebida do cliente: %s\n", buffer);
		}
		else {
			printf("cliente desconectou\n");
			*clientSockPtr->executado = 1;
			break;
		}
	}

	closesocket(*clientSockPtr->sock);
	return 0;
}

DWORD WINAPI Enviar(LPVOID lpParam) {
	servidor* clientSockPtr = (servidor*)lpParam;
	char buffer[1024];

	while (1) {

		memset(buffer, 0, sizeof(buffer));
		printf("Digite uma mensagem para enviar ao cliente: ");
		fgets(buffer, sizeof(buffer), stdin);
		buffer[strcspn(buffer, "\n")] = '\0';

		if (*clientSockPtr->executado == 1) {
			free(clientSockPtr->executado);
			free(clientSockPtr->sock);
			free(clientSockPtr);
			break;
		}

		send(*clientSockPtr->sock, buffer, strlen(buffer), 0);
	}

	return 0;

}

int Iniciar() {
	int iResult;

	struct sockaddr_in serverAddr;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	printf("Winsock initialized successfully.\n");

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		printf("Erro ao inicializar o socket\n");
		WSACleanup();
		return 1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(2040);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		printf("Erro no bind\n");
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	listen(sock, 5);

	printf("Servidor iniciado e aguardando conexoes...\n");


	HANDLE hthread;
	HANDLE hthread1;

	while (1) {
		printf("Aguardando novas conexões\n");
		SOCKET* clientSock = (SOCKET*)malloc(sizeof(SOCKET));
		*clientSock = accept(sock, NULL, NULL);

		if (*clientSock != INVALID_SOCKET) {
			printf("Cliente conectado com sucesso\n");

			servidor* serve = (servidor*)malloc(sizeof(servidor));
			serve->executado = (int*)malloc(sizeof(int));
			*serve->executado = 0;
			serve->sock = clientSock;

			hthread = CreateThread(NULL, 0, Receber, serve, 0, NULL);
			hthread1 = CreateThread(NULL, 0, Enviar, serve, 0, NULL);


			if (hthread == NULL || hthread1 == NULL) {
				return 1;
			}
		}


	}
	WaitForSingleObject(hthread, INFINITE);
	WaitForSingleObject(hthread1, INFINITE);

	CloseHandle(hthread1);
	CloseHandle(hthread);

	closesocket(sock);
	WSACleanup();
	return 0;
}

int main() {

	int result = Iniciar();

	if (result == 1) {
		return 1;
	}

	Cadastro cadastro;
	cadastro.id = 1;
	cadastro.idade = 23;
	cadastro.salario = 2500.50;

	strncpy(cadastro.name, "Joao Silva", sizeof(cadastro.name));
	cadastro.name[strcspn(cadastro.name, "\n")] = '\0';

	strncpy(cadastro.cpf, "49323512", sizeof(cadastro.cpf));
	cadastro.cpf[strcspn(cadastro.cpf, "\n")] = '\0';

	return 0;
}