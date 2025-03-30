
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <winsock2.h>
#include <fstream>
#include <string>
#include <vector>
#include <ws2tcpip.h> // ��� WSADATA, ������� WSAStartup, WSACleanup �� ������ ���� ������
using namespace std;

// ��������� �������� Winsock 2 DLL
using namespace std;

#pragma comment(lib,"ws2_32.lib") // ���������� winsock
#pragma warning(disable:4996) 

#define BUFLEN 1024 // ������ ������
#define PORT 34985 // ���� �������

enum MazeObject { HALL, WALL, COIN, ENEMY, BORDER };
enum Color { DARKGREEN = 2, YELLOW = 14, RED = 12, BLUE = 9, WHITE = 15, DARKYELLOW = 6, DARKRED = 4 };
enum KeyCode { ENTER = 13, ESCAPE = 27, SPACE = 32, LEFT = 75, RIGHT = 77, UP = 72, DOWN = 80 };

SOCKET ConnectSocket = INVALID_SOCKET;





class maze
{
public:
	std::string mazeMessage;

	maze()
	{
		srand(time(0));




		mazeData.resize(MAZE_HEIGHT, std::vector<int>(MAZE_WIDTH));

		string mazeGenCommand = " \".\\mazegen\\maze_generator.exe\" "; //������ ����: � � ���� ���������� (maze_generator.exe) ������� 4 ���� ������ ����� ��������������, � � ��� ���� ��������? ������ cmake ������� ����� :)
		mazeGenCommand.append(to_string(WIDTH) + " " + to_string(HEIGHT));
		mazeGenCommand.append(" maze.txt");

		system(mazeGenCommand.c_str()); //������ ���������� ���������, ������� ������� �������� � ���� maze.txt

		FILE* file = NULL;

		if (fopen_s(&file, ".\\maze.txt", "r") != 0)
		{
			exit(-1);
		}

		for (int y = 0; y < MAZE_HEIGHT; y++) // ������� �����
		{
			for (int x = 0; x < MAZE_WIDTH; x++) // ������� ��������
			{
				char tile = fgetc(file);


				if (tile != '#' && tile != ' ')
				{
					tile = fgetc(file);
				}
				if (tile == '#')
				{
					mazeData[y][x] = MazeObject::WALL;
				}
				if (tile == ' ')
				{
					mazeData[y][x] = MazeObject::HALL;
				}

				if (x == 1 && y == 2) mazeData[y][x] = MazeObject::HALL; // ����

				if (exitPos == 0)
				{
					if (x == MAZE_WIDTH - 1 && y == 1 || x == MAZE_WIDTH - 2 && y == 1)
					{
						mazeData[y][x] = MazeObject::HALL;
					}
				}
				else
				{
					if (x == MAZE_WIDTH - 1 && y == MAZE_HEIGHT - 2 || x == MAZE_WIDTH - 2 && y == MAZE_HEIGHT - 2)
					{
						mazeData[y][x] = MazeObject::HALL;
					}
				}
				// �����

			}
		}

		fclose(file);



		for (int i = 0; i < MAZE_HEIGHT; i++)
		{
			for (int j = 0; j < MAZE_WIDTH; j++)
			{
				mazeMessage.push_back(mazeData[i][j] + '0');
			}
		}
	}

	string getWidthHeight()
	{
		return (to_string(MAZE_WIDTH) + " " + to_string(MAZE_HEIGHT));
	}

	string getMazeMessage()
	{
		return mazeMessage;
	}

	std::vector<vector<int>> getMazeData()
	{
		return mazeData;
	}

	bool getExitPos()
	{
		return exitPos;
	}

	int getMazeWidth()
	{
		return MAZE_WIDTH;
	}

	int getMazeHeight()
	{
		return MAZE_HEIGHT;
	}

private:
	const int WIDTH = 20; // ������ ���������
	const int HEIGHT = 10; // ������ ���������

	const int MAZE_WIDTH = WIDTH * 2 + 1;
	const int MAZE_HEIGHT = HEIGHT * 2 + 1;

	int exitPos = (rand() % 2); //0 - ����� ������, 1 - ����� �����

	std::vector<vector<int>>mazeData = {}; // maze - �������� ��-��������� //������-�� ��������������� �������� ������, ��� �������� (���� �� ��������� � ����������� ����� � �������� ��� �������)

};

bool send(SOCKET& ClientSocket, std::string answer)
{
	int iSendResult = send(ClientSocket, answer.c_str(), answer.size(), 0);

	if (iSendResult == SOCKET_ERROR) {
		cout << "send �� ������� � ��������: " << WSAGetLastError() << "\n";
		cout << "���, �i������� (send) �i����i����� ���i�������� �� �i������� ((\n";
		closesocket(ClientSocket);
		WSACleanup();
		return 0;
	}
	return 1;
}

