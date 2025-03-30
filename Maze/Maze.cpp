
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <windows.h>
#include <conio.h>
#include <sstream>
#include <string>
#include <vector>
#include <io.h>
#include <fcntl.h>

using namespace std;

#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib") // подключение библиотеки winsock
#pragma warning(disable:4996)

#define SERVER "127.0.0.1" // ip-адрес сервера (локальный хост)
#define BUFLEN 1024 // максимальная длина ответа
#define PORT 34985 // порт для приема данных

enum MazeObject { HALL, WALL, COIN, ENEMY, BORDER };
enum Color { DARKGREEN = 2, YELLOW = 14, RED = 12, BLUE = 9, WHITE = 15, DARKYELLOW = 6, DARKRED = 4 };
enum KeyCode { ENTER = 13, ESCAPE = 27, SPACE = 32, LEFT = 75, RIGHT = 77, UP = 72, DOWN = 80 };





void updateMoveCounter(int& moves, COORD& infobox, HANDLE& h, const int MAZE_WIDTH)
{
	moves++;
	infobox.X = MAZE_WIDTH + 1;
	infobox.Y = 1;
	SetConsoleCursorPosition(h, infobox);
	SetConsoleTextAttribute(h, Color::DARKYELLOW);
	std::cout << "MOVES: ";
	SetConsoleTextAttribute(h, Color::YELLOW);
	std::cout << moves << "\n";
}

void updateCoords(COORD& position, COORD& infobox, HANDLE& h, const int MAZE_WIDTH)
{
	infobox.X = MAZE_WIDTH + 1;
	infobox.Y = 3;
	SetConsoleCursorPosition(h, infobox);
	SetConsoleTextAttribute(h, Color::WHITE);
	std::cout << "COORIDNATES: ";
	std::cout << "(" << position.X << "," << position.Y << ")\n";
}

bool send(SOCKET& ConnectSocket, std::string message)
{
	int iResult = send(ConnectSocket, message.c_str(), strlen(message.c_str()), 0);
	if (iResult == SOCKET_ERROR) {
		std::cout << "send не вдалося з помилкою: " << WSAGetLastError() << "\n";
		closesocket(ConnectSocket);
		WSACleanup();
		return 15;
	}
	else {
		//std::cout << "данi успiшно вiдправлено на сервер: " << message << "\n";
		//std::cout << "байтiв вiдправлено з клiєнта: " << iResult << "\n";

	}
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

vector<string> split(const string& s)
{
	stringstream ss(s);
	vector<string> sizes; // я даже не помню где я этот код нашел
	for (string w; ss >> w; ) sizes.push_back(w);
	return sizes;
}

bool tryMove(SOCKET& ConnectSocket, COORD& position, char direction)
{
	char direc[2];
	direc[0] = direction;
	direc[1] = '\0';

	send(ConnectSocket, "Move pls");

	send(ConnectSocket, direc);

	char answer[BUFLEN];

	int iResult = recv(ConnectSocket, answer, BUFLEN, 0);
	answer[iResult] = '\0';

	if (strcmp(answer, "ok") == 0)
	{
		switch (direction)
		{
		case 'd': // down
		{
			position.Y++;
			break;
		}
		case 'u': // up
		{
			position.Y--;
			break;
		}
		case 'l': // left
		{
			position.X--;
			break;
		}
		case 'r': // right
		{
			position.X++;
			break;
		}
		}

		return 1;
	}
	else
	{
		return 0;
	}
}

int main()
{

	locale::global(locale("en_US.UTF-8"));

	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

	// настройки шрифта консоли
	CONSOLE_FONT_INFOEX font; // https://docs.microsoft.com/en-us/windows/console/console-font-infoex
	font.cbSize = sizeof(font);
	font.dwFontSize.Y = 70;
	font.FontFamily = FF_DONTCARE;
	font.FontWeight = FW_NORMAL;
	wcscpy_s(font.FaceName, 9, L"Consolas");
	SetCurrentConsoleFontEx(h, 0, &font);

	// скрытие мигающего курсора 
	CONSOLE_CURSOR_INFO cursor;
	cursor.bVisible = false; // спрятать курсор
	cursor.dwSize = 1; // 1...100
	SetConsoleCursorInfo(h, &cursor);

	system("title Cool maze");
	MoveWindow(GetConsoleWindow(), 20, 60, 1850, 900, true);
	// 20 - отступ слева от левой границы рабочего стола до левой границы окна консоли (в пикселях!)
	// 60 - отступ сверху от верхней границы рабочего стола до верхней границы окна консоли
	// 1850 - ширина окна консоли в пикселях
	// 900 - высота окна консоли
	// true - перерисовать окно после перемещения
	std::cout << "процес клiєнта запущено!\n";


	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	// iніціалізація Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cout << "WSAStartup не вдалося з помилкою: " << iResult << "\n";
		return 11;
	}
	else {
		std::cout << "пiдключення до Winsock.dll пройшло успiшно!\n";

	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// отримання адреси сервера та порту
	const char* ip = "localhost"; // за замовчуванням, обидва додатки, і клієнт, і сервер, працюють на одній і тій самій машині

	// iResult = getaddrinfo(argv[1], PORT, &hints, &result); // раскоментувати, якщо потрібно буде зчитувати ім'я сервера з командного рядка
	addrinfo* result = NULL;
	iResult = getaddrinfo(ip, to_string(PORT).c_str(), &hints, &result);

	if (iResult != 0) {
		std::cout << "getaddrinfo не вдалося з помилкою: " << iResult << "\n";
		WSACleanup();
		return 12;
	}
	else {
		std::cout << "отримання адреси та порту клiєнта пройшло успiшно!\n";

	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	// спроба підключення до адреси до успіху
	SOCKET ConnectSocket = INVALID_SOCKET;

	for (addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) { // серверів може бути кілька, тому не завадить цикл

		// створення SOCKET для підключення до сервера
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (ConnectSocket == INVALID_SOCKET) {
			std::cout << "socket не вдалося створити з помилкою: " << WSAGetLastError() << "\n";
			WSACleanup();
			return 13;
		}

		// підключення до сервера
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}

		std::cout << "створення сокета на клiєнтi пройшло успiшно!\n";


		break;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		std::cout << "неможливо пiдключитися до сервера. перевiрте, чи запущено процес сервера!\n";
		WSACleanup();
		return 14;
	}
	else {
		std::cout << "пiдключення до сервера пройшло успiшно!\n";

	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	// отримання даних до закриття з'єднання зі сторони сервера
	char answer[BUFLEN];
	send(ConnectSocket, "Maze pls");


	//Получение размера лабиринта
	iResult = recv(ConnectSocket, answer, BUFLEN, 0);
	if (iResult == SOCKET_ERROR || iResult > BUFLEN)
	{
		safeCrash(ConnectSocket);
	}
	answer[iResult] = '\0';

	std::vector<string> answerInts = split(answer);


	int MAZE_HEIGHT = atoi(answerInts[1].c_str());
	int MAZE_WIDTH = atoi(answerInts[0].c_str());

	std::vector<vector<int>>maze = {};
	maze.resize(MAZE_HEIGHT, std::vector<int>(MAZE_WIDTH));

	//получение данных лабиринта

	send(ConnectSocket, "ok");
	iResult = recv(ConnectSocket, answer, BUFLEN, 0);
	if (iResult == SOCKET_ERROR || iResult > BUFLEN)
	{
		safeCrash(ConnectSocket);
	}
	answer[iResult] = '\0';

	std::cout << "\nGot the maze\n";

	system("cls");

	wchar_t ch = L'■';
	// показ лабиринта
	int mazeReadProgress = 0;
	for (int y = 0; y < MAZE_HEIGHT; y++) // перебор строк
	{
		for (int x = 0; x < MAZE_WIDTH; x++) // перебор столбцов
		{
			maze[y][x] = static_cast<int>(answer[mazeReadProgress] - '0'); //чтение ответа сервера с лабиринтом
			mazeReadProgress++;
			switch (maze[y][x])
			{
			case MazeObject::HALL: // hall - коридор
				std::cout << " ";
				break;

			case MazeObject::WALL: // wall - стена
				SetConsoleTextAttribute(h, Color::WHITE);
				std::cout << '#';
				break;
			}

		}
		std::cout << "\n";
	}

	//определение выхода
	int exitPos = 1;
	if (maze[1][MAZE_WIDTH - 1] == MazeObject::HALL)
	{
		exitPos = 0;
	}

	/*
	do {

		iResult = recv(ConnectSocket, answer, BUFLEN, 0);
		answer[iResult] = '\0';

		if (iResult > 0) {
			std::cout << "процес сервера надiслав вiдповiдь: " << answer << "\n";
			std::cout << "байтiв отримано: " << iResult << "\n";
		}
		else if (iResult == 0)
			std::cout << "з'єднання з сервером закрито.\n";
		else
			std::cout << "recv не вдалося з помилкою: " << WSAGetLastError() << "\n";

	} while (iResult > 0);
	*/
	/////////////////////////////////////////////////////////////////////

	send(ConnectSocket, "Coords pls");
	iResult = recv(ConnectSocket, answer, BUFLEN, 0);
	if (iResult == SOCKET_ERROR || iResult > BUFLEN)
	{
		safeCrash(ConnectSocket);
	}
	answer[iResult] = '\0';

	answerInts = split(answer);


	// размещение главного героя (ГГ)
	COORD position; // координаты нашего персонажа
	position.X = atoi(answerInts[0].c_str());
	position.Y = atoi(answerInts[1].c_str());
	SetConsoleCursorPosition(h, position);
	SetConsoleTextAttribute(h, Color::BLUE);
	std::cout << static_cast<char>(64);



	/////////////////////////////////////////////////////////////////////
	// информация по всем показателям
	int moves = 0; // счётчик шагов
	COORD infobox;
	infobox.X = MAZE_WIDTH + 1;
	infobox.Y = 1;

	SetConsoleCursorPosition(h, infobox);
	SetConsoleTextAttribute(h, Color::DARKYELLOW);
	std::cout << "MOVES: ";
	SetConsoleTextAttribute(h, Color::YELLOW);
	std::cout << moves << "\n";

	infobox.Y += 2;
	SetConsoleCursorPosition(h, infobox);
	SetConsoleTextAttribute(h, Color::WHITE);
	std::cout << "COORIDNATES: ";
	std::cout << "(" << position.X << "," << position.Y << ")\n";

	//////////перемещение
	while (true)
	{
		if (_kbhit()) // если было нажатие на клавиши пользователем
		{
			int code = _getch(); // get character, получение кода нажатой клавиши
			if (code == 224) { // если это стрелка
				code = _getch(); // получить конкретный код стрелки
			}

			// стирание персонажика в старой позиции
			SetConsoleCursorPosition(h, position);
			std::cout << " ";

			if (code == KeyCode::ENTER)
			{
				std::cout << "ENTER!\n";
				break;
			}
			else if (code == KeyCode::ESCAPE)
			{
				std::cout << "ESCAPE!\n";
				break;
			}
			else if (code == KeyCode::SPACE)
			{
				std::cout << "SPACE!\n";
				break;
			}
			else if (code == KeyCode::LEFT
				&& maze[position.Y][position.X - 1] != MazeObject::WALL)
			{
				if (tryMove(ConnectSocket, position, 'l'))
				{
					updateMoveCounter(moves, infobox, h, MAZE_WIDTH);
					updateCoords(position, infobox, h, MAZE_WIDTH);
				}
			}
			else if (code == KeyCode::RIGHT // если ГГ собрался пойти направо
				&& maze[position.Y][position.X + 1] != MazeObject::WALL)
				// и при этом в лабиринте на той же строке (где ГГ) и
				// немного (на одну ячейку) правее на 1 столбец от ГГ
			{
				if (tryMove(ConnectSocket, position, 'r'))
				{
					updateMoveCounter(moves, infobox, h, MAZE_WIDTH);
					updateCoords(position, infobox, h, MAZE_WIDTH);
				}
			}
			else if (code == KeyCode::UP
				&& maze[position.Y - 1][position.X] != MazeObject::WALL)
			{
				if (tryMove(ConnectSocket, position, 'u'))
				{
					updateMoveCounter(moves, infobox, h, MAZE_WIDTH);
					updateCoords(position, infobox, h, MAZE_WIDTH);
				}
			}
			else if (code == KeyCode::DOWN
				&& maze[position.Y + 1][position.X] != MazeObject::WALL)
			{

				if (tryMove(ConnectSocket, position, 'd'))
				{
					updateMoveCounter(moves, infobox, h, MAZE_WIDTH);
					updateCoords(position, infobox, h, MAZE_WIDTH);
				}
			}

			// показ ГГ в новой позиции
			SetConsoleCursorPosition(h, position);
			SetConsoleTextAttribute(h, Color::BLUE);
			std::cout << static_cast<char>(64);


			////////////////////////////////////////////////////////////////
			// пересечение с элементами массива

			if (exitPos == 0)
			{
				if (position.Y == 1 && position.X == MAZE_WIDTH - 1)
				{
					send(ConnectSocket, "Win pls");

					fd_set readfds;
					FD_ZERO(&readfds);
					FD_SET(ConnectSocket, &readfds);
					struct timeval timeout;
					timeout.tv_sec = 2; // Ожидание 2 секунды
					timeout.tv_usec = 0;

					int ready = select(ConnectSocket + 1, &readfds, NULL, NULL, &timeout); //по какой-то причине, только в этой части кода, recv не дожидается сервера и проваливается
					if (ready > 0) {
						iResult = recv(ConnectSocket, answer, BUFLEN, 0);
					}
					else {
						safeCrash(ConnectSocket);  // Если нет данных, аварийный выход
					}

					if (iResult == INVALID_SOCKET)
					{
						cout << "Error:" << WSAGetLastError() << endl; // ошибка сброса соединения?? но я не отключаю ещё сервер? так как он сейчас ещё готовится отправлять ок?
						safeCrash(ConnectSocket);
					}

					answer[iResult] = '\0';

					if (strcmp(answer, "ok") == 0)
					{
						std::string message = "you finded exit!\nMoves:";
						message.append(std::to_string(moves));
						MessageBoxA(0, message.c_str(), "CONGRATIONS!", 0);
						system("cls");
						break;
					}

				}
			}
			else
			{
				if (position.Y == MAZE_HEIGHT - 2 && position.X == MAZE_WIDTH - 1)
				{
					send(ConnectSocket, "Win pls");


					fd_set readfds;
					FD_ZERO(&readfds);
					FD_SET(ConnectSocket, &readfds);
					struct timeval timeout;
					timeout.tv_sec = 2; // Ожидание 2 секунды
					timeout.tv_usec = 0;

					int ready = select(ConnectSocket + 1, &readfds, NULL, NULL, &timeout); //по какой-то причине, только в этой части кода, recv не дожидается сервера и проваливается
					if (ready > 0) {
						iResult = recv(ConnectSocket, answer, BUFLEN, 0);
					}
					else {
						safeCrash(ConnectSocket);  // Если нет данных, аварийный выход
					}

					if (iResult == SOCKET_ERROR)
					{
						cout << "Error:" << WSAGetLastError() << endl; // ошибка сброса соединения?? но я не отключаю ещё сервер? так как он сейчас ещё готовится отправлять ок?
						safeCrash(ConnectSocket);
					}

					answer[iResult] = '\0';

					if (strcmp(answer, "ok") == 0)
					{
						std::string message = "you finded exit!\nMoves:";
						message.append(std::to_string(moves));
						MessageBoxA(0, message.c_str(), "CONGRATIONS!", 0);
						system("cls");
						break;
					}
				}
			}
		}
	}

	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		std::cout << "shutdown не вдалося з помилкою: " << WSAGetLastError() << "\n";
		closesocket(ConnectSocket);
		WSACleanup();
		return 16;
	}
	else {
		std::cout << "процес клiєнта iнiцiює закриття з'єднання з сервером.\n";
	}


	// очистка
	closesocket(ConnectSocket);
	WSACleanup();

	std::cout << "процес клiєнта завершує свою роботу!\n";

	return 0;

}
