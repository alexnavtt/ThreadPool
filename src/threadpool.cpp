#include <chrono>
#include "threadpool/threadpool.hpp"

using namespace std::chrono_literals;

// Infinite thread loop
void ThreadPool::waitForTask_(size_t idx){
    active_indices_[idx] = false;
    std::unique_lock<std::mutex> task_lock(task_mtx_, std::defer_lock);
    std::unique_lock<std::mutex> state_lock(state_mtx_, std::defer_lock);

    while (true){
        task_lock.lock();

        // Retrieve the next task or signal to finish
        task_condition_.wait(task_lock, [this](){return destruct_ || !task_queue_.empty();});
        
        // If we have received a signal to finish, end the loop
        if (destruct_) return;
        
        // Retreive the next task
        auto task = task_queue_.front();
        task_queue_.pop_front();
        task_lock.unlock();

        // Mark the thread as occupied, and the task as active
        state_lock.lock();
        active_indices_[idx] = true;  
        task_queued_.erase(task.idx);
        task_active_.insert(task.idx);     
        state_lock.unlock();

        // Perform the task
        task.task();

        // Signal that a task has just finished
        state_lock.lock();
        task_active_.erase(task.idx);
        active_indices_[idx] = false;        
        state_lock.unlock();
        finish_condition_.notify_one();
    }
}

// ================================================================================================
// ================================================================================================

ThreadPool::ThreadPool(int num_threads) : pool_(num_threads), active_indices_(num_threads, true) {
    // Create all of the threads
    for (int idx = 0; idx < num_threads; idx++){
        pool_[idx] = std::thread(&ThreadPool::waitForTask_, this, idx);
    }
    
    // Block until all the threads have been created and started their vigil
    while (anyTrue(active_indices_));
}

// ================================================================================================
// ================================================================================================

ThreadPool::~ThreadPool(){
    destroyPool_();
}

// ================================================================================================
// ================================================================================================

int ThreadPool::getNumThreads() const{
    return pool_.size();
}

// ================================================================================================
// ================================================================================================

void ThreadPool::setNumThreads(int num_threads){
    // Make sure all tasks finish
    joinAll();
    
    // Destroy all current threads
    destroyPool_();

    // Resize the threadpool to the new size
    pool_.resize(num_threads);
    active_indices_.resize(num_threads);

    // Make sure no threads start any tasks until we finish creating them
    std::lock_guard<std::mutex> task_lock(task_mtx_);

    // Set the new threads to work on the task loop
    for (int i = 0; i < num_threads; i++){
        active_indices_[i] = true;
        pool_[i] = std::thread(&ThreadPool::waitForTask_, this, i);
    }

    // Wait for all threads to start
    while (anyTrue(active_indices_));
}

// ================================================================================================
// ================================================================================================

size_t ThreadPool::queueTask(const std::function<void(void)>& fcn){
    {
        Task new_task{fcn, ++task_idx_};
        std::lock_guard<std::mutex> lock(task_mtx_);
        task_queue_.push_back(new_task);

        std::lock_guard<std::mutex> state_lock(state_mtx_);
        task_queued_.insert(task_idx_);
    }
    task_condition_.notify_one();
    return task_idx_;
}

// ================================================================================================
// ================================================================================================

bool ThreadPool::poolIsIdle(){
    std::lock_guard<std::mutex> lock(task_mtx_);
    return not anyTrue(active_indices_) && task_queue_.empty();
}

// ================================================================================================
// ================================================================================================

void ThreadPool::joinAll(){
    // Wait for all the threads to finish their current task
    std::unique_lock<std::mutex> lock(state_mtx_);
    finish_condition_.wait(lock, [this](){return poolIsIdle();});
}

// ================================================================================================
// ================================================================================================

void ThreadPool::joinOne(size_t idx){
    // Wiat for the thread in question to finish its task
    std::unique_lock<std::mutex> lock(state_mtx_);
    finish_condition_.wait(lock, [this, idx](){return not (task_queued_.count(idx) || task_active_.count(idx));});
}

// ================================================================================================
// ================================================================================================

void ThreadPool::destroyPool_(){
    // Clear any upcoming tasks, and signal the threads to join once they finish the current task
    std::unique_lock<std::mutex> lock(task_mtx_);
    task_queue_.clear();
    destruct_ = true;
    lock.unlock();
    task_condition_.notify_all();

    // Wait for all the threads to finish
    for (std::thread& t : pool_){
        t.join();
    }

    // Reset destruct flag
    destruct_ = false;
}

// ================================================================================================
// ================================================================================================

void ThreadPool::startTaskGroup(){
    if (task_group_started_){
        throw std::runtime_error("Cannot start a task group while another is already active!");
    }
    task_group_started_ = true;
    task_group_idx_ = task_idx_ + 1;
}

// ================================================================================================
// ================================================================================================

void ThreadPool::endTaskGroup(){
    if (not task_group_started_) return;
    task_group_started_ = false;
    for (size_t idx = task_group_idx_; idx < task_idx_; idx++){
        joinOne(idx);
    }
}

// ================================================================================================
// ================================================================================================

bool ThreadPool::allTrue(const std::vector<bool>& v){
    bool all_true = true;
    for (const auto& b : v){
        all_true = all_true && b;
    }
    return all_true;
}

// --------------------------------------------------------

bool ThreadPool::anyTrue(const std::vector<bool>& v){
    bool any_true = false;
    for (const auto& b : v){
        any_true = any_true || b;
    }
    return any_true;
}

// --------------------------------------------------------
