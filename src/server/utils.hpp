#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <utility>

class timeout_exception : public std::exception 
{
public:
    timeout_exception() = default;
    constexpr char const *what() const noexcept override
    {
        return "timeout";
    }
};

/**
 * @brief A wrapper method for a call with timeout.
 * 
 * @tparam ReturnType The type of the return value.
 * @tparam Func The type of the function pointer.
 * @tparam Args The types of the arguments.
 * 
 * @param ms The timeout time in milliseconds.
 * @param function The function pointer.
 * @param if_timeout The return value if the timeout occurs.
 * @param args The arguments.
 * 
 * @return The return value of the function call, or if_timeout if the timeout occurs.
 */
template<typename ReturnType, typename Func, typename... Args>
ReturnType _timeout(unsigned long ms, Func function, ReturnType if_timeout, Args&&... args)
{
    std::mutex mutex;
    std::condition_variable cv;
    ReturnType return_value;

    std::thread worker([&cv, &return_value, function, &args...]()
    {
        return_value = function(std::forward<Args>(args)...);
        cv.notify_one();
    });

    worker.detach();
    
    std::unique_lock lock(mutex);
    if (cv.wait_for(lock, std::chrono::milliseconds(ms))==std::cv_status::timeout)
        throw timeout_exception();
    return return_value;
}

/**
 * @brief A wrapper method for a call with timeout. This overload is used for 
 * member functions.
 * 
 * @tparam ClassType The type of the class that owns the function.
 * @tparam ReturnType The type of the return value.
 * @tparam Func The type of the function pointer.
 * @tparam Args The types of the arguments.
 * 
 * @param ms The timeout time in milliseconds.
 * @param this_ptr The pointer to the this object that owns the function.
 * @param function The function pointer.
 * @param if_timeout The return value if the timeout occurs.
 * @param args The arguments.
 * 
 * @return The return value of the function call, or if_timeout if the timeout occurs.
 */
template<typename ClassType, typename ReturnType, typename Func, typename... Args>
ReturnType _timeout(unsigned long ms, ClassType* this_ptr, Func function, ReturnType if_timeout, Args&&... args)
{
    std::mutex mutex;
    std::condition_variable cv;
    ReturnType return_value;

    std::thread worker([&cv, &return_value, this_ptr, function, &args...]()
    {

        return_value = (this_ptr->*function)(std::forward<Args>(args)...);
        cv.notify_one();
    });

    worker.detach();

    std::unique_lock lock(mutex);
    if (cv.wait_for(lock, std::chrono::milliseconds(ms))==std::cv_status::timeout)
        throw timeout_exception();

    return return_value;
}
