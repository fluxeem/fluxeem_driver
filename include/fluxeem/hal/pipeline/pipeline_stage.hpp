#ifndef __PIPELINE_STAGE_HPP__
#define __PIPELINE_STAGE_HPP__

#include <memory>
#include <functional>
#include <string>
#include <atomic>

#include <fluxeem/base/define/base_define.h>

namespace fluxeem
{

    /**
     * @brief Base class for all pipeline stages
     * 
     * A pipeline stage represents a single processing step in the data flow.
     * Each stage receives data from the previous stage, processes it, and
     * passes the result to the next stage.
     * 
     * @tparam InputT  The input data type for this stage
     * @tparam OutputT The output data type for this stage
     */
    template <typename InputT, typename OutputT>
    class PipelineStage
    {
    public:
        using InputType = InputT;
        using OutputType = OutputT;
        using OutputCallback = std::function<void(OutputT)>;

        PipelineStage(const std::string& name = "unnamed_stage") 
            : name_(name), enabled_(true) {}

        virtual ~PipelineStage() = default;

        /// @brief Process input data and produce output
        /// @param input The input data to process
        virtual void process(InputT input) = 0;

        /// @brief Set the callback for output data
        void setOutputCallback(OutputCallback cb)
        {
            output_callback_ = std::move(cb);
        }

        /// @brief Enable or disable this stage
        void setEnabled(bool enabled) { enabled_ = enabled; }

        /// @brief Check if this stage is enabled
        bool isEnabled() const { return enabled_; }

        /// @brief Get the name of this stage
        const std::string& getName() const { return name_; }

        /// @brief Called when pipeline starts
        virtual void onStart() {}

        /// @brief Called when pipeline stops
        virtual void onStop() {}

    protected:
        /// @brief Emit output to the next stage
        void emit(OutputT output)
        {
            if (output_callback_ && enabled_)
            {
                output_callback_(std::move(output));
            }
        }

        std::string name_;
        std::atomic<bool> enabled_;
        OutputCallback output_callback_;
    };

    /**
     * @brief A source stage that only produces output (no input)
     * 
     * @tparam OutputT The output data type
     */
    template <typename OutputT>
    class SourceStage
    {
    public:
        using OutputType = OutputT;
        using OutputCallback = std::function<void(OutputT)>;

        SourceStage(const std::string& name = "source_stage") 
            : name_(name), running_(false) {}

        virtual ~SourceStage() = default;

        /// @brief Set the callback for output data
        void setOutputCallback(OutputCallback cb)
        {
            output_callback_ = std::move(cb);
        }

        /// @brief Start producing data
        virtual void start() = 0;

        /// @brief Stop producing data
        virtual void stop() = 0;

        /// @brief Check if running
        bool isRunning() const { return running_; }

        /// @brief Get the name of this stage
        const std::string& getName() const { return name_; }

    protected:
        /// @brief Emit output to the next stage
        void emit(OutputT output)
        {
            if (output_callback_)
            {
                output_callback_(std::move(output));
            }
        }

        std::string name_;
        std::atomic<bool> running_;
        OutputCallback output_callback_;
    };

    /**
     * @brief A sink stage that only consumes input (no output)
     * 
     * @tparam InputT The input data type
     */
    template <typename InputT>
    class SinkStage
    {
    public:
        using InputType = InputT;

        SinkStage(const std::string& name = "sink_stage") 
            : name_(name), enabled_(true) {}

        virtual ~SinkStage() = default;

        /// @brief Process input data
        virtual void process(InputT input) = 0;

        /// @brief Enable or disable this stage
        void setEnabled(bool enabled) { enabled_ = enabled; }

        /// @brief Check if this stage is enabled
        bool isEnabled() const { return enabled_; }

        /// @brief Get the name of this stage
        const std::string& getName() const { return name_; }

        /// @brief Called when pipeline starts
        virtual void onStart() {}

        /// @brief Called when pipeline stops
        virtual void onStop() {}

    protected:
        std::string name_;
        std::atomic<bool> enabled_;
    };

} // namespace fluxeem

#endif // __PIPELINE_STAGE_HPP__
