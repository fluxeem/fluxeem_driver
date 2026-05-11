#ifndef EVENT_CAMERA_HPP
#define EVENT_CAMERA_HPP

#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <unordered_map>
#include <condition_variable>
#include <fluxeem/driver/camera/base/i_camera.hpp>
#include <fluxeem/hal/common/interface.hpp>
#include <fluxeem/hal/registers/register_controller.h>
#include <fluxeem/hal/data_stream/decoder.hpp>
#include <fluxeem/hal/data_stream/decode_statistics.h>
#include <fluxeem/driver/sensors/sensor.hpp>
#include <fluxeem/hal/pipeline/data_pipeline.hpp>
#include <fluxeem/hal/data_stream/data_transfer_factory.hpp>
#include <fluxeem/hal/data_stream/raw_event_stream_encoding_type.hpp>

namespace fluxeem {

	class FLUXEEM_API EventCamera : public ICamera
	{
	public:
		EventCamera(CameraDescription cameraDesc);

		virtual ~EventCamera();

		bool start() override;

		bool stop() override;

		bool close() override;

		bool getNextBatch(EventBatch& event_batch) override;

		bool startRecording(const std::string& file_path) override;

		bool stopRecording() override;

		uint32_t registerEventBatchCallback(EventBatchHandleCallback cb) override;

		bool unregisterEventBatchCallback(uint32_t callback_id) override;

		uint32_t registerTriggerInCallback(EvTriggerInCallback cb);

		bool unregisterTriggerInCallback(uint32_t callback_id) override;

        void setBatchEventsNum(uint64_t n) override;

        void setBatchEventsTime(Timestamp t) override;

		void setStatisticsCallback(EvCameraStatisticInfoCallback cb) override;

		bool firmwareUpgrade(const std::string &image_path) override;

	protected:
        //get batch events
        BatchConditionType get_batch_condition_ = BatchConditionType::NO_CONDITION;
		BatchConditionType get_batch_condition_tmp_;
		std::mutex get_batch_condition_mutex_;
        uint64_t accumulate_events_num_ = 128 * 1024;//By default, 128K events are collected each time
        uint64_t accumulate_events_time_ = 10 * 1000; //us. The default value is 10ms each time
		
		//set register
		std::shared_ptr<RegisterController> register_controller_;

		// Camera components (decoupled)
		std::shared_ptr<Interface> interface_;
		std::unique_ptr<Sensor> sensor_;           // Hardware configuration only
		std::unique_ptr<DataPipeline> pipeline_;   // Data flow management

		// Event handling
		void eventStreamArrived(EventIterator_t begin, EventIterator_t end);

		//Camera status
		bool is_initialized_ = false;
		std::atomic<CameraStatus> status_ = CameraStatus::STOPPED;

		//n events
		std::vector<Event2D> n_events_front_, n_events_back_;
		std::mutex n_events_processing_mutex_;
		std::condition_variable n_events_available_cond_;

		// n time events
		std::vector<Event2D> n_time_events_front_, n_time_events_back_;
		std::mutex n_time_events_processing_mutex_;
		std::condition_variable n_time_events_available_cond_;

		// Save file
		EvFileInfo ev_file_info_;
		std::atomic<bool> is_recording_{false};
		std::atomic<bool> recording_stop_requested_{false};
		std::ofstream recording_output_file_stream_;
		std::thread recording_thread_;
		std::mutex recording_mutex_;
		std::condition_variable recording_available_cond_;
		std::queue<std::shared_ptr<std::vector<uint8_t>>> recording_queue_;

		void enqueueRawDataForRecording(const std::vector<uint8_t>& buffer);
		void recordingLoop();

		virtual RawEventStreamFormat getRawEventStreamFormat() = 0;

		// data statistics (decoupled from decoder)
		DecodeStatistics decode_statistics_;
		std::thread statistic_loop_thread_;
		EvCameraStatisticInfoCallback ds_statistic_info_callback_ = nullptr;
		std::mutex statistic_callback_mutex_;
		void statisticLoop();
	};

}

#endif // EVENT_CAMERA_HPP