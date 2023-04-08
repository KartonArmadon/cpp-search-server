#include <iostream>
#include <string>
using namespace std;

/// <summary>
/// Считает количество чисел хотя бы с одной тройкой в диапазоне [a,b]. Границы включены.
/// </summary>
/// <param name="">Левая граница диапазона</param>
/// <param name="">Правая граница диапазона</param>
/// <returns>Количество чисел хотя бы с одной тройкой</returns>
int CountThrees(int, int);

int CountThrees1(int, int);

int main() {
	if (CountThrees(1, 1000) == 271)
		cout << "Test passed :)" << endl;
	
	if (CountThrees1(1, 1000) == 271)
		cout << "Test passed :)" << endl;
}

int CountThrees(int a, int b) {
	int result = 0;
	int temp;
	for (int i = a; i <= b; ++i) {
		temp = i;
		while (temp != 0)
		{
			if (temp % 10 == 3){
				cout << result << " "s << i << endl;
				++result;
				break;
			}
			temp /= 10;
		}
	}
	return result;
}

int CountThrees1(int a, int b) {
	int result = 0;
	for (int i = a; i <= b; ++i) {
		string temp = to_string(i);
		if (temp.find("3") != string::npos)
			++result;
	}
	return result;
}