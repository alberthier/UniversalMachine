#include <exception>
#include <iostream>
#include "UniversalMachine.h"

using namespace std;

void printDesc()
{
	cout << "Thier's Universal Machine v" TUM_VERSION << endl;
}

void printUsage()
{
	printDesc();
	cout << "Usage :" << endl;
	cout << "        tum UniversalBinary" << endl;
}

int main(int argc, char** argv)
{
	UniversalMachine um;
	try {
		if (argc == 2) {
			um.run(argv[1]);
		} else {
			printUsage();
		}
	} catch (exception &e) {
		printDesc();
		cout << "The following error occured :" << endl << endl;
		cout << e.what() << endl;
	}
}
