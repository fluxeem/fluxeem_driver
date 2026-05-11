// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef __OBJECT_POOL_H
#define __OBJECT_POOL_H

#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace fluxeem {

/// \~english @brief A recyclable pool for heap-allocated objects
/// \~chinese @brief 堆对象可复用池
///
/// \~english @details When an object obtained from the pool is destroyed, it is
///                  automatically returned to the pool instead of being freed.
///                  Call @ref acquire to obtain an object: if the pool has a free
///                  one it will be reused, otherwise a new instance is allocated.
///                  Depending on the @a acquire_shared_ptr template argument, objects
///                  are handed out as @a std::shared_ptr or @a std::unique_ptr
///                  with a custom deleter that recycles the object back into the pool.
/// \~chinese @details ObjectPool 在对象销毁时将其归还池中，而非释放内存。
///                  通过 @ref acquire 获取对象：若池中有可用对象则复用，否则新建。
///                  根据 @a acquire_shared_ptr 模板参数，对象以 @a std::shared_ptr
///                  或 @a std::unique_ptr 形式返回，并携带自定义删除器，
///                  在引用计数归零时自动将对象回收到池中。
///
/// \~english @tparam T Element type stored inside the pool
/// \~chinese @tparam T 池中存储的对象类型
/// \~english @tparam acquire_shared_ptr When true, @ref acquire returns shared_ptr; otherwise unique_ptr
/// \~chinese @tparam acquire_shared_ptr 为 true 时返回 shared_ptr，否则返回 unique_ptr
template<class T, bool acquire_shared_ptr = false>
class ObjectPool {
private:
    struct Impl;
    struct Deleter {
        explicit Deleter(std::weak_ptr<Impl> pool) : pool_(pool) {}
        void operator()(T *ptr) {
            if (auto pool_ptr = pool_.lock())
                try {
                    pool_ptr->add(std::unique_ptr<T>{ptr});
                } catch (...) {
                    /// \~english Allocation failure — fall back to default destruction
                    /// \~chinese 内存不足，直接释放
                    std::default_delete<T>{}(ptr);
                }
            else
                std::default_delete<T>{}(ptr);
        }

    private:
        std::weak_ptr<Impl> pool_;
    };

public:
    using ptr_type =
        typename std::conditional<acquire_shared_ptr, std::shared_ptr<T>, std::unique_ptr<T, Deleter>>::type;

    /// \~english @brief Build a fixed-capacity object pool
    /// \~chinese @brief 创建容量有限的对象池
    ///
    /// \~english @details When the pool is exhausted, @ref acquire will block until
    ///                  an object is returned, rather than allocating on the heap.
    /// \~chinese @details 池满时调用 @ref acquire 不会触发新的堆分配，
    ///                  而是阻塞等待直到有对象被回收到池中。
    ///
    /// \~english @param num_initial_objects Number of objects to pre-allocate
    /// \~chinese @param num_initial_objects 池初始预分配的对象数量
    /// \~english @return A bounded-capacity object pool
    /// \~chinese @return 容量有限的对象池实例
    static ObjectPool<T, acquire_shared_ptr> make_bounded(size_t num_initial_objects = 64) {
        return ObjectPool(num_initial_objects, true);
    }

    /// \~english @brief Build a fixed-capacity object pool with constructor arguments
    /// \~chinese @brief 创建容量有限的对象池（带构造参数）
    ///
    /// \~english @details When the pool is exhausted, @ref acquire will block until
    ///                  an object is returned, rather than allocating on the heap.
    /// \~chinese @details 池满时调用 @ref acquire 不会触发新的堆分配，
    ///                  而是阻塞等待直到有对象被回收到池中。
    ///
    /// \~english @param num_initial_objects Number of objects to pre-allocate
    /// \~chinese @param num_initial_objects 池初始预分配的对象数量
    /// \~english @param args Arguments forwarded to each object's constructor
    /// \~chinese @param args 传递给对象构造函数的参数
    /// \~english @return A bounded-capacity object pool
    /// \~chinese @return 容量有限的对象池实例
    template<typename... Args>
    static ObjectPool<T, acquire_shared_ptr> make_bounded(size_t num_initial_objects, Args &&...args) {
        return ObjectPool(num_initial_objects, true, std::forward<Args>(args)...);
    }

    /// \~english @brief Build a dynamically-growing object pool
    /// \~chinese @brief 创建可弹性扩展的对象池
    ///
    /// \~english @details If all pooled objects are in use when @ref acquire is called,
    ///                  a fresh object is allocated on the heap and returned.
    /// \~chinese @details 当池中所有对象均在用且 @ref acquire 被调用时，
    ///                  将在堆上新建对象并返回。
    ///
    /// \~english @param num_initial_objects Number of objects to pre-allocate
    /// \~chinese @param num_initial_objects 池初始预分配的对象数量
    /// \~english @return An unbounded object pool
    /// \~chinese @return 可弹性扩展的对象池实例
    template<typename... Args>
    static ObjectPool<T, acquire_shared_ptr> make_unbounded(size_t num_initial_objects = 64) {
        return ObjectPool(num_initial_objects, false);
    }

    /// \~english @brief Build a dynamically-growing object pool with constructor arguments
    /// \~chinese @brief 创建可弹性扩展的对象池（带构造参数）
    ///
    /// \~english @details If all pooled objects are in use when @ref acquire is called,
    ///                  a fresh object is allocated on the heap and returned.
    /// \~chinese @details 当池中所有对象均在用且 @ref acquire 被调用时，
    ///                  将在堆上新建对象并返回。
    ///
    /// \~english @param num_initial_objects Number of objects to pre-allocate
    /// \~chinese @param num_initial_objects 池初始预分配的对象数量
    /// \~english @param args Arguments forwarded to each object's constructor
    /// \~chinese @param args 传递给对象构造函数的参数
    /// \~english @return An unbounded object pool
    /// \~chinese @return 可弹性扩展的对象池实例
    template<typename... Args>
    static ObjectPool<T, acquire_shared_ptr> make_unbounded(size_t num_initial_objects, Args &&...args) {
        return ObjectPool(num_initial_objects, false, std::forward<Args>(args)...);
    }

    /// \~english @brief Default constructor — creates an unbounded pool with 64 pre-allocated objects
    /// \~chinese @brief 默认构造函数，创建含 64 个对象的弹性池
    /// \~english @see make_unbounded
    /// \~chinese @see make_unbounded
    ObjectPool() : ObjectPool(64, false) {
        static_assert(std::is_default_constructible<T>::value, "Using ObjectPool default constructor: object "
                                                               "must be default constructible. Otherwise, use static "
                                                               "build method.");
    };

    /// \~english @brief Return an object to the pool for reuse
    /// \~chinese @brief 将对象归还到池中
    /// \~english @param t Unique pointer holding the object to recycle
    /// \~chinese @param t 包含待归还对象的 unique_ptr
    void add(std::unique_ptr<T> t) {
        impl_->add(std::move(t));
    }

    /// \~english @brief Obtain an object from the pool (reused or newly allocated)
    /// \~chinese @brief 从池中获取对象（复用或新建）
    /// \~english @param args Optional constructor arguments used when a new allocation is needed
    /// \~chinese @param args 传递给对象构造函数的可选参数
    /// \~english @return A smart pointer (unique or shared) to the acquired object
    /// \~chinese @return 指向获取对象的 unique_ptr 或 shared_ptr
    template<typename... Args>
    ptr_type acquire(Args &&...args) {
        return impl_->acquire(std::forward<Args>(args)...);
    }

    /// \~english @brief Test whether the pool has no available objects
    /// \~chinese @brief 判断池中是否没有可用对象
    /// \~english @return true if empty, false if at least one object is ready for reuse
    /// \~chinese @return 池为空返回 true，有可用对象返回 false
    bool empty() const {
        return impl_->empty();
    }

    /// \~english @brief Query the number of idle objects in the pool
    /// \~chinese @brief 获取池中可用对象的数量
    /// \~english @return Count of objects that are allocated and waiting to be reused
    /// \~chinese @return 已分配且可复用的对象数量
    size_t size() const {
        return impl_->size();
    }

    /// \~english @brief Check whether the pool has a fixed capacity
    /// \~chinese @brief 查询池的容量模式
    /// \~english @return true for bounded, false for unbounded
    /// \~chinese @return 容量有限返回 true，弹性扩展返回 false
    bool is_bounded() const {
        return impl_->is_bounded();
    }

    /// \~english @brief Grow the pool so that at least @p size objects are ready to be acquired
    /// \~chinese @brief 确保池中至少有 size 个可用对象
    /// \~english @note Only effective for unbounded pools
    /// \~chinese @note 仅对弹性池有效
    /// \~english @param size Desired number of available objects
    /// \~chinese @param size 期望的可用对象数量
    /// \~english @param args Constructor arguments for any newly allocated objects
    /// \~chinese @param args 分配新对象时传递的构造参数
    /// \~english @return Number of objects newly added to the pool
    /// \~chinese @return 本次新分配的对象数量
    template<typename... Args>
    size_t arrange(size_t size, Args &&...args) {
        return impl_->arrange(size, std::forward<Args>(args)...);
    }

private:
    /// \~english @brief Internal constructor used by the static factory methods
    /// \~chinese @brief 内部构造函数
    template<typename... Args>
    ObjectPool(size_t num_initial_objects, bool bounded_memory, Args &&...args) :
        impl_(new Impl(num_initial_objects, bounded_memory, std::forward<Args>(args)...)) {}

    /// \~english @brief Pimpl holder — separates implementation to enable move semantics
    /// \~chinese @brief 对象池实现体（Pimpl 惯用法，支持移动语义）
    struct Impl : public std::enable_shared_from_this<Impl> {
        /// \~english @brief Construct the internal pool
        /// \~chinese @brief 构造函数
        template<typename... Args>
        Impl(size_t num_initial_objects, bool bounded_memory, Args &&...args) : fixed_capacity_(bounded_memory) {
            if (num_initial_objects == 0 && fixed_capacity_) {
                throw std::invalid_argument(
                    "Failed to allocate memory for the bounded object pool: pool's size can not be 0.");
            }
            free_list_.reserve(num_initial_objects);
            for (size_t i = 0; i < num_initial_objects; ++i) {
                free_list_.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
            }
        }

        /// \~english @brief Return an object to the pool
        /// \~chinese @brief 将对象归还到池中
        /// \~english @param t Unique pointer holding the object to recycle
        /// \~chinese @param t 包含待归还对象的 unique_ptr
        void add(std::unique_ptr<T> t) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                free_list_.emplace_back(std::move(t));
            }
            if (fixed_capacity_) {
                available_.notify_one();
            }
        }

        /// \~english @brief Grow the pool to the requested size (unbounded only)
        /// \~chinese @brief 扩容至指定大小（仅弹性池）
        /// \~english @param size Target number of idle objects
        /// \~chinese @param size 目标可用对象数量
        /// \~english @param args Constructor arguments for new allocations
        /// \~chinese @param args 分配新对象时传递的构造参数
        /// \~english @return Number of freshly allocated objects
        /// \~chinese @return 本次新分配的对象数量
        template<typename... Args>
        size_t arrange(size_t size, Args &&...args) {
            if (fixed_capacity_) {
                return 0;
            }

            std::lock_guard<std::mutex> lock(mutex_);
            if (size <= free_list_.size()) {
                return 0;
            }

            const size_t allocated_count = size - free_list_.size();
            free_list_.reserve(size);
            while (free_list_.size() < size) {
                free_list_.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
            }

            return allocated_count;
        }

        /// \~english @brief Obtain an object from the pool (reused or newly allocated)
        /// \~chinese @brief 从池中获取对象（复用或新建）
        /// \~english @param args Optional constructor arguments for a new allocation
        /// \~chinese @param args 传递给对象构造函数的可选参数
        /// \~english @return A smart pointer to the acquired object
        /// \~chinese @return 指向获取对象的智能指针
        template<typename... Args>
        ptr_type acquire(Args &&...args) {
            std::unique_ptr<T> selected;
            std::unique_lock<std::mutex> lock(mutex_);
            if (free_list_.empty()) {
                if (fixed_capacity_) {
                    available_.wait(lock, [this] { return !free_list_.empty(); });
                } else {
                    free_list_.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
                }
            }

            selected = std::move(free_list_.back());
            free_list_.pop_back();
            lock.unlock();

            return ptr_type(selected.release(), Deleter{this->shared_from_this()});
        }

        /// \~english @brief Test whether the pool has no available objects
        /// \~chinese @brief 判断池中是否没有可用对象
        /// \~english @return true if empty, false if at least one object is idle
        /// \~chinese @return 池为空返回 true，有可用对象返回 false
        bool empty() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return free_list_.empty();
        }

        /// \~english @brief Query the number of idle objects
        /// \~chinese @brief 获取池中可用对象的数量
        /// \~english @return Count of allocated objects waiting to be reused
        /// \~chinese @return 已分配且可复用的对象数量
        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return free_list_.size();
        }

        /// \~english @brief Check whether the pool has a fixed capacity
        /// \~chinese @brief 查询池的容量模式
        /// \~english @return true for bounded, false for unbounded
        /// \~chinese @return 容量有限返回 true，弹性扩展返回 false
        bool is_bounded() const {
            return fixed_capacity_;
        }

        mutable std::mutex mutex_;
        mutable std::condition_variable available_;
        std::vector<std::unique_ptr<T>> free_list_;
        bool fixed_capacity_{false};
    };

    std::shared_ptr<Impl> impl_;
};

/// \~english @brief Convenience alias for an ObjectPool that dispenses shared pointers
/// \~chinese @brief 返回 shared_ptr 的对象池便捷别名
/// \~english @tparam T Element type stored inside the pool
/// \~chinese @tparam T 池中存储的对象类型
template<typename T>
using SharedObjectPool = ObjectPool<T, true>;

} // namespace fluxeem

#endif // FLUXEEM_SDK_BASE_OBJECT_POOL_H
