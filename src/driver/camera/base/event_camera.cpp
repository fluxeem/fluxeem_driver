#include <fluxeem/driver/camera/base/event_camera.hpp>
#include <fluxeem/base/utility/func_utils.h>
#include <fluxeem/base/logging/logger.h>
#include <fluxeem/hal/common/usb_interface.h>
#include <fstream>
#include <limits>

namespace fluxeem
{
    EventCamera::EventCamera(CameraDescription cameraDesc) : ICamera(cameraDesc)
    {
        ev_file_info_ = {0, 0, 0, 1280, 720, cameraDesc.serial, {}};
    }

    EventCamera::~EventCamera()
    {
        close();
    }

    uint32_t EventCamera::registerEventBatchCallback(EventBatchHandleCallback cb) {
        if (pipeline_) {
            return pipeline_->registerEventCallback(cb);
        }
        return UINT32_MAX;
    }

    bool EventCamera::unregisterEventBatchCallback(uint32_t callback_id)
    {
        if (pipeline_) {
            return pipeline_->unregisterEventCallback(callback_id);
        }
        return false;
    }

    uint32_t EventCamera::registerTriggerInCallback(EvTriggerInCallback cb) {
        if (pipeline_) {
            return pipeline_->registerTriggerCallback(cb);
        }
        return UINT32_MAX;
    }

    bool EventCamera::unregisterTriggerInCallback(uint32_t callback_id)
    {
        if (pipeline_) {
            return pipeline_->unregisterTriggerCallback(callback_id);
        }
        return false;
    }

    void EventCamera::setBatchEventsNum(uint64_t n) {
        accumulate_events_num_ = n;
        std::unique_lock<std::mutex> lock(get_batch_condition_mutex_);
        get_batch_condition_ = BatchConditionType::N_EVENTS;
    }

    void EventCamera::setBatchEventsTime(Timestamp t)
    {
        accumulate_events_time_ = t;
        std::unique_lock<std::mutex> lock(get_batch_condition_mutex_);
        get_batch_condition_ = BatchConditionType::N_US;
    }

    bool EventCamera::getNextBatch(EventBatch& event_batch)
    {
        event_batch.clear();
        {
            std::unique_lock<std::mutex> lock(get_batch_condition_mutex_);
            get_batch_condition_tmp_ = get_batch_condition_;
        }
        if (get_batch_condition_tmp_ == BatchConditionType::N_EVENTS)
        {
            std::unique_lock<std::mutex> lock(n_events_processing_mutex_);
            n_events_available_cond_.wait(lock);
            event_batch.swap(n_events_front_);
            return true;
        }
        else if (get_batch_condition_tmp_ == BatchConditionType::N_US)
        {
            std::unique_lock<std::mutex> lock(n_time_events_processing_mutex_);
            n_time_events_available_cond_.wait(lock);
            event_batch.swap(n_time_events_front_);
            return true;
        }
        return false;
    }

    bool EventCamera::start() {
        if (!is_initialized_) {
            throw std::runtime_error("Camera is not inited");
        }

        // Register internal event callback for batch processing
        pipeline_->registerEventCallback([this](EventIterator_t begin, EventIterator_t end) {
            eventStreamArrived(begin, end);
        });

        // Register raw buffer callback for file recording
        pipeline_->registerRawBufferCallback([this](const std::vector<uint8_t>& buffer) {
            if (is_recording_) {
                enqueueRawDataForRecording(buffer);
            }
        });

        status_ = CameraStatus::STARTED;

        // Start sensor streaming (configures hardware registers)
        sensor_->startStreaming();

        // Start pipeline (data transfer + decoding)
        pipeline_->start();

        // Inject statistics collector into decoder
        if (auto* decoder = pipeline_->getDecoder()) {
            decode_statistics_.reset();
            decoder->setStatistics(&decode_statistics_);
        }

        // Start statistics thread
        statistic_loop_thread_ = std::thread([this] { statisticLoop(); });

        LOG_INFO("Camera started successfully");
        return true;
    }

    bool EventCamera::stop() {
        if (!is_initialized_) {
            return true;
        }
        if (status_ == CameraStatus::STOPPED) {
            return true;
        }
        try
        {
            status_ = CameraStatus::STOPPED;

            // Stop statistics thread first (it reads from decode_statistics_)
            if (statistic_loop_thread_.joinable()) {
                statistic_loop_thread_.join();
            }

            // Flush final statistics snapshot to callback
            {
                std::lock_guard<std::mutex> lock(statistic_callback_mutex_);
                if (ds_statistic_info_callback_) {
                    auto snap = decode_statistics_.consumeAndReset();
                    if (snap.bandwidth_bytes > 0 || snap.events_count > 0) {
                        ds_statistic_info_callback_(EvCameraStatisticInfo(
                            snap.bandwidth_bytes, snap.events_count));
                    }
                }
            }

            // Detach statistics from decoder before destroying pipeline
            if (pipeline_) {
                if (auto* decoder = pipeline_->getDecoder()) {
                    decoder->setStatistics(nullptr);
                }
            }

            // Stop pipeline (stops data flow)
            if (pipeline_) {
                pipeline_->stop();
            }

            // Stop sensor streaming (configures hardware registers)
            if (sensor_) {
                sensor_->stopStreaming();
            }

            n_events_available_cond_.notify_one();
            n_time_events_available_cond_.notify_one();

            is_initialized_ = false;
            return true;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Error stopping camera: %s", e.what());
            return false;
        }
    }

    bool EventCamera::close()
    {
        bool stop_ret = stop();

        try
        {
            stopRecording();

            if (interface_)
            {
                interface_->close();
            }

            tools_.clear();
            pipeline_.reset();
            sensor_.reset();
            register_controller_.reset();
            interface_.reset();

            {
                std::lock_guard<std::mutex> lock(n_events_processing_mutex_);
                n_events_front_.clear();
                n_events_back_.clear();
            }
            {
                std::lock_guard<std::mutex> lock(n_time_events_processing_mutex_);
                n_time_events_front_.clear();
                n_time_events_back_.clear();
            }

            n_events_available_cond_.notify_all();
            n_time_events_available_cond_.notify_all();

            decode_statistics_.reset();
            status_ = CameraStatus::STOPPED;
            is_initialized_ = false;

            return stop_ret;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Error closing camera: %s", e.what());
            return false;
        }
    }

    bool EventCamera::startRecording(const std::string& file_path)
    {
        if (!is_initialized_) {
            throw std::runtime_error("Camera is not inited");
        }

        if (is_recording_)
        {
            LOG_WARN("Recording already started.");
            return true;
        }

        recording_output_file_stream_.open(file_path, std::ios::binary | std::ios::out);
        if (!recording_output_file_stream_.is_open())
        {
            LOG_ERROR("Failed to open output file: %s", file_path.c_str());
            return false;
        }

        auto decoder = pipeline_->getDecoder();
        ev_file_info_.start_timestamp = decoder ? decoder->getLastTimestamp() : 0;
        recording_output_file_stream_ << generateEvFileHeader(ev_file_info_);
        if (!recording_output_file_stream_.good())
        {
            recording_output_file_stream_.close();
            LOG_ERROR("Failed to write file header: %s", file_path.c_str());
            return false;
        }

        recording_stop_requested_ = false;
        is_recording_ = true;
        recording_thread_ = std::thread(&EventCamera::recordingLoop, this);
        return true;
    }

    bool EventCamera::stopRecording()
    {
        if (!is_recording_ && !recording_thread_.joinable())
        {
            return true;
        }

        is_recording_ = false;
        recording_stop_requested_ = true;
        recording_available_cond_.notify_all();

        if (recording_thread_.joinable())
        {
            recording_thread_.join();
        }

        {
            std::lock_guard<std::mutex> lock(recording_mutex_);
            while (!recording_queue_.empty())
            {
                recording_queue_.pop();
            }
        }

        if (recording_output_file_stream_.is_open())
        {
            recording_output_file_stream_.flush();
            recording_output_file_stream_.close();
        }

        return true;
    }

    void EventCamera::enqueueRawDataForRecording(const std::vector<uint8_t>& buffer)
    {
        auto packet = std::make_shared<std::vector<uint8_t>>(buffer);
        {
            std::lock_guard<std::mutex> lock(recording_mutex_);
            recording_queue_.push(std::move(packet));
        }
        recording_available_cond_.notify_one();
    }

    void EventCamera::recordingLoop()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(recording_mutex_);
            recording_available_cond_.wait(lock, [this] {
                return !recording_queue_.empty() || recording_stop_requested_;
            });

            while (!recording_queue_.empty())
            {
                auto packet = recording_queue_.front();
                recording_queue_.pop();
                lock.unlock();

                recording_output_file_stream_.write(
                    reinterpret_cast<const char*>(packet->data()),
                    static_cast<std::streamsize>(packet->size()));

                lock.lock();
            }

            if (recording_stop_requested_ && recording_queue_.empty())
            {
                break;
            }
        }
    }

    void EventCamera::statisticLoop()
    {
        while(status_ == CameraStatus::STARTED)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            std::lock_guard<std::mutex> lock(statistic_callback_mutex_);
            if (ds_statistic_info_callback_)
            {
                auto snap = decode_statistics_.consumeAndReset();
                ds_statistic_info_callback_(EvCameraStatisticInfo(
                    snap.bandwidth_bytes, snap.events_count));
            }
        }
    }

    void EventCamera::eventStreamArrived(EventIterator_t begin, EventIterator_t end)
    {
        {
            std::unique_lock<std::mutex> lock(get_batch_condition_mutex_);
            get_batch_condition_tmp_ = get_batch_condition_;
        }
        //n events accumulation
        if (get_batch_condition_tmp_ == BatchConditionType::N_EVENTS)
        {
            uint64_t events_required = accumulate_events_num_ - n_events_back_.size();
            int64_t remain = std::distance(begin, end);
            EventIterator_t start_to_pushback = begin;
            while (remain > 0)
            {
                events_required = accumulate_events_num_ - n_events_back_.size();
                if (remain > static_cast<int64_t>(events_required))
                {
                    n_events_back_.insert(n_events_back_.end(), start_to_pushback, start_to_pushback + events_required);
                    {
                        std::unique_lock<std::mutex> lock(n_events_processing_mutex_);
                        n_events_front_.clear();
                        n_events_front_.swap(n_events_back_);
                        n_events_available_cond_.notify_one();
                    }
                    start_to_pushback += events_required;
                    remain -= events_required;
                }
                else
                {
                    n_events_back_.insert(n_events_back_.end(), start_to_pushback, start_to_pushback + remain);
                    remain = 0;
                }
            }
        }

        //n time events accumulation
        if (get_batch_condition_tmp_ == BatchConditionType::N_US)
        {
            EventIterator_t last_event = std::prev(end);

            int64_t remain = std::distance(begin, end);
            EventIterator_t start_to_pushback = begin;
            Timestamp target_ts;
            if (n_time_events_back_.size() > 0)
            {
                target_ts = n_time_events_back_.begin()->timestamp + accumulate_events_time_;
            }
            else
            {
                target_ts = start_to_pushback->timestamp + accumulate_events_time_;
            }

            while (remain > 0)
            {
                if (last_event->timestamp > target_ts)
                {
                    fluxeem::Event2D* target_pos = binarySearchTimestamp(target_ts, start_to_pushback, end);

                    n_time_events_back_.insert(n_time_events_back_.end(), start_to_pushback, target_pos);
                    {
                        std::unique_lock<std::mutex> lock(n_time_events_processing_mutex_);
                        n_time_events_front_.clear();
                        n_time_events_front_.swap(n_time_events_back_);
                        n_time_events_available_cond_.notify_one();
                    }
                    remain -= std::distance(start_to_pushback, target_pos);
                    start_to_pushback = target_pos;
                    target_ts += accumulate_events_time_;
                }
                else
                {
                    n_time_events_back_.insert(n_time_events_back_.end(), start_to_pushback, end);
                    remain = 0;
                }
            }
        }
    }

    void EventCamera::setStatisticsCallback(EvCameraStatisticInfoCallback cb)
    {
        std::lock_guard<std::mutex> lock(statistic_callback_mutex_);
        ds_statistic_info_callback_ = cb;
    }

    bool EventCamera::firmwareUpgrade(const std::string &image_path)
    {
        auto usb = std::dynamic_pointer_cast<UsbInterface>(interface_);
        if (!usb) { return false; }

        if (status_ == CameraStatus::STARTED)
        {
            if (stop() != 0)
            {
                LOG_ERROR("Failed to stop camera before firmware upgrade.");
                return false;
            }
        }

        LOG_INFO("Current firmware version: %s", usb->fwGetVersion().c_str());

        const bool upgraded = usb->fwUpgrade(image_path, true, true, nullptr);
        if (!upgraded)
        {
            return false;
        }

        if (close() != 0)
        {
            LOG_WARN("Firmware upgrade succeeded, but camera state cleanup failed.");
        }

        LOG_INFO("Firmware upgrade succeeded. Re-open the camera after USB re-enumeration.");
        return true;
    }
}
