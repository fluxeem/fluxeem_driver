#include <fluxeem/hal/pipeline/data_pipeline.hpp>
#include <fluxeem/hal/data_stream/data_transfer.hpp>
#include <fluxeem/hal/data_stream/decoder.hpp>
#include <fluxeem/base/logging/logger.h>

namespace fluxeem
{

    DataPipeline::DataPipeline(const std::string& name)
        : name_(name)
    {
    }

    DataPipeline::~DataPipeline()
    {
        stop();
    }

    void DataPipeline::setSource(std::unique_ptr<DataTransfer> source)
    {
        if (status_ == StreamStatus::RUNNING)
        {
            LOG_WARN("Cannot set source while pipeline is running");
            return;
        }
        source_ = std::move(source);
    }

    void DataPipeline::setDecoder(std::unique_ptr<Decoder> decoder)
    {
        if (status_ == StreamStatus::RUNNING)
        {
            LOG_WARN("Cannot set decoder while pipeline is running");
            return;
        }
        decoder_ = std::move(decoder);

        // Setup decoder callbacks to dispatch to our registered callbacks
        if (decoder_)
        {
            decoder_->setEventBatchHandleCallback(
                [this](EventIterator_t begin, EventIterator_t end)
                {
                    dispatchEvents(begin, end);
                });

            decoder_->setTriggerInCallback(
                [this](EventTriggerIn trigger)
                {
                    dispatchTrigger(trigger);
                });
        }
    }

    void DataPipeline::start()
    {
        if (status_ == StreamStatus::RUNNING)
        {
            LOG_WARN("Pipeline {} is already running", name_);
            return;
        }

        if (!source_)
        {
            LOG_ERROR("Pipeline {} has no source", name_);
            return;
        }

        if (!decoder_)
        {
            LOG_ERROR("Pipeline {} has no decoder", name_);
            return;
        }

        // Clear buffer queue
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            std::queue<std::shared_ptr<std::vector<uint8_t>>> empty;
            buffer_queue_.swap(empty);
        }

        status_ = StreamStatus::RUNNING;

        // Setup source callback to feed our queue
        source_->setNewBufferCallback(
            [this](std::shared_ptr<std::vector<uint8_t>> buffer)
            {
                onSourceData(buffer);
            });

        // Start processing thread
        processing_thread_ = std::thread(&DataPipeline::processingLoop, this);

        // Start source (data transfer)
        source_->start();
    }

    void DataPipeline::stop()
    {
        if (status_ != StreamStatus::RUNNING)
        {
            return;
        }

        status_ = StreamStatus::STOP;

        // Stop source first
        if (source_)
        {
            source_->stop();
        }

        // Notify processing thread to exit
        queue_cond_.notify_all();

        // Wait for processing thread
        if (processing_thread_.joinable())
        {
            processing_thread_.join();
        }

        // Clear buffer queue
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            std::queue<std::shared_ptr<std::vector<uint8_t>>> empty;
            buffer_queue_.swap(empty);
        }

    }

    void DataPipeline::onSourceData(std::shared_ptr<std::vector<uint8_t>> buffer)
    {
        if (status_ != StreamStatus::RUNNING)
        {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            buffer_queue_.push(buffer);
        }
        queue_cond_.notify_one();
    }

    void DataPipeline::processingLoop()
    {
        while (status_ == StreamStatus::RUNNING)
        {
            std::shared_ptr<std::vector<uint8_t>> buffer;

            // Wait for buffer
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cond_.wait_for(lock, std::chrono::milliseconds(100), [this]()
                {
                    return !buffer_queue_.empty() || status_ != StreamStatus::RUNNING;
                });

                if (status_ != StreamStatus::RUNNING && buffer_queue_.empty())
                {
                    break;
                }

                if (buffer_queue_.empty())
                {
                    continue;
                }

                buffer = buffer_queue_.front();
                buffer_queue_.pop();
            }

            // Process buffer
            if (buffer && !buffer->empty())
            {
                // Dispatch raw buffer to callbacks
                dispatchRawBuffer(*buffer);

                // Decode buffer (this will trigger event callbacks via decoder)
                if (decoder_)
                {
                    decoder_->decode(*buffer);
                }
            }
        }
    }

    void DataPipeline::dispatchEvents(EventIterator_t begin, EventIterator_t end)
    {
        std::lock_guard<std::mutex> lock(event_callbacks_mutex_);
        for (auto& [id, callback] : event_callbacks_)
        {
            if (callback)
            {
                callback(begin, end);
            }
        }
    }

    void DataPipeline::dispatchTrigger(EventTriggerIn trigger)
    {
        std::lock_guard<std::mutex> lock(trigger_callbacks_mutex_);
        for (auto& [id, callback] : trigger_callbacks_)
        {
            if (callback)
            {
                callback(trigger);
            }
        }
    }

    void DataPipeline::dispatchRawBuffer(const Buffer& buffer)
    {
        std::lock_guard<std::mutex> lock(buffer_callbacks_mutex_);
        for (auto& [id, callback] : buffer_callbacks_)
        {
            if (callback)
            {
                callback(buffer);
            }
        }
    }

    uint32_t DataPipeline::registerEventCallback(EventCallback cb)
    {
        std::lock_guard<std::mutex> lock(event_callbacks_mutex_);
        uint32_t id = next_event_callback_id_++;
        event_callbacks_[id] = std::move(cb);
        return id;
    }

    bool DataPipeline::unregisterEventCallback(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(event_callbacks_mutex_);
        return event_callbacks_.erase(id) > 0;
    }

    uint32_t DataPipeline::registerTriggerCallback(TriggerCallback cb)
    {
        std::lock_guard<std::mutex> lock(trigger_callbacks_mutex_);
        uint32_t id = next_trigger_callback_id_++;
        trigger_callbacks_[id] = std::move(cb);
        return id;
    }

    bool DataPipeline::unregisterTriggerCallback(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(trigger_callbacks_mutex_);
        return trigger_callbacks_.erase(id) > 0;
    }

    uint32_t DataPipeline::registerRawBufferCallback(BufferCallback cb)
    {
        std::lock_guard<std::mutex> lock(buffer_callbacks_mutex_);
        uint32_t id = next_buffer_callback_id_++;
        buffer_callbacks_[id] = std::move(cb);
        return id;
    }

    bool DataPipeline::unregisterRawBufferCallback(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(buffer_callbacks_mutex_);
        return buffer_callbacks_.erase(id) > 0;
    }

} // namespace fluxeem
