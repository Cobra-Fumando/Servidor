#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MaxClients 5

#pragma comment(lib, "ws2_32.lib")

CRITICAL_SECTION cs;

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

typedef struct {
	int id;
	SOCKET* sock;
} Cliente;

typedef void (*TaskFunc)(void*);

typedef struct {
	TaskFunc func;
	void* param;
} Task;


HANDLE semTarefas;
Task Fila[MaxClients];
int inicio = 0;
int Fim = 0;
int count = 0;

void Enfileirar(TaskFunc func, void* param) {
	EnterCriticalSection(&cs);

	if (count == MaxClients) {
		printf("Fila cheia\n");
		LeaveCriticalSection(&cs);
		return;
	}

	Fila[Fim].func = func;
	Fila[Fim].param = param;
	
	Fim = (Fim + 1) % MaxClients;
	count++;
	
	LeaveCriticalSection(&cs);
	ReleaseSemaphore(semTarefas, 1, NULL);
}

Task Desenfileirar() {
	Task task;

	if (count == 0) {
		printf("Fila vazia\n");
		task.func = NULL;
		task.param = NULL;
		return task;
	}

	task = Fila[inicio];
	inicio = (inicio + 1) % MaxClients;
	count--;

	return task;
}

DWORD WINAPI ExecutarTarefas(LPVOID lpParam) {
	while (1) {
		WaitForSingleObject(semTarefas, INFINITE);

		EnterCriticalSection(&cs);
		Task task = Desenfileirar();
		LeaveCriticalSection(&cs);

		if (task.func != NULL) {
			task.func(task.param);
		}
	}
}

void ReceberTarefas(LPVOID lpParam) {
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
			free(clientSockPtr->executado);
			free(clientSockPtr->sock);
			free(clientSockPtr);
			break;
		}
	}

	closesocket(*clientSockPtr->sock);
	return 0;
}

WSADATA wsaData;
SOCKET sock;

DWORD WINAPI Enviar(LPVOID lpParam) {
	servidor* clientSockPtr = (servidor*)lpParam;
	char buffer[1024];

	while (1) {

		memset(buffer, 0, sizeof(buffer));
		printf("Digite uma mensagem para enviar ao cliente: ");
		fgets(buffer, sizeof(buffer), stdin);
		buffer[strcspn(buffer, "\n")] = '\0';

		send(*clientSockPtr->sock, buffer, strlen(buffer), 0);
	}

	return 0;

}

int Iniciar() {
	int iResult;
	int TotalClient = 0;

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

	printf("Servidor iniciado\n");

	HANDLE hthread;
	HANDLE hthread1;

	Cliente clients[5];
	hthread = CreateThread(NULL, 0, ExecutarTarefas, NULL, 0, NULL);

	while (1) {

		printf("Aguardando novas conexões...\n");
		SOCKET* clientSock = (SOCKET*)malloc(sizeof(SOCKET));
		*clientSock = accept(sock, NULL, NULL);

		if (TotalClient >= 5) {
			printf("Numero maximo de clientes atingido. Conexao recusada.\n");
			closesocket(*clientSock);
			free(clientSock);
			continue;
		}

		if (*clientSock != INVALID_SOCKET) {
			printf("Cliente conectado com sucesso\n");

			servidor* serve = (servidor*)malloc(sizeof(servidor));
			serve->executado = (int*)malloc(sizeof(int));
			*serve->executado = 0;
			serve->sock = clientSock;

			clients[TotalClient].sock = clientSock;
			clients[TotalClient].id = TotalClient + 1;
			TotalClient++;

			Enfileirar(ReceberTarefas, serve);
			hthread1 = CreateThread(NULL, 0, Enviar, serve, 0, NULL);

			if (hthread == NULL || hthread1 == NULL) {
				return 1;
			}
		}
		else {
			printf("Erro ao aceitar conexao do cliente\n");
			closesocket(*clientSock);
			free(clientSock);
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
	InitializeCriticalSection(&cs);

	semTarefas = CreateSemaphore(NULL, 0, MaxClients, NULL);

	int result = Iniciar();

	if (result == 1) {
		return 1;
	}

	return 0;
}