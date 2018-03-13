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
	std::queue<Runnable *> tasks;

	std::mutex globalLock;
	std::condition_variable monitor;

public:
	ThreadPool();
	~ThreadPool();

	void addTask(Runnable * runnable);
	void shutDown();

private:
	void pollTask();
	
};