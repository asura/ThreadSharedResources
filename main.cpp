#include "plog/Log.h"
#include <iostream>
#include <thread>
using namespace std;

int main()
{
	plog::init(plog::info, "ThreadSharedResources.log");

	LOGI << "START";
	LOGI << "END";

	char ch;
	cin >> ch;
}