#include<iostream>
#include<sstream>
#include<string>
#include<cstring>
#include"server.h"
#include"htmlserver.h"
#include"util.h"
using namespace std;


int main(int ac, char** av)
{
	Dndd f;
	int port = ac < 2 ? 2001 : atoi(av[1]);
	Server sv{port};
	sv.start(f);
}
