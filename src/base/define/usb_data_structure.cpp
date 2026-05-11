// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#include <fluxeem/base/define/usb_data_structure.h>
#include <fluxeem/base/logging/logger.h>
#include <cstring>

namespace fluxeem
{
    uint32_t CtrlFrame::get32(std::size_t index)
    {
        if (payloadSize() < ((index + 1) * (sizeof(uint32_t))))
            throw std::range_error("Ctrl frame vect index out of range");
        return *((uint32_t *)(payload() + (index * sizeof(uint32_t))));
    }

    uint64_t CtrlFrame::get64(std::size_t index)
    {
        if (payloadSize() < ((index + 1) * (sizeof(uint64_t))))
            throw std::range_error("Ctrl frame vect index out of range");
        return *((uint64_t *)(payload() + (index * sizeof(uint64_t))));
    }

    std::size_t CtrlFrame::payloadSize()
    {
        int32_t payloadSize = vect.size() - sizeof(CtrlFrameHeader);
        if (payloadSize < 0)
            throw std::length_error("payload resized to less than 0");
        return payloadSize;
    }

    uint8_t *CtrlFrame::payload()
    {
        return vect.data() + sizeof(CtrlFrameHeader);
    }

    CtrlFrame::CtrlFrame(uint32_t property)
    {
        struct CtrlFrameHeader *frame;
        vect.resize(8);
        frame = reinterpret_cast<CtrlFrameHeader *>(vect.data());
        frame->property = property;
        frame->size = 0;
    }

    uint32_t CtrlFrame::getProperty()
    {
        struct CtrlFrameHeader *frame;
        frame = reinterpret_cast<CtrlFrameHeader *>(vect.data());
        return frame->property;
    }

    void CtrlFrame::updateSize()
    {
        int32_t payloadSize = vect.size() - sizeof(CtrlFrameHeader);
        if (payloadSize < 0)
            throw std::length_error("payload resized to less than 0");
        struct CtrlFrameHeader *frame = reinterpret_cast<CtrlFrameHeader *>(vect.data());
        frame->size = payloadSize;
    }

    std::size_t CtrlFrame::frameSize()
    {
        return vect.size();
    }

    bool CtrlFrame::swapReqAndAnswer(std::vector<uint8_t> &x)
    {
        uint32_t req_property = getProperty();
        struct CtrlFrameHeader *frame;

        vect.swap(x);
        frame = reinterpret_cast<CtrlFrameHeader *>(vect.data());
        if (frame->size != (vect.size() - sizeof(CtrlFrameHeader)))
        {
            LOG_ERROR("size mismatch; expected %d, got %d", frame->size, vect.size() - sizeof(CtrlFrameHeader));
            return false;
        }

        if (frame->property == USB_UNKNOWN_CMD)
        {
            LOG_ERROR("command not implemented");
            return false;
        }

        if (frame->property == (req_property | USB_FAILURE_FLAG))
        {
            LOG_ERROR("command failed");
            return false;
        }

        if (frame->property != req_property)
        {
            LOG_ERROR("property mismatch; expected %d, got %d", req_property, frame->property);
            return false;
        }

        return true;
    }

    void CtrlFrame::pushBack32(const uint32_t val)
    {
        vect.push_back(val & 0xFF);
        vect.push_back((val >> 8) & 0xFF);
        vect.push_back((val >> 16) & 0xFF);
        vect.push_back((val >> 24) & 0xFF);
    }

    void CtrlFrame::pushBack32(const std::vector<uint32_t> &vals)
    {
        vect.reserve(vect.size() + (sizeof(uint32_t) * vals.size()));
        for (auto const &val : vals)
            pushBack32(val);
    }

    StringsCtrlFrame::StringsCtrlFrame(uint32_t property, uint32_t device) : CtrlFrame(property)
    {
        vect.push_back(device & 0xFF);
        vect.push_back((device >> 8) & 0xFF);
        vect.push_back((device >> 16) & 0xFF);
        vect.push_back((device >> 24) & 0xFF);
    }

    std::vector<std::string> StringsCtrlFrame::getStrings()
    {
        std::vector<std::string> res;

        uint8_t *frame = vect.data();
        std::size_t remaining = vect.size();
        frame += sizeof(CtrlFrameHeader);
        remaining -= sizeof(CtrlFrameHeader);

        if (remaining < sizeof(uint32_t))
            // TODO: throw std::system_error(TZ_TOO_SHORT, TzError());
            LOG_ERROR("TZ_TOO_SHORT");

        frame += sizeof(uint32_t);
        remaining -= sizeof(uint32_t);

        if (!remaining)
            // TODO: throw std::system_error(TZ_TOO_SHORT, TzError());
            LOG_ERROR("TZ_TOO_SHORT, not enough space for NULL terminator");
        if (*(frame + remaining - 1) != '\0')
            // TODO: throw std::system_error(TZ_INVALID_ANSWER, TzError(), "compatible string shall be NULL-terminated");
            LOG_ERROR("TZ_INVALID_ANSWER, compatible string shall be NULL-terminated");

        while (remaining)
        {
            std::string str((char *)frame);
            frame += str.size() + 1;
            remaining -= str.size() + 1;
            res.push_back(str);
        }

        return res;
    }

    void StringsCtrlFrame::pushBack(const std::string &str)
    {
        auto size = vect.size();
        vect.resize(vect.size() + str.size() + 1);
        memcpy(vect.data() + size, str.c_str(), str.size() + 1);
    }

} // namespace fluxeem
