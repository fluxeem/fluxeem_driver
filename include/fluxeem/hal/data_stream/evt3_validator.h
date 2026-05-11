// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file evt3_validator.h
 * @author Fluxeem
 * @since 1.0.0
 * @brief EVT3 协议验证器 — 检测原始事件流中的编码违规
 * @details 提供三级验证：空检查、基本时序检查和完整语法检查。
 *          发现违规时通过回调通知调用方。
 * @ingroup fluxeem_event_types
 */
#ifndef EVT3_VALIDATOR_H__
#define EVT3_VALIDATOR_H__

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <ostream>
#include <string>
#include <fluxeem/base/define/evt3_types.h>

namespace fluxeem
{
    namespace evt3_validation_detail
    {
        constexpr int kVect12RawWords = sizeof(Evt3Raw::Event_Vect12_12_8) / sizeof(Evt3Raw::RawEvent);
        constexpr int kContinueRawWords = sizeof(Evt3Raw::Event_Continue12_12_4) / sizeof(Evt3Raw::RawEvent);

        inline uint8_t rawType(Evt3EventTypes_4bits type)
        {
            return static_cast<uint8_t>(type);
        }

        inline bool hasType(const Evt3Raw::RawEvent *events, int offset, Evt3EventTypes_4bits expected)
        {
            return (events + offset)->type == rawType(expected);
        }

        template <std::size_t Count>
        int firstTypeMismatch(const Evt3Raw::RawEvent *events,
                              const std::array<Evt3EventTypes_4bits, Count> &expected,
                              std::size_t first_index = 0)
        {
            for (std::size_t i = first_index; i < expected.size(); ++i)
            {
                if (!hasType(events, static_cast<int>(i), expected[i]))
                {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        inline bool vectorWindowFits(bool has_base, unsigned base_x, int sensor_width)
        {
            return has_base && static_cast<int>(base_x + 32) <= sensor_width;
        }

        inline int64_t signedTimeHighDelta(uint64_t previous, uint64_t current)
        {
            return static_cast<int64_t>(current) - static_cast<int64_t>(previous);
        }
    }

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
            0xFF; // 区分数据交换与数据丢失的容差阈值
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
            next_valid_offset = evt3_validation_detail::kVect12RawWords;
            return true;
        }

        bool validateContinue12By12By4PatternImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events,
                                                int &next_valid_offset)
        {
            next_valid_offset = evt3_validation_detail::kContinueRawWords;
            return true;
        }

        void validateTimeHighImpl([[maybe_unused]] timestamp prev_time_high, [[maybe_unused]] timestamp time_high) {}

