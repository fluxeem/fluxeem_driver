#ifndef EVT3_VALIDATOR_H__
#define EVT3_VALIDATOR_H__

#include <functional>
#include <map>
#include <ostream>
#include <string>
#include <fluxeem/base/define/evt3_types.h>

namespace fluxeem
{

    template <class SelfType>
    class ValidatorInterface
    {
    protected:
        // std::map<size_t, I_Decoder::ProtocolViolationCallback_t> notifiers_map_;
        size_t next_cb_idx_{0};
        int height_;
        int width_;

    public:
        constexpr static int TIME_HIGH_MAX_VALUE = 0xFFF;
        constexpr static int LOOSE_TIME_HIGH_OVERFLOW_EPSILON =
            0xFF; // Arbitrary value to distinguish data swap from data drop
        using timestamp = uint64_t;
        ValidatorInterface(int height, int width) : height_(height), width_(width) {}

        bool validateEvent2D(const Evt3Raw::RawEvent *raw_events)
        {
            return static_cast<SelfType *>(this)->validateEvent2DImpl(raw_events);
        }

        bool validateExtTrigger(const Evt3Raw::RawEvent *raw_events)
        {
            return static_cast<SelfType *>(this)->validateExtTriggerImpl(raw_events);
        }

        bool hasValidVectBase()
        {
            return static_cast<SelfType *>(this)->hasValidVectBaseImpl();
        }

        bool validateVect12By12By8Pattern(const Evt3Raw::RawEvent *raw_events, unsigned vect_base,
                                          int &next_valid_offset)
        {
            return static_cast<SelfType *>(this)->validateVect12By12By8PatternImpl(raw_events, vect_base,
                                                                                   next_valid_offset);
        }

        bool validateContinue12By12By4Pattern(const Evt3Raw::RawEvent *raw_events, int &next_valid_offset)
        {
            return static_cast<SelfType *>(this)->validateContinue12By12By4PatternImpl(raw_events, next_valid_offset);
        }

        void validateTimeHigh(timestamp prev_time_high, timestamp time_high)
        {
            static_cast<SelfType *>(this)->validateTimeHighImpl(prev_time_high, time_high);
        }

        void stateUpdate(const Evt3Raw::RawEvent *raw_event)
        {
            static_cast<SelfType *>(this)->stateUpdateImpl(raw_event);
        }

    protected:
        static bool isStrictTimeHighOverflow(timestamp prev_time_high, timestamp time_high)
        {
            return prev_time_high == TIME_HIGH_MAX_VALUE && time_high == 0;
        }
        static bool isLooseTimeHighOverflow(timestamp prev_time_high, timestamp time_high)
        {
            return (time_high - prev_time_high + TIME_HIGH_MAX_VALUE) < LOOSE_TIME_HIGH_OVERFLOW_EPSILON;
        }
    };

    class NullCheckValidator : public ValidatorInterface<NullCheckValidator>
    {
    public:
        NullCheckValidator(int height, int width) : ValidatorInterface<NullCheckValidator>(height, width) {}

        bool validateEvent2DImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events)
        {
            return true;
        }

        bool validateExtTriggerImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events)
        {
            return true;
        }

        bool hasValidVectBaseImpl()
        {
            return true;
        }

        bool validateVect12By12By8PatternImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events,
                                            [[maybe_unused]] unsigned vect_base, int &next_valid_offset)
        {
            next_valid_offset = sizeof(Evt3Raw::Event_Vect12_12_8) / sizeof(Evt3Raw::RawEvent);
            return true;
        }

        bool validateContinue12By12By4PatternImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events,
                                                int &next_valid_offset)
        {
            next_valid_offset = sizeof(Evt3Raw::Event_Continue12_12_4) / sizeof(Evt3Raw::RawEvent);
            return true;
        }

        void validateTimeHighImpl([[maybe_unused]] timestamp prev_time_high, [[maybe_unused]] timestamp time_high) {}

        void stateUpdateImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_event) {}
    };

    class BasicCheckValidator : public ValidatorInterface<BasicCheckValidator>
    {
        bool has_vect_base_ = false;

    public:
        BasicCheckValidator(int height, int width) : ValidatorInterface<BasicCheckValidator>(height, width) {}

        bool validateEvent2DImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events)
        {
            return true;
        }

        bool validateExtTriggerImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events)
        {
            return true;
        }

        bool hasValidVectBaseImpl()
        {
            return true;
        }

        bool validateVect12By12By8PatternImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events, unsigned vect_base,
                                              int &next_valid_offset)
        {
            next_valid_offset = sizeof(Evt3Raw::Event_Vect12_12_8) / sizeof(Evt3Raw::RawEvent);

            if (!has_vect_base_ || int(vect_base + 32) > width_)
            {
                has_vect_base_ = false;
                // notify(DecoderProtocolViolation::InvalidVectBase);
                return false;
            }

            return true;
        }

        bool validateContinue12By12By4PatternImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events,
                                                  int &next_valid_offset)
        {
            next_valid_offset = sizeof(Evt3Raw::Event_Continue12_12_4) / sizeof(Evt3Raw::RawEvent);
            return true;
        }

        void validateTimeHighImpl(timestamp prev_time_high, timestamp time_high)
        {
            if (isStrictTimeHighOverflow(prev_time_high, time_high))
            {
                return;
            }

            int64_t timehigh_delta = static_cast<int64_t>(time_high) - static_cast<int64_t>(prev_time_high);
            bool is_monotonic = 0 <= timehigh_delta;
            if (!is_monotonic && !isLooseTimeHighOverflow(prev_time_high, time_high))
            {
                // notify(DecoderProtocolViolation::NonMonotonicTimeHigh);
            }
        }

        void stateUpdateImpl(const Evt3Raw::RawEvent *raw_event)
        {
            if (raw_event->type == uint8_t(Evt3EventTypes_4bits::VECT_BASE_X))
            {
                has_vect_base_ = true;
            }
        }
    };

    class GrammarValidator : public ValidatorInterface<GrammarValidator>
    {
        bool is_valid_time_high_ = false;
        bool has_addr_y_ = false;
        bool has_vect_base_ = false;

    public:
        GrammarValidator(int height, int width) : ValidatorInterface<GrammarValidator>(height, width) {}

        bool validateEvent2DImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events)
        {
            if (!has_addr_y_)
            {
                // notify(DecoderProtocolViolation::MissingYAddr);
                return false;
            }
            return is_valid_time_high_;
        }

        bool validateExtTriggerImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events)
        {
            return is_valid_time_high_;
        }

        bool hasValidVectBaseImpl()
        {
            return has_vect_base_;
        }

        bool validateVect12By12By8PatternImpl(const Evt3Raw::RawEvent *raw_events, unsigned vect_base,
                                              int &next_valid_offset)
        {
            next_valid_offset = 0;
            if ((raw_events + 1)->type != uint8_t(Evt3EventTypes_4bits::VECT_12))
            {
                next_valid_offset = 1;
            }
            else if ((raw_events + 2)->type != uint8_t(Evt3EventTypes_4bits::VECT_8))
            {
                next_valid_offset = 2;
            }

            if (next_valid_offset > 0)
            {
                // notify(DecoderProtocolViolation::PartialVect_12_12_8);
                has_vect_base_ = false;
                return false;
            }

            next_valid_offset = sizeof(Evt3Raw::Event_Vect12_12_8) / sizeof(Evt3Raw::RawEvent);

            if (!has_vect_base_ || int(vect_base + 32) > width_)
            {
                has_vect_base_ = false;
                // notify(DecoderProtocolViolation::InvalidVectBase);
                return false;
            }

            if (!has_addr_y_)
            {
                // notify(DecoderProtocolViolation::MissingYAddr);
                return false;
            }

            return is_valid_time_high_;
        }

        bool validateContinue12By12By4PatternImpl(const Evt3Raw::RawEvent *raw_events, int &next_valid_offset)
        {
            next_valid_offset = 0;
            if ((raw_events)->type != uint8_t(Evt3EventTypes_4bits::CONTINUED_12))
            {
                // notify(DecoderProtocolViolation::PartialContinued_12_12_4);
                return false;
            }
            else if ((raw_events + 1)->type != uint8_t(Evt3EventTypes_4bits::CONTINUED_12))
            {
                next_valid_offset = 1;
            }
            else if ((raw_events + 2)->type != uint8_t(Evt3EventTypes_4bits::CONTINUED_4))
            {
                next_valid_offset = 2;
            }

            if (next_valid_offset > 0)
            {
                // notify(DecoderProtocolViolation::PartialContinued_12_12_4);
                return false;
            }

            next_valid_offset = sizeof(Evt3Raw::Event_Continue12_12_4) / sizeof(Evt3Raw::RawEvent);

            return is_valid_time_high_;
        }

        void validateTimeHighImpl(timestamp prev_time_high, timestamp time_high)
        {
            int64_t timehigh_delta = static_cast<int64_t>(time_high) - static_cast<int64_t>(prev_time_high);
            bool is_monotonic = 0 <= timehigh_delta;

            is_valid_time_high_ = is_monotonic || isLooseTimeHighOverflow(prev_time_high, time_high);

            if (isStrictTimeHighOverflow(prev_time_high, time_high))
            {
                return;
            }

            if (!is_valid_time_high_)
            {
                // notify(DecoderProtocolViolation::NonMonotonicTimeHigh);
            }
            else if (timehigh_delta != 0 && timehigh_delta != 1)
            {
                // notify(DecoderProtocolViolation::NonContinuousTimeHigh);
            }
        }

        void stateUpdateImpl(const Evt3Raw::RawEvent *raw_event)
        {
            if (raw_event->type == uint8_t(Evt3EventTypes_4bits::EVT_ADDR_Y))
            {
                has_addr_y_ = true;
            }
            if (raw_event->type == uint8_t(Evt3EventTypes_4bits::VECT_BASE_X))
            {
                has_vect_base_ = true;
            }
        }
    };

} // namespace fluxeem

#endif // EVT3_VALIDATOR_H__
