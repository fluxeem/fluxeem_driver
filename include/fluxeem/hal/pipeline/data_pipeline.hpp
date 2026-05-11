#ifndef __DATA_PIPELINE_HPP__
#define __DATA_PIPELINE_HPP__

#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <functional>
#include <string>
#include <unordered_map>

#include <fluxeem/base/define/base_define.h>
#include <fluxeem/base/define/event_type.h>
#include <fluxeem/hal/pipeline/pipeline_stage.hpp>

namespace fluxeem
{
    // Forward declarations
    class DataTransfer;
    class Decoder;

    /**
     * @brief A data pipeline that connects source -> processing stages -> sink
     * 
     * The pipeline manages the data flow from a DataTransfer source through
     * a Decoder and delivers decoded events to registered callbacks.
     * 
     * Architecture:
     *   DataTransfer (Source) --> Decoder (Transform) --> EventCallbacks (Sinks)
     * 
     * Features:
     * - Single worker thread for processing
     * - Non-blocking buffer queue between source and decoder
     * - Multiple event callbacks support
     * - Thread-safe start/stop
     */
    class FLUXEEM_API DataPipeline
    {
    public:
        using Buffer = std::vector<uint8_t>;
        using BufferPtr = std::shared_ptr<Buffer>;
        using EventCallback = EventBatchHandleCallback;
        using TriggerCallback = EvTriggerInCallback;
        using BufferCallback = std::function<void(const Buffer&)>;

        /**
         * @brief Construct a data pipeline
         * @param name Pipeline name for logging
         */
        explicit DataPipeline(const std::string& name = "data_pipeline");

        ~DataPipeline();

        // Non-copyable, non-movable
        DataPipeline(const DataPipeline&) = delete;
        DataPipeline& operator=(const DataPipeline&) = delete;

        /**
         * @brief Set the data source (DataTransfer)
         * @param source The data transfer source
         */
        void setSource(std::unique_ptr<DataTransfer> source);

        /**
         * @brief Set the decoder for raw data
         * @param decoder The decoder to use
         */
        void setDecoder(std::unique_ptr<Decoder> decoder);

        /**
         * @brief Get the decoder
         */
        Decoder* getDecoder() { return decoder_.get(); }

        /**
         * @brief Get the data transfer source
         */
        DataTransfer* getSource() { return source_.get(); }

        /**
         * @brief Start the pipeline
         */
        void start();

        /**
         * @brief Stop the pipeline
         */
        void stop();

        /**
         * @brief Check if pipeline is running
         */
        bool isRunning() const { return status_ == StreamStatus::RUNNING; }

        // ----- Event Callbacks Management -----

        /**
         * @brief Register an event callback
         * @param cb The callback function
         * @return Callback ID for unregistration
         */
        uint32_t registerEventCallback(EventCallback cb);

        /**
         * @brief Unregister an event callback
         * @param id The callback ID
         * @return true if removed successfully
         */
        bool unregisterEventCallback(uint32_t id);

        /**
         * @brief Register a trigger callback
         * @param cb The callback function
         * @return Callback ID for unregistration
         */
        uint32_t registerTriggerCallback(TriggerCallback cb);

        /**
         * @brief Unregister a trigger callback
         * @param id The callback ID
         * @return true if removed successfully
         */
        bool unregisterTriggerCallback(uint32_t id);

        /**
         * @brief Register a raw buffer callback (called before decoding)
         * @param cb The callback function
         * @return Callback ID for unregistration
         */
        uint32_t registerRawBufferCallback(BufferCallback cb);

        /**
         * @brief Unregister a raw buffer callback
         * @param id The callback ID
         * @return true if removed successfully
         */
        bool unregisterRawBufferCallback(uint32_t id);

    private:
        /**
         * @brief Main processing loop
         */
        void processingLoop();

        /**
         * @brief Called when source has new data
         */
        void onSourceData(std::shared_ptr<std::vector<uint8_t>> buffer);

        /**
         * @brief Dispatch events to all registered callbacks
         */
        void dispatchEvents(EventIterator_t begin, EventIterator_t end);

        /**
         * @brief Dispatch trigger to all registered callbacks
         */
        void dispatchTrigger(EventTriggerIn trigger);

        /**
         * @brief Dispatch raw buffer to all registered callbacks
         */
        void dispatchRawBuffer(const Buffer& buffer);

        std::string name_;
        std::atomic<StreamStatus> status_{StreamStatus::STOP};

        // Pipeline components
        std::unique_ptr<DataTransfer> source_;
        std::unique_ptr<Decoder> decoder_;

        // Processing thread
        std::thread processing_thread_;
        std::mutex queue_mutex_;
        std::condition_variable queue_cond_;
        std::queue<std::shared_ptr<std::vector<uint8_t>>> buffer_queue_;

        // Event callbacks
        std::unordered_map<uint32_t, EventCallback> event_callbacks_;
        uint32_t next_event_callback_id_ = 0;
        std::mutex event_callbacks_mutex_;

        // Trigger callbacks
        std::unordered_map<uint32_t, TriggerCallback> trigger_callbacks_;
        uint32_t next_trigger_callback_id_ = 0;
        std::mutex trigger_callbacks_mutex_;

        // Raw buffer callbacks
        std::unordered_map<uint32_t, BufferCallback> buffer_callbacks_;
        uint32_t next_buffer_callback_id_ = 0;
        std::mutex buffer_callbacks_mutex_;
    };

} // namespace fluxeem

#endif // __DATA_PIPELINE_HPP__
