
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <winsock2.h>
#include <fstream>
#include <string>
#include <vector>
#include <ws2tcpip.h> // тип WSADATA, функції WSAStartup, WSACleanup та багато чого іншого
using namespace std;

// реалізація бібліотеки Winsock 2 DLL
using namespace std;

#pragma comment(lib,"ws2_32.lib") // библиотека winsock
#pragma warning(disable:4996) 

#define BUFLEN 1024 // размер буфера
#define PORT 34985 // порт сервера

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

		string mazeGenCommand = " \".\\mazegen\\maze_generator.exe\" "; //Весёлый факт: я с этой программой (maze_generator.exe) возился 4 часа просто чтобы скомпилировать, и в чём была проблема? версия cmake слишком новая :)
		mazeGenCommand.append(to_string(WIDTH) + " " + to_string(HEIGHT));
		mazeGenCommand.append(" maze.txt");

		system(mazeGenCommand.c_str()); //запуск генератора лабиринта, который выведет лабиринт в файл maze.txt

		FILE* file = NULL;

		if (fopen_s(&file, ".\\maze.txt", "r") != 0)
		{
			exit(-1);
		}

		for (int y = 0; y < MAZE_HEIGHT; y++) // перебор строк
		{
			for (int x = 0; x < MAZE_WIDTH; x++) // перебор столбцов
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

				if (x == 1 && y == 2) mazeData[y][x] = MazeObject::HALL; // вход

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
				// выход

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
	const int WIDTH = 20; // ширина лабиринта
	const int HEIGHT = 10; // высота лабиринта

	const int MAZE_WIDTH = WIDTH * 2 + 1;
	const int MAZE_HEIGHT = HEIGHT * 2 + 1;

	int exitPos = (rand() % 2); //0 - выход сверху, 1 - выход снизу

	std::vector<vector<int>>mazeData = {}; // maze - лабиринт по-английски //почему-то сгенерированный лабиринт больше, чем заказано (даже на страничке с генератором можно в примерах это увидеть)

};

bool send(SOCKET& ClientSocket, std::string answer)
{
	int iSendResult = send(ClientSocket, answer.c_str(), answer.size(), 0);

	if (iSendResult == SOCKET_ERROR) {
		cout << "send не вдалося з помилкою: " << WSAGetLastError() << "\n";
		cout << "упс, вiдправка (send) вiдповiдного повiдомлення не вiдбулася ((\n";
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
		std::cout << "shutdown не вдалося з помилкою: " << WSAGetLastError() << "\n";
		closesocket(ConnectSocket);
		WSACleanup();
		exit(16);
	}
	else {
		std::cout << "процес клiєнта iнiцiює закриття з'єднання з сервером.\n";
	}


	// очистка
	closesocket(ConnectSocket);
	WSACleanup();

	std::cout << "процес клiєнта завершує свою роботу!\n";

	exit(0);
}

