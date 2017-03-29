#include "plog/Log.h"
#include "plog/Appenders/ColorConsoleAppender.h"
#include <concurrent_queue.h>
#include <libpq-fe.h>
#include <memory>
#include <ppl.h>
#include <thread>
#include <vector>
using namespace std;

class PostgresqlConnection
{
	PGconn* m_conn;
	mutex m_mtx;
	int m_counter;

public:
	PostgresqlConnection() = delete;

	explicit PostgresqlConnection(const string& conn_string)
		: m_counter(0)
	{
		m_conn = PQconnectdb(conn_string.c_str());
		if (m_conn == nullptr) {
			throw "PQconnectdb失敗";
		}

		auto conn_status = PQstatus(m_conn);
		if (conn_status != CONNECTION_OK) {
			throw PQerrorMessage(m_conn);
		}
	}

	~PostgresqlConnection()
	{
		LOGI << "counter=" << m_counter;

		if (m_conn == nullptr) {
			return;
		}

		auto conn_status = PQstatus(m_conn);
		if (conn_status != CONNECTION_OK) {
			return;
		}

		PQfinish(m_conn);
	}

	vector<string> Select(const string& query_string)
	{
		{
			lock_guard<mutex> lock(m_mtx);
			++m_counter;
		}
		auto exec_result = PQexec(m_conn, query_string.c_str());
		if (exec_result == nullptr) {
			throw PQerrorMessage(m_conn);
		}

		auto status = PQresultStatus(exec_result);
		if (status != PGRES_TUPLES_OK) {
			throw PQresultErrorMessage(exec_result);
		}

		auto result = parse(exec_result);

		PQclear(exec_result);

		return result;
	}

private:
	vector<string> parse(PGresult* exec_result)
	{
		const auto n_fields = PQnfields(exec_result);
		const auto n_tuples = PQntuples(exec_result);

		vector<string> result;
		for (int i = 0; i < n_tuples; ++i) {
			for (int j = 0; j < n_fields; ++j) {
				const auto value = PQgetvalue(exec_result, i, j);

				result.emplace_back(value);
			} // 列
		} // 行
		return result;
	}
};

using resource_type = shared_ptr<PostgresqlConnection>;
using resources_type = Concurrency::concurrent_queue<resource_type>;

void process(
	resources_type& resources,
	int i)
{
	// キューからリソースを取り出す
	resource_type resource;
	while (true) {
		auto success = resources.try_pop(resource);
		if (success) {
			break;
		}
		LOGE << "empty";
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// クエリ実行
	string query_string;
	{
		ostringstream oss;
		oss << "SELECT version()";

		query_string = oss.str();
	}
	const auto result = resource->Select(query_string);

	{
		ostringstream oss;
		bool first = true;
		for (const auto& column : result) {
			if (first) {
				first = false;
			}
			else {
				oss << " ";
			}
			oss << column;
		}
		LOGI << oss.str();
	}

	// キューへリソースを戻す
	resources.push(resource);
}

int main(int argc, char *argv[])
{
	plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
	plog::init(plog::info, &consoleAppender);

	try {
		const auto num_cpus = std::thread::hardware_concurrency();

		Concurrency::SchedulerPolicy policy(2,
			Concurrency::MinConcurrency, 1,
			Concurrency::MaxConcurrency, num_cpus - 1);
		Concurrency::CurrentScheduler::Create(policy);

		// 上記により同時処理数を制限していても超過することがあるので
		// コア数*2のリソースを作っておく
		resources_type resources;
		for (size_t i = 0; i < num_cpus * 2; ++i) {
			auto resource = make_shared<PostgresqlConnection>("dbname=test user=postgres password=postgres");
			resources.push(resource);
		}

		std::vector<int> numbers;
		for (int i = 0; i < 100; ++i) {
			numbers.push_back(i);
		}

		// 並列処理
		Concurrency::parallel_for_each(
			numbers.begin(),
			numbers.end(),
			[&](int i) {
				process(resources, i);
			}
		);
	}
	catch (const string& e) {
		LOGE << e;
		return -1;
	}
	catch (const char* e) {
		LOGE << e;
		return -2;
	}
	catch (...) {
		LOGE << "例外発生";
		return -3;
	}
}