        void stateUpdateImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_event) {}
    };

    class BasicCheckValidator : public ValidatorInterface<BasicCheckValidator>
    {
        bool vect_base_seen_ = false;

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
            next_valid_offset = evt3_validation_detail::kVect12RawWords;

            if (!evt3_validation_detail::vectorWindowFits(vect_base_seen_, vect_base, width_))
            {
                vect_base_seen_ = false;
                // notify(DecoderProtocolViolation::InvalidVectBase);
                return false;
            }

            return true;
        }

        bool validateContinue12By12By4PatternImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events,
                                                  int &next_valid_offset)
        {
            next_valid_offset = evt3_validation_detail::kContinueRawWords;
            return true;
        }

        void validateTimeHighImpl(timestamp prev_time_high, timestamp time_high)
        {
            if (isStrictTimeHighOverflow(prev_time_high, time_high))
            {
                return;
            }

            int64_t timehigh_delta = evt3_validation_detail::signedTimeHighDelta(prev_time_high, time_high);
            bool is_monotonic = 0 <= timehigh_delta;
            if (!is_monotonic && !isLooseTimeHighOverflow(prev_time_high, time_high))
            {
                // notify(DecoderProtocolViolation::NonMonotonicTimeHigh);
            }
        }

        void stateUpdateImpl(const Evt3Raw::RawEvent *raw_event)
        {
            if (raw_event->type == evt3_validation_detail::rawType(Evt3EventTypes_4bits::VECT_BASE_X))
            {
                vect_base_seen_ = true;
            }
        }
    };

    class GrammarValidator : public ValidatorInterface<GrammarValidator>
    {
        bool time_high_ok_ = false;
        bool addr_y_seen_ = false;
        bool vect_base_seen_ = false;

    public:
        GrammarValidator(int height, int width) : ValidatorInterface<GrammarValidator>(height, width) {}

        bool validateEvent2DImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events)
        {
            if (!addr_y_seen_)
            {
                // notify(DecoderProtocolViolation::MissingYAddr);
                return false;
            }
            return time_high_ok_;
        }

        bool validateExtTriggerImpl([[maybe_unused]] const Evt3Raw::RawEvent *raw_events)
        {
            return time_high_ok_;
        }

        bool hasValidVectBaseImpl()
        {
            return vect_base_seen_;
        }

        bool validateVect12By12By8PatternImpl(const Evt3Raw::RawEvent *raw_events, unsigned vect_base,
                                              int &next_valid_offset)
        {
            constexpr std::array<Evt3EventTypes_4bits, 3> kExpectedPattern{
                Evt3EventTypes_4bits::VECT_12,
                Evt3EventTypes_4bits::VECT_12,
                Evt3EventTypes_4bits::VECT_8,
            };

            next_valid_offset = evt3_validation_detail::firstTypeMismatch(raw_events, kExpectedPattern, 1);
            if (next_valid_offset >= 0)
            {
                // notify(DecoderProtocolViolation::PartialVect_12_12_8);
                vect_base_seen_ = false;
                return false;
            }

            next_valid_offset = evt3_validation_detail::kVect12RawWords;

            if (!evt3_validation_detail::vectorWindowFits(vect_base_seen_, vect_base, width_))
            {
                vect_base_seen_ = false;
                // notify(DecoderProtocolViolation::InvalidVectBase);
                return false;
            }

            if (!addr_y_seen_)
            {
                // notify(DecoderProtocolViolation::MissingYAddr);
                return false;
            }

            return time_high_ok_;
        }

        bool validateContinue12By12By4PatternImpl(const Evt3Raw::RawEvent *raw_events, int &next_valid_offset)
        {
            constexpr std::array<Evt3EventTypes_4bits, 3> kExpectedPattern{
                Evt3EventTypes_4bits::CONTINUED_12,
                Evt3EventTypes_4bits::CONTINUED_12,
                Evt3EventTypes_4bits::CONTINUED_4,
            };

            next_valid_offset = evt3_validation_detail::firstTypeMismatch(raw_events, kExpectedPattern);
            if (next_valid_offset >= 0)
            {
                // notify(DecoderProtocolViolation::PartialContinued_12_12_4);
                return false;
            }

            next_valid_offset = evt3_validation_detail::kContinueRawWords;

            return time_high_ok_;
        }

        void validateTimeHighImpl(timestamp prev_time_high, timestamp time_high)
        {
            int64_t timehigh_delta = evt3_validation_detail::signedTimeHighDelta(prev_time_high, time_high);
            bool is_monotonic = 0 <= timehigh_delta;

            time_high_ok_ = is_monotonic || isLooseTimeHighOverflow(prev_time_high, time_high);

            if (isStrictTimeHighOverflow(prev_time_high, time_high))
            {
                return;
            }

            if (!time_high_ok_)
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
            switch (static_cast<Evt3EventTypes_4bits>(raw_event->type))
            {
            case Evt3EventTypes_4bits::EVT_ADDR_Y:
                addr_y_seen_ = true;
                break;
            case Evt3EventTypes_4bits::VECT_BASE_X:
                vect_base_seen_ = true;
                break;
            default:
                break;
            }
        }
    };

} // namespace fluxeem

#endif // EVT3_VALIDATOR_H__