void safeCrash(SOCKET& ConnectSocket)
{
	int iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		std::cout << "shutdown �� ������� � ��������: " << WSAGetLastError() << "\n";
		closesocket(ConnectSocket);
		WSACleanup();
		exit(16);
	}
	else {
		std::cout << "������ ��i���� i�i�i�� �������� �'������� � ��������.\n";
	}


	// �������
	closesocket(ConnectSocket);
	WSACleanup();

	std::cout << "������ ��i���� ������� ���� ������!\n";

	exit(0);
}

int main() {
	setlocale(LC_ALL, "");

	maze maize;

	// ����������� Winsock
	WSADATA wsaData; // ��������� WSADATA ������ ���������� ��� ��������� Windows Sockets: https://docs.microsoft.com/en-us/windows/win32/api/winsock/ns-winsock-wsadata
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // ������� WSAStartup ������ ������������ �������� Winsock DLL ��������: https://firststeps.ru/mfc/net/socket/r.php?2
	if (iResult != 0) {
		cout << "WSAStartup �� ������� � ��������: " << iResult << "\n";
		cout << "�i��������� Winsock.dll ������� � ��������!\n";
		return 1;
	}
	else {
		cout << "�i��������� Winsock.dll ������� ���i���!\n";

	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct addrinfo hints; // ��������� addrinfo ��������������� �������� getaddrinfo ��� ��������� ���������� ��� ������ �����: https://docs.microsoft.com/en-us/windows/win32/api/ws2def/ns-ws2def-addrinfoa
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET; // ������ �������� ��������� ��������� ���� 4 (IPv4)
	hints.ai_socktype = SOCK_STREAM; // ��������� ��������, �����, ����������, �������� �� �'������ ������ ����� � ��������� �������� �����
	hints.ai_protocol = IPPROTO_TCP; // �������� �������� ��������� (TCP). �� ������� ��������, ���� ���� ai_family � AF_INET ��� AF_INET6, � ���� ai_socktype � SOCK_STREAM
	hints.ai_flags = AI_PASSIVE; // ������ ������ ���� ����������������� � ������� ������� "bind"



	// ���������� ������ �� ����� �������
	struct addrinfo* result = NULL;
	iResult = getaddrinfo(NULL, to_string(PORT).c_str(), &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo �� ������� � ��������: " << iResult << "\n";
		cout << "��������� ������ �� ����� ������� ������� � ��������!\n";
		WSACleanup(); // ������� WSACleanup ������� ������������ �������� Winsock 2 DLL (Ws2_32.dll): https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsacleanup
		return 2;
	}
	else {
		cout << "��������� ������ �� ����� ������� ������� ���i���!\n";

	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// ��������� ������ ��� ���������� �� �������
	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "socket �� ������� �������� � ��������: " << WSAGetLastError() << "\n";
		cout << "��������� ������ ������� � ��������!\n";
		freeaddrinfo(result);
		WSACleanup();

		return 3;
	}
	else {
		cout << "��������� ������ �� ������i ������� ���i���!\n";

	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// ������������ ������ ��� ���������������
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen); // ������� bind ������� �������� ������ � �������: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-bind
	if (iResult == SOCKET_ERROR) {
		cout << "bind �� ������� � ��������: " << WSAGetLastError() << "\n";
		cout << "������� ������ �� IP-�����i ������� � ��������!\n";
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 4;
	}
	else {
		cout << "������������ ������ �� IP-�����i ������� ���i���!\n";

	}

	freeaddrinfo(result);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	iResult = listen(ListenSocket, SOMAXCONN); // ������� listen ������� ����� � ����, ���� �� ���� ������ ����������: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
	if (iResult == SOCKET_ERROR) {
		cout << "listen �� ������� � ��������: " << WSAGetLastError() << "\n";
		cout << "��������������� i��������� �i� ��i���� �� ��������. ���� �i��� �� ���!\n";
		closesocket(ListenSocket);
		WSACleanup();
		return 5;
	}
	else {
		cout << "���������� ��������������� i�������i� �i� ��i����. ���� �����, ������i�� ��i������� ��������! (client.exe)\n";
		// ��� ����� ���� � ��������� ������ ��������� � �������� ������
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// ������ ������ �볺���
	SOCKET ClientSocket = INVALID_SOCKET;
	ClientSocket = accept(ListenSocket, NULL, NULL); // ������� accept �������� �������� ������ ���������� �� �����: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-accept
	if (ClientSocket == INVALID_SOCKET) {
		cout << "accept �� ������� � ��������: " << WSAGetLastError() << "\n";
		cout << "�i��������� � ��i�������� ��������� �� �����������! �����, ����� �� �� ���� ((\n";
		closesocket(ListenSocket);
		WSACleanup();
		return 6;
	}
	else {
		cout << "�i��������� � ��i�������� ��������� ����������� ���i���!\n";
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// ����� �� ������� �������� ����� �������
	closesocket(ListenSocket);


	COORD position; // ���������� ������ ���������
	position.X = 1;
	position.Y = 2;

	// ��������� ����� �� ���� �������, ���� �'������� �� ���� ���������
	do {

		char message[BUFLEN];

		iResult = recv(ClientSocket, message, BUFLEN, 0); // ������� recv ��������������� ��� ������� ������� �����: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recv
		if (iResult == INVALID_SOCKET)
		{
			cout << "Error:" << WSAGetLastError() << endl;
			safeCrash(ConnectSocket);
		}
		message[iResult] = '\0';





		if (iResult > 0)
		{
			if (strcmp(message, "Move pls") == 0)
			{
				//send(ClientSocket, "OK where");

				iResult = recv(ClientSocket, message, BUFLEN, 0); // ������� recv ��������������� ��� ������� ������� �����: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recv
				if (iResult == INVALID_SOCKET)
				{
					cout << "Error:" << WSAGetLastError() << endl;
					safeCrash(ConnectSocket);
				}
				message[iResult] = '\0';



				if (strcmp(message, "u") == 0)
				{
					if ((maize.getMazeData()[position.Y - 1][position.X]) == MazeObject::HALL)
					{
						send(ClientSocket, "ok");
						position.Y--;
					}
				}
				else if (strcmp(message, "d") == 0)
				{
					if ((maize.getMazeData()[position.Y + 1][position.X]) == MazeObject::HALL)
					{
						send(ClientSocket, "ok");
						position.Y++;
					}
				}
				else if (strcmp(message, "l") == 0)
				{
					if ((maize.getMazeData()[position.Y][position.X - 1]) == MazeObject::HALL)
					{
						send(ClientSocket, "ok");
						position.X--;
					}
				}
				else if (strcmp(message, "r") == 0)
				{
					if ((maize.getMazeData()[position.Y][position.X + 1]) == MazeObject::HALL)
					{
						send(ClientSocket, "ok");
						position.X++;
					}
				}

			}
			else if (strcmp(message, "Maze pls") == 0)
			{
				std::cout << "Recieved the request for the maze\n";

				string widthHeight = maize.getWidthHeight();


				send(ClientSocket, widthHeight);

				std::cout << "Sent the maze size,waiting for OK to continue\n";

				iResult = recv(ClientSocket, message, BUFLEN, 0);
				if (iResult == SOCKET_ERROR)
				{
					safeCrash(ConnectSocket);
				}
				message[iResult] = '\0';

				if (strcmp(string(message).c_str(), "ok") == 0)
				{
					send(ClientSocket, maize.getMazeMessage().c_str());
				}
				// �������� ���������
				std::cout << "\n Done sending the maze \n";


			}
			else if (strcmp(message, "Coords pls") == 0)
			{
				std::cout << "Recieved the request for the coords\n";

				std::string coords = to_string(position.X) + " " + to_string(position.Y);
				send(ClientSocket, coords);
			}
			else if (strcmp(message, "Win pls") == 0)
			{
				std::cout << "Recieved the request for the win\n";

				if (maize.getExitPos() == 0)
				{
					if (position.Y == 1 && position.X == maize.getMazeWidth() - 1)
					{
						Sleep(1000);
						send(ConnectSocket, "ok");
						cout << "#1 Victory royale!!! Good job player!\n";

						break;
					}
					else
					{
						send(ConnectSocket, "no");
					}
				}
				else
				{
					if (position.Y == maize.getMazeHeight() - 2 && position.X == maize.getMazeWidth() - 1)
					{
						Sleep(1000);
						send(ConnectSocket, "ok");
						cout << "#1 Victory royale!!! Good job player!\n";

						break;
					}
					else
					{
						send(ConnectSocket, "no");
					}

				}
			}

			else if (iResult == 0) {
				cout << "�'������� �����������...\n";

			}
			else {
				cout << "recv �� ������� � ��������: " << WSAGetLastError() << "\n";
				cout << "���, ��������� (recv) �i����i����� ���i�������� �� �i������� ((\n";
				closesocket(ClientSocket);
				WSACleanup();
				return 8;
			}
		}
	} while (iResult > 0);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// �������� �'�������, ������� ������ ���������
	iResult = shutdown(ClientSocket, SD_SEND); // ������� shutdown ������ ���������� ��� ��������� ����� �� �����: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-shutdown
	if (iResult == SOCKET_ERROR) {
		cout << "shutdown �� ������� � ��������: " << WSAGetLastError() << "\n";
		cout << "���, ������ �'������� (shutdown) ����� ������� ((\n";
		closesocket(ClientSocket);
		WSACleanup();
		return 9;
	}
	else {
		cout << "������ ������� �������� ���� ������! �� ����� ������i�! :)\n";
	}

	// ���������� �����
	closesocket(ClientSocket);
	WSACleanup();

	return 0;




}