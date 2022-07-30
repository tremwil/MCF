// Test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <vector>

std::vector<char> input_buf(1024);

void log(const char* text)
{
	std::cout << text << std::endl;

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO bInfo;
	GetConsoleScreenBufferInfo(hOut, &bInfo);

	auto windowBefore = bInfo.srWindow;

	std::vector<char> separator(bInfo.dwSize.X-1, '=');
	separator.push_back('\0');

	SetConsoleCursorPosition(hOut, { 0, (SHORT)(bInfo.srWindow.Bottom - 1) });
	std::cout << separator.data();
	SetConsoleCursorPosition(hOut, bInfo.dwCursorPosition);

	GetConsoleScreenBufferInfo(hOut, &bInfo);

	if (bInfo.srWindow.Bottom - bInfo.dwCursorPosition.Y < 3)
	{
		bInfo.srWindow.Top += bInfo.dwCursorPosition.Y - bInfo.srWindow.Bottom + 3;
		bInfo.srWindow.Bottom += bInfo.dwCursorPosition.Y - bInfo.srWindow.Bottom + 3;
		SetConsoleWindowInfo(hOut, true, &bInfo.srWindow);
	}
}

void main() {
	for (int i = 0; i < 100; i++) {
		log("TEST");
		Sleep(500);
	}
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
