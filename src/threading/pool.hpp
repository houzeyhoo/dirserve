
#pragma once
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>

namespace threading
{

using task = std::function<void()>;

class pool
{
  public:
    enum class close_policy
    {
        finish_tasks,
        discard_tasks,
    };

    explicit pool(size_t size = std::thread::hardware_concurrency(),
                  close_policy on_close = close_policy::finish_tasks);

    virtual ~pool();

    pool(const pool&) = delete;
    pool& operator=(const pool&) = delete;
    pool(pool&& other) noexcept = delete;
    pool& operator=(pool&& other) noexcept = delete;

    template <typename _f, typename... _args> auto enqueue(_f&& f, _args&&... args);

  protected:
    virtual task fetch_one_task() = 0;

    virtual void enqueue_one_task(task task) = 0;

    std::deque<task>& get_task_queue()
    {
        return tasks;
    }

  private:
    const close_policy on_close;

    std::vector<std::thread> threads;

    std::deque<task> tasks;

    std::condition_variable state_change;

    std::mutex mutex;

    bool closed = false;

    void thread_loop();
};

class fifo_pool : public pool
{
  public:
    using pool::pool;

  protected:
    task fetch_one_task() override;

    void enqueue_one_task(task task) override;
};

template <typename _f, typename... _args> auto pool::enqueue(_f&& f, _args&&... args)
{
    using return_type = std::invoke_result<_f, _args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<_f>(f), std::forward<_args>(args)...));
    auto future = task->get_future();

    {
        std::unique_lock lock(mutex);
        if (closed)
        {
            throw std::runtime_error("Attempted to enqueue task on a closed pool");
        }
        enqueue_one_task([task] { (*task)(); });
    }

    state_change.notify_one();
    return future;
}

} // namespace threading
