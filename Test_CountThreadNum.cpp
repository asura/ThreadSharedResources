#include "plog/Log.h"
#include "plog/Appenders/ColorConsoleAppender.h"
#include <iostream>
#include <ppl.h>
#include <set>
#include <thread>
#include <vector>
using namespace std;

class Counter
{
	int m_counter;
	int m_highest;
	mutable mutex m_mtx;	// const���\�b�h�p��mutable��������

public:
	Counter()
		: m_counter(0)
		, m_highest(0)
	{
	}

	~Counter() = default;

	void inc()
	{
		lock_guard<mutex> lock(m_mtx);
		++m_counter;
		if (m_highest < m_counter) {
			m_highest = m_counter;
		}
	}

	void dec()
	{
		lock_guard<mutex> lock(m_mtx);
		--m_counter;
	}

	int value() const
	{
		lock_guard<mutex> lock(m_mtx);
		return m_counter;
	}

	int highest() const
	{
		lock_guard<mutex> lock(m_mtx);
		return m_highest;
	}
};

void usage(const char* const app_name)
{
	LOGE << "�����s��" << endl
		<< app_name << " ���҂���ő�X���b�h��(1�`)";
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
	Counter counter;

	Concurrency::parallel_for_each(
		numbers.begin(),
		numbers.end(),
		[&](int i) {
			counter.inc();
			process(i, mtx, thread_ids);
			counter.dec();
		}
	);

	LOGI << "thread_ids num:" << thread_ids.size();
	for (const auto& thread_id : thread_ids) {
		LOGI << thread_id;
	}

	LOGI << "highest concurrent counter:" << counter.highest();

	Concurrency::CurrentScheduler::Detach();
}

// �Q�l�T�C�g:
//   https://blogs.msdn.microsoft.com/nativeconcurrency/2009/11/18/concurencyparallel_for-and-concurrencyparallel_for_each/
//
// �X���b�hID���ϑ�����Ɓu���g+�w�萔�̃X���b�h�v�Ƃ͂Ȃ�Ȃ����Ƃ�����B
// (���s���邲�Ƃɕω����邱�Ƃ�����)
//
// �����ł͂Ȃ��������s�����J�E���^�ɂ��ϑ����Ă݂��
// �w�萔+1�ƂȂ��Ă����B
//
// <VS2017@ThinkPad T450(4�R�A)�ł̎�������>
// �w�萔 : ���j�[�N�X���b�hID�� : �ő哯�����s��
//      1 : 2                    : 2
//      2 : 3                    : 3
//      3 : 4�`5                 : 4
//      4 : 6                    : 5
//      5 : 7�`8                 : 6
//      6 : 8�`10                : 7
//      7 : 9�`11                : 8
