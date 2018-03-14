#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>
#include <list>
#include <queue>

class Runnable
{
public:
	virtual void run() = 0;
};

class ThreadPool
{
private:
	std::list<std::thread> pool;
	std::queue<std::unique_ptr<Runnable>> tasks;

	std::mutex globalLock;
	std::condition_variable monitor;

	bool active;
	unsigned int poolSize;
public:
	ThreadPool();
	~ThreadPool();

	unsigned int getPoolSize() { return poolSize; }
	bool isActive() { return active; }
	void init();
	void addTask(std::unique_ptr<Runnable> task);
	void shutDown();
	void pollTask();
};