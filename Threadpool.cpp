
#include "Threadpool.h"

ThreadPool::ThreadPool()
{
	unsigned maxThreads = std::thread::hardware_concurrency();
	maxThreads = maxThreads < 1 ? 1 : maxThreads;

	for (unsigned i = 0; i < maxThreads; i++)
	{
		std::thread t(&ThreadPool::pollTask, *this);
		pool.push_back(t);
	}
}

ThreadPool::~ThreadPool()
{
	shutDown();
}

void ThreadPool::shutDown()
{

}

void ThreadPool::addTask(Runnable * task)
{
	std::unique_lock<std::mutex> lock(globalLock);
	tasks.push(task);
	monitor.notify_one();
}

void ThreadPool::pollTask()
{
	std::unique_lock<std::mutex> lock(globalLock);
	while (tasks.empty())
	{
		monitor.wait(lock);
	}

	Runnable * task = tasks.front();
	tasks.pop();

	task->run();

	delete task;
}