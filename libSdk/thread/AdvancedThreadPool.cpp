#include "AdvancedThreadPool.h"
AdvancedThreadPool::AdvancedThreadPool(size_t init_threads, size_t max_threads)
: max_threads(max_threads), stop(false), idle_threads(0) {
    if (init_threads < 1) init_threads = 1;
    if (max_threads < init_threads) max_threads = init_threads;

    add_threads(init_threads);
}

AdvancedThreadPool::~AdvancedThreadPool() {
    stop.store(true);
    condition.notify_all();
    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }

    // 清理线程局部数据
    std::unique_lock<std::mutex> lock(data_mutex);
    for (auto& data : all_thread_data) {
        if (data.second) {
            delete static_cast<char*>(data.second);
            data.second = nullptr;
        }
    }
}

void AdvancedThreadPool::add_threads(size_t n)
{
    for (size_t i = 0; i < n && workers.size() < max_threads; ++i) 
    {
        workers.emplace_back([this] {               
            for (;;) 
            {
                std::function<void()> task;                   
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    ++idle_threads;
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });
                    --idle_threads;
                        
                    if (this->stop && this->tasks.empty()) 
                        break;    
                        
                    if (!this->tasks.empty()) 
                    {
                        task = std::move(this->tasks.front());
                        this->tasks.pop_front();
                    }
                }
                    
                if (task) 
                    task();
            }
                
            // 清理线程局部数据
            std::unique_lock<std::mutex> lock(data_mutex);
            auto it = all_thread_data.find(std::this_thread::get_id());
            if (it != all_thread_data.end()) 
            {
                delete static_cast<char*>(it->second);
                all_thread_data.erase(it);
            }
        });
    }
}