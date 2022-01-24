/**
 * Try to improve the performance of the program a bit by learning to write some
 * new things.
 */

#ifndef MJ_UTILS_OPTIM_HPP
#define MJ_UTILS_OPTIM_HPP

#include <memory>

template <std::size_t MaxSize>
struct optim
{
    optim() = delete;

    /**
     * @brief Allocators for allocating vectors below a certain size on the stack.
     */
    template <typename Type>
    struct allocator : public std::allocator<Type>
    {
        Type *allocate(size_t n)
        {
            if (n > MaxSize)
                return new Type[n];
            return data;
        }

        void deallocate(Type *ptr, size_t n)
        {
            if (n > MaxSize)
                delete[] ptr;
        }
    private:
        Type data[MaxSize];
    };
};

#endif
