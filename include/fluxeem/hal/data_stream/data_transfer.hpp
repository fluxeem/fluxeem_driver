#ifndef __DATA_TRANSFER_HPP__
#define __DATA_TRANSFER_HPP__

#include <mutex>
#include <atomic>
#include <memory>
#include <functional>

#include <fluxeem/base/define/base_define.h>
#include <fluxeem/base/utility/object_pool/object_pool.h>

namespace fluxeem
{

    /**
     * @brief Base class for data transfer from hardware
     * 
     * Responsibilities:
     * - Buffer pool management (memory allocation)
     * - Lifecycle management (start/stop)
     * - Callback-based data delivery
     * 
     * Subclass responsibilities:
     * - Actual data transfer implementation
     * - Worker thread management (if needed)
     */
    class FLUXEEM_API DataTransfer
    {
    public:
        /// Alias for the type of the internal buffer of data
        using Buffer = std::vector<uint8_t>;

        using BufferPool = SharedObjectPool<Buffer>;

        using BufferPtr = BufferPool::ptr_type;

        using NewBufferCallback = std::function<void(BufferPtr)>;

        virtual ~DataTransfer();

        /// @brief Start the data transfer
        virtual void start() = 0;

        /// @brief Stop the data transfer
        virtual void stop() = 0;

        /// @brief Register a callback to be called when a new buffer is available
        void setNewBufferCallback(NewBufferCallback cb);

        /// @brief Check if the transfer is running
        bool isRunning() const { return status_ == StreamStatus::RUNNING; }

    protected:
        DataTransfer(uint32_t buffer_pool_object_size = 128 * 1024, 
                     size_t initial_pool_size = 10);

        /// @brief Create a buffer from the pool
        BufferPtr createBuffer(size_t buffer_size = 0);

        /// @brief Called by subclass to deliver a completed buffer to consumer
        void deliverBuffer(BufferPtr buffer);

        /// @brief Flush pending data (implementation specific)
        virtual void flush() {}

        std::atomic<StreamStatus> status_{StreamStatus::STOP};

    private:
        BufferPool buffer_pool_;
        NewBufferCallback new_buffer_cb_;
        std::mutex callback_mutex_;
    };

} // namespace fluxeem

#endif // __DATA_TRANSFER_HPP__
