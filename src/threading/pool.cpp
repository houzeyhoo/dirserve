
#include "pool.hpp"

namespace threading
{

pool::pool(size_t size, close_policy on_close) : on_close(on_close)
{
    for (size_t i = 0; i < size; i++)
        threads.emplace_back(&pool::thread_loop, this);
}

pool::~pool()
{
    {
        std::unique_lock lock(mutex);
        closed = true;
    }
    state_change.notify_all();
    for (auto& t : threads)
        t.join();
}

void pool::thread_loop()
{
    while (true)
    {
        task task;
        {
            std::unique_lock lock(mutex);
            state_change.wait(lock, [this] { return closed || !tasks.empty(); });
            if (closed && ((on_close != close_policy::finish_tasks) || tasks.empty()))
                return;
            if (tasks.empty())
                continue;
            task = fetch_one_task();
        }
        if (task)
            task();
    }
}

task fifo_pool::fetch_one_task()
{
    auto& task_queue = get_task_queue();
    task tmp = task_queue.front();
    task_queue.pop_front();
    return tmp;
}

void fifo_pool::enqueue_one_task(task task)
{
    auto& task_queue = get_task_queue();
    task_queue.push_back(std::move(task));
};

} // namespace threading
