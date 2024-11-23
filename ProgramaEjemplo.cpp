#include <iostream>
using namespace std;

void hanoi(int num, char origen, char destino, char auxiliar)
{
    if (num == 1)
    {
        cout << "Mueva el bloque " << num << " desde " << origen << " hasta " << destino << endl;
    }
    else
    {
        hanoi(num - 1, origen, auxiliar, destino);
        cout << "Mueva el bloque " << num << " desde " << origen << " hasta " << destino << endl;
        hanoi(num - 1, auxiliar, destino, origen);
    }
}

int main()
{
    int n;
    char A, B, C;

    cout << "Las clavijas son A B C\n";
    cout << "Número de discos: ";
    cin >> n;

    hanoi(n, 'A', 'C', 'B');

    return 0;
}