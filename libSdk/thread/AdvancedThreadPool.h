#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>
#include <atomic>
#include <memory>
#include <map>

class AdvancedThreadPool {
public:
    // 任务优先级类型
    enum class Priority { High, Normal, Low };
    
    // 构造函数，设置初始和最大线程数
    AdvancedThreadPool(size_t init_threads, size_t max_threads);
    
    // 提交任务（带优先级）
    template<class F, class... Args>
    auto enqueue(Priority priority, F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;
        
        if (stop.load())
            throw std::runtime_error("enqueue on stopped ThreadPool");
            
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            
        std::future<return_type> res = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            // 根据优先级插入任务
            auto insert_pos = priority == Priority::High ? tasks.begin() :
                             priority == Priority::Low ? tasks.end() :
                             tasks.begin() + tasks.size() / 2;
            
            tasks.insert(insert_pos, [task](){ (*task)(); });
            
            // 动态调整线程数量
            if (idle_threads < tasks.size() && workers.size() < max_threads) {
                add_threads(1);
            }
        }
        
        condition.notify_one();
        return res;
    }
    
    // 取消所有未开始的任务
    void cancel_pending() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.clear();
    }
    
    // 析构函数
    ~AdvancedThreadPool();

private:
    // 添加指定数量的工作线程
    void add_threads(size_t n);

private:
    std::vector<std::thread> workers;
    std::deque<std::function<void()>> tasks; // 改用deque支持中间插入
    
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    
    size_t max_threads;
    std::atomic<size_t> idle_threads;
    
    // 线程局部数据支持
    std::function<void*()> thread_data_initializer;
    std::mutex data_mutex;
    std::map<std::thread::id, void*> all_thread_data;
};
