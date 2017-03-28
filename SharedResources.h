#pragma once

#include <deque>
#include <mutex>
#include "plog/Log.h"

template<typename T>
class SharedResources
{
	std::deque<T> m_resources;
	std::mutex m_resources_mutex;

public:
	SharedResources() = default;
	~SharedResources() = default;

	void Push(T& new_resource)
	{
		std::lock_guard<std::mutex> lock(m_resources_mutex);
		m_resources.push_back(new_resource);
	}

	T Pop()
	{
		while (true) {
			{
				std::lock_guard<std::mutex> lock(m_resources_mutex);
				if (!m_resources.empty()) {
					T item = m_resources.front();
					m_resources.pop_front();
					return item;
				}
				LOGE << "empty";
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	size_t size() const
	{
		std::lock_guard<std::mutex> lock(m_resources_mutex);
		return m_resources.size();
	}
};
