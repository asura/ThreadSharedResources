#include "plog/Log.h"
#include "plog/Appenders/ColorConsoleAppender.h"
#include <iostream>
#include <ppl.h>
#include <set>
#include <thread>
#include <vector>
using namespace std;

void usage(const char* const app_name)
{
	LOGE << "引数不正" << endl
		<< app_name << " 期待する最大スレッド数(1～)";
}

void process(int i, mutex& mtx, set<thread::id>& thread_ids)
{
	if (i == 0) {
		LOGI
			<< "MaxConcurrency="
			<< Concurrency::CurrentScheduler::GetPolicy().GetPolicyValue(Concurrency::MaxConcurrency);
	}
	{
		lock_guard<mutex> log(mtx);
		thread_ids.insert(this_thread::get_id());
	}
	this_thread::sleep_for(chrono::milliseconds(10));
}

int main(int argc, char *argv[])
{
	plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
	plog::init(plog::info, &consoleAppender);

	if (argc != 2) {
		usage(argv[0]);
		return -1;
	}

	const int expected = atoi(argv[1]);

	if (expected == 0) {
		usage(argv[0]);
		return -2;
	}

	const auto num_cpus = std::thread::hardware_concurrency();

	LOGI
		<< "MaxConcurrency="
		<< Concurrency::CurrentScheduler::GetPolicy().GetPolicyValue(Concurrency::MaxConcurrency)
		<< ",MaxExecutionResources="
		<< Concurrency::MaxExecutionResources
		<< ",num_cpus="
		<< num_cpus;

	Concurrency::SchedulerPolicy policy(2,
		Concurrency::MinConcurrency, 1,
		Concurrency::MaxConcurrency, expected);
	Concurrency::CurrentScheduler::Create(policy);

	std::vector<int> numbers;
	for (int i = 0; i < 100; ++i) {
		numbers.push_back(i);
	}

	mutex mtx;
	set<thread::id> thread_ids;

	Concurrency::parallel_for_each(
		numbers.begin(),
		numbers.end(),
		[&](int i) {
			process(i, mtx, thread_ids);
		}
	);

	LOGI << "thread_ids num:" << thread_ids.size();
	for (const auto& thread_id : thread_ids) {
		LOGI << thread_id;
	}

	Concurrency::CurrentScheduler::Detach();
}

// 参考サイト:
//   https://blogs.msdn.microsoft.com/nativeconcurrency/2009/11/18/concurencyparallel_for-and-concurrencyparallel_for_each/
//
// ここのコメント欄にあるような「自身+指定数のスレッド」とはならないことがある。
// (実行するごとに変化することがある)
//
// <VS2017@ThinkPad T450(4コア)での実験結果>
// 指定数 : ユニークスレッドID数
//      1 : 2
//      2 : 3
//      3 : 4～5
//      4 : 6
//      5 : 7～8
//      6 : 8～9
//      7 : 9～11