int main() {
	setlocale(LC_ALL, "");

	maze maize;

	// ініціалізація Winsock
	WSADATA wsaData; // Структура WSADATA містить інформацію про реалізацію Windows Sockets: https://docs.microsoft.com/en-us/windows/win32/api/winsock/ns-winsock-wsadata
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // Функція WSAStartup ініціює використання бібліотеки Winsock DLL процесом: https://firststeps.ru/mfc/net/socket/r.php?2
	if (iResult != 0) {
		cout << "WSAStartup не вдалося з помилкою: " << iResult << "\n";
		cout << "пiдключення Winsock.dll пройшло з помилкою!\n";
		return 1;
	}
	else {
		cout << "пiдключення Winsock.dll пройшло успiшно!\n";

	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct addrinfo hints; // структура addrinfo використовується функцією getaddrinfo для зберігання інформації про адресу хоста: https://docs.microsoft.com/en-us/windows/win32/api/ws2def/ns-ws2def-addrinfoa
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET; // адреса сімейства протоколу Інтернету версії 4 (IPv4)
	hints.ai_socktype = SOCK_STREAM; // забезпечує послідовні, надійні, двосторонні, засновані на з'єднанні потоки байтів з механізмом передачі даних
	hints.ai_protocol = IPPROTO_TCP; // протокол передачі управління (TCP). Це можливе значення, коли член ai_family є AF_INET або AF_INET6, а член ai_socktype — SOCK_STREAM
	hints.ai_flags = AI_PASSIVE; // адреса сокета буде використовуватися в виклику функції "bind"



	// визначення адреси та порту сервера
	struct addrinfo* result = NULL;
	iResult = getaddrinfo(NULL, to_string(PORT).c_str(), &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo не вдалося з помилкою: " << iResult << "\n";
		cout << "отримання адреси та порту сервера пройшло з помилкою!\n";
		WSACleanup(); // функція WSACleanup завершує використання бібліотеки Winsock 2 DLL (Ws2_32.dll): https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsacleanup
		return 2;
	}
	else {
		cout << "отримання адреси та порту сервера пройшло успiшно!\n";

	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// створення сокету для підключення до сервера
	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "socket не вдалося створити з помилкою: " << WSAGetLastError() << "\n";
		cout << "створення сокета пройшло з помилкою!\n";
		freeaddrinfo(result);
		WSACleanup();

		return 3;
	}
	else {
		cout << "створення сокета на серверi пройшло успiшно!\n";

	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// налаштування сокета для прослуховування
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen); // Функція bind асоціює локальну адресу з сокетом: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-bind
	if (iResult == SOCKET_ERROR) {
		cout << "bind не вдалося з помилкою: " << WSAGetLastError() << "\n";
		cout << "вбудова сокета по IP-адресi пройшла з помилкою!\n";
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 4;
	}
	else {
		cout << "впровадження сокету по IP-адресi пройшло успiшно!\n";

	}

	freeaddrinfo(result);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	iResult = listen(ListenSocket, SOMAXCONN); // функція listen ставить сокет у стан, коли він чекає вхідне підключення: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
	if (iResult == SOCKET_ERROR) {
		cout << "listen не вдалося з помилкою: " << WSAGetLastError() << "\n";
		cout << "прослуховування iнформації вiд клiєнта не почалося. щось пiшло не так!\n";
		closesocket(ListenSocket);
		WSACleanup();
		return 5;
	}
	else {
		cout << "починається прослуховування iнформацiї вiд клiєнта. будь ласка, запустiть клiєнтську програму! (client.exe)\n";
		// тут можна було б запустити якийсь прелоадер в окремому потоці
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// прийом сокета клієнта
	SOCKET ClientSocket = INVALID_SOCKET;
	ClientSocket = accept(ListenSocket, NULL, NULL); // Функція accept дозволяє прийняти вхідне підключення на сокет: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-accept
	if (ClientSocket == INVALID_SOCKET) {
		cout << "accept не вдалося з помилкою: " << WSAGetLastError() << "\n";
		cout << "пiдключення з клiєнтською програмою не встановлено! сумно, сумно аж за край ((\n";
		closesocket(ListenSocket);
		WSACleanup();
		return 6;
	}
	else {
		cout << "пiдключення з клiєнтською програмою встановлено успiшно!\n";
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// більше не потрібно зберігати сокет сервера
	closesocket(ListenSocket);


	COORD position; // координаты нашего персонажа
	position.X = 1;
	position.Y = 2;

	// отримання даних до того моменту, поки з'єднання не буде завершено
	do {

		char message[BUFLEN];

		iResult = recv(ClientSocket, message, BUFLEN, 0); // функція recv використовується для читання вхідних даних: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recv
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

				iResult = recv(ClientSocket, message, BUFLEN, 0); // функція recv використовується для читання вхідних даних: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recv
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
				// Отправка лабиринта
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
				cout << "з'єднання закривається...\n";

			}
			else {
				cout << "recv не вдалося з помилкою: " << WSAGetLastError() << "\n";
				cout << "упс, отримання (recv) вiдповiдного повiдомлення не вiдбулося ((\n";
				closesocket(ClientSocket);
				WSACleanup();
				return 8;
			}
		}
	} while (iResult > 0);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// закриття з'єднання, оскільки роботу завершено
	iResult = shutdown(ClientSocket, SD_SEND); // функція shutdown вимикає надсилання або отримання даних на сокеті: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-shutdown
	if (iResult == SOCKET_ERROR) {
		cout << "shutdown не вдалося з помилкою: " << WSAGetLastError() << "\n";
		cout << "упс, розрив з'єднання (shutdown) видав помилку ((\n";
		closesocket(ClientSocket);
		WSACleanup();
		return 9;
	}
	else {
		cout << "процес сервера припиняє свою роботу! до нових запускiв! :)\n";
	}

	// вивільнення памяті
	closesocket(ClientSocket);
	WSACleanup();

	return 0;




}