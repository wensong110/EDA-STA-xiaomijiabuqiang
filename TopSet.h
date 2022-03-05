#pragma once
#include <set>
#include <mutex>
template<typename T>
struct TopSet
{
	public:
	multiset<T> container;
	int size;
	mutex _mutex;
	TopSet(int size = 20) {
		this->size = size;
	}
	void insert(T elem) {
		_mutex.lock();
		container.insert(elem);
		if (container.size()>size) {
			auto it=container.end();
			it--;
			container.erase(it);
		}
		_mutex.unlock();
	}
};