
#include "Threadpool.h"

#include <iostream>

ThreadPool::ThreadPool()
{
	init();
	std::cout << "ThreadPool: Using " << poolSize << " thread(s)" << std::endl;
}

void ThreadPool::init()
{
	active = true;
	poolSize = std::thread::hardware_concurrency();
	poolSize = poolSize < 1 ? 1 : poolSize;
	for (unsigned i = 0; i < poolSize; i++)
	{
		std::thread t(&ThreadPool::pollTask, this);
		pool.push_back(std::move(t));
	}
}

ThreadPool::~ThreadPool()
{
	shutDown();
}

void ThreadPool::shutDown()
{
	active = false;
	std::unique_lock<std::mutex> lock(globalLock);
	monitor.notify_all();
	lock.unlock();
	for (auto & thread : pool)
	{
		thread.join();
	}
}

void ThreadPool::addTask(std::unique_ptr<Runnable> task)
{
	std::unique_lock<std::mutex> lock(globalLock);
	tasks.push(std::move(task));
	lock.unlock();
	monitor.notify_one();
}

void ThreadPool::pollTask()
{
	while (active)
	{
		std::unique_lock<std::mutex> lock(globalLock);
		while (tasks.empty() && active)
		{
			monitor.wait(lock);
		}

		if (!tasks.empty())
		{
			std::unique_ptr<Runnable> task = std::move(tasks.front());
			tasks.pop();
			lock.unlock();

			task->run();
		}
		else
		{
			lock.unlock();
		}
	}
}