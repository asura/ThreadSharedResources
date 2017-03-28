#include "SharedResources.h"
#include "plog/Log.h"
#include <iostream>
#include <ppl.h>
#include <thread>
#include <vector>
using namespace std;

int main(int argc, char *argv[])
{
	plog::init(plog::info, "ThreadSharedResources.log");

	LOGI << "START";

	const auto num_cpus = std::thread::hardware_concurrency();

	SharedResources<int> resources;
	for (int i = 0; i < num_cpus; ++i) {
		resources.Push(i);
	}

	std::vector<int> tasks;
	for (int i = 0; i < 100; ++i) {
		tasks.push_back(i);
	}
	Concurrency::parallel_for_each(
		tasks.begin(),
		tasks.end(),
		[&](int i) {
			auto resource = resources.Pop();
			LOGI << i << "," << resource;

			resources.Push(resource);
		});

	LOGI << "END";

	char ch;
	cin >> ch;
}