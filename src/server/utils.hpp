#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>

/**
 * @brief A wrapper method for a call with timeout.
 * 
 * @tparam TimeType The type of the timeout time.
 * @tparam ReturnType The type of the return value.
 * @tparam Func The type of the function pointer.
 * @tparam Args The types of the arguments.
 * 
 * @param timeout The timeout time.
 * @param function The function pointer.
 * @param if_timeout The return value if the timeout occurs.
 * @param args The arguments.
 * 
 * @return The return value of the function call, or if_timeout if the timeout occurs.
 */
template<typename TimeType, typename ReturnType, typename Func, typename... Args>
ReturnType _timeout(TimeType timeout, Func function, ReturnType if_timeout, Args&&... args)
{
    std::mutex mutex;
    std::condition_variable cv;
    ReturnType return_value;

    std::thread worker([&cv, &return_value, function, &args...]()
    {
        return_value = function(args...);
        cv.notify_one();
    });

    worker.detach();

    std::unique_lock lock(mutex);
    if (cv.wait_for(lock, timeout)==std::cv_status::timeout)
        return if_timeout;

    return return_value;
}
