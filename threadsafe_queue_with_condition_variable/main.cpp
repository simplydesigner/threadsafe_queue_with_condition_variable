#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

template <typename T>
class threadsafe_queue
{
public:
    threadsafe_queue() {}
    threadsafe_queue(threadsafe_queue const & other)
    {
        std::lock_guard<std::mutex> lock { other.m_mutex };
        m_queue = other.m_queue;
    }
    auto operator =(threadsafe_queue const &) -> threadsafe_queue & = delete;
    
    auto is_empty() const -> bool
    {
        std::lock_guard<std::mutex> lock { m_mutex };
        return m_queue.empty();
    }
    
    auto wait_and_pop() -> std::shared_ptr<T>
    {
        std::unique_lock<std::mutex> lock { m_mutex };
        m_condition_variable.wait(lock, [this] {
            return !m_queue.empty();
        });
        
        std::shared_ptr<T> rv = std::make_shared<T>(m_queue.front());
        m_queue.pop();
        
        return rv;
    }
    
    auto push(T value) -> void
    {
        std::lock_guard<std::mutex> lock { m_mutex };
        m_queue.push(std::move(value));
        m_condition_variable.notify_one();
    }
private:
    mutable std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    std::queue<T> m_queue;
};

int main(int argc, const char * argv[]) {
    
    threadsafe_queue<ssize_t> shared_queue;
    std::thread consumer {[&shared_queue]{
        for (ssize_t i = 0; i < 10; ++i) {
            std::cout << *(shared_queue.wait_and_pop().get()) << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }};
    
    std::thread producer {[&shared_queue]{
        for (ssize_t i = 0; i < 10; ++i) {
            shared_queue.push(i);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }};
    
    consumer.join();
    producer.join();
    
    return 0;
}
