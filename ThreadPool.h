#pragma once
#include<future>
#include<functional>
#include<atomic>
#include<thread>
#include<queue>

class ThreadPool
{
public:
	ThreadPool(int thread_num) {
		this->running.store(true);
		for (int i = 0; i < thread_num; i++) {
			pool.emplace_back([this]() {
				while (this->running) {
					std::function<void(void)> task;
					{
						std::unique_lock<std::mutex> lock(this->lock);
						while (this->running &&this->task_queue.empty())
							cv.wait(lock);
						if (!this->running.load() && this->task_queue.empty())
							return;
						task = std::move(task_queue.front());
						task_queue.pop();
					}
					task();
				}//running loop
			});
		}
	}

	template <class F, class... Args>
	auto commit(F&& f, Args &&... args) -> std::future<decltype(f(args...))> {
		// commit a task, return std::future
		// example: .commit(std::bind(&Dog::sayHello, &dog));

		if (!this->running.load())
			throw std::runtime_error("commit on ThreadPool is stopped.");

		// declare return type
		using return_type = decltype(f(args...));

		// make a shared ptr for packaged_task
		// packaged_task package the bind function and future
		auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		{
			std::lock_guard<std::mutex> lock(this->lock);
			task_queue.emplace([task_ptr]() { (*task_ptr)(); });
		}

		// wake a thread
		this->cv.notify_one();
		return task_ptr->get_future();
	}
	~ThreadPool() {
		this->running.store(false);
		cv.notify_all();
		for (auto& thread : pool)
			thread.join();
	}
private:
	std::vector<std::thread> pool;
	std::mutex lock;
	std::atomic<bool> running;
	std::queue<std::function<void(void)>> task_queue;
	std::condition_variable cv;
};

