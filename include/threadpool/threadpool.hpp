#ifndef THREADPOOL_HPP_
#define THREADPOOL_HPP_

#include <unordered_set>
#include <list>
#include <mutex>
#include <vector>
#include <thread>
#include <functional>
#include <condition_variable>

class ThreadPool{
public:
    ThreadPool(int num_threads = std::max(std::thread::hardware_concurrency(), 1U));
    ~ThreadPool();

    int getNumThreads() const;
    void setNumThreads(int num_threads); 

    size_t queueTask(const std::function<void(void)>& fcn);

    bool poolIsIdle();

    void joinAll();
    void joinOne(size_t idx);

    void startTaskGroup();
    void endTaskGroup();

private:

    struct Task{
        std::function<void(void)> task;
        size_t idx;
    };

    bool destruct_ = false;
    std::vector<std::thread> pool_;
    std::vector<bool> active_indices_;

    std::mutex task_mtx_;
    std::list<Task> task_queue_;
    std::condition_variable task_condition_;

    std::mutex state_mtx_;
    std::condition_variable finish_condition_;

    std::unordered_set<size_t> task_queued_;
    std::unordered_set<size_t> task_active_;

    size_t task_idx_ = 0;
    size_t task_group_idx_ = 0;
    bool task_group_started_ = false;

    void waitForTask_(size_t i);
    void destroyPool_();
    static bool allTrue(const std::vector<bool>& v);
    static bool anyTrue(const std::vector<bool>& v);
};

#endif
