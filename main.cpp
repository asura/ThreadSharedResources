#include "plog/Log.h"
#include <iostream>
#include <ppl.h>
#include <concurrent_queue.h>
#include <thread>
#include <vector>
using namespace std;

int main(int argc, char *argv[])
{
	plog::init(plog::info, "ThreadSharedResources.log");

	LOGI << "START";

	const auto num_cpus = std::thread::hardware_concurrency();

	Concurrency::concurrent_queue<int> resources;
	for (int i = 0; i < num_cpus; ++i) {
		resources.push(i);
	}

	std::vector<int> numbers;
	for (int i = 0; i < 100; ++i) {
		numbers.push_back(i);
	}
	Concurrency::parallel_for_each(
		numbers.begin(),
		numbers.end(),
		[&](int i) {
			int resource;
			while (true) {
				auto success = resources.try_pop(resource);
				if (success) {
					break;
				}
				LOGE << "empty";
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			LOGI << i << "," << resource;

			resources.push(resource);
		});

	LOGI << "END";

	char ch;
	cin >> ch;
}