#pragma once
#include "../common/Common.h"
#include <functional>
#include <future>

namespace DistributedChat {
    class ThreadPool {
    private:
        std::vector<std::thread> m_workers;
        std::queue<std::function<void()>> m_tasks;
        std::mutex m_queueMutex;
        std::condition_variable m_condition;
        bool m_stop;

    public:
        explicit ThreadPool(size_t threads = 4);
        ~ThreadPool();

        template<class F, class... Args>
        auto Enqueue(F&& f, Args&&... args) 
            -> std::future<typename std::invoke_result<F, Args...>::type> {
            using return_type = typename std::invoke_result<F, Args...>::type;

            auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
            
            std::future<return_type> res = task->get_future();
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                if (m_stop) {
                    throw std::runtime_error("Enqueue on stopped ThreadPool");
                }
                m_tasks.emplace([task]() { (*task)(); });
            }
            m_condition.notify_one();
            return res;
        }
    };
}