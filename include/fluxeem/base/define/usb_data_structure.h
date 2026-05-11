// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef USB_DATA_STRUCTURE_HPP_
#define USB_DATA_STRUCTURE_HPP_

#include <cstdint>
#include <vector>
#include <system_error>
#include <fluxeem/base/define/base_define.h>

#define USB_FAILURE_FLAG (0x80000000)
#define USB_WRITE_FLAG (0x40000000)

#define USB_UNKNOWN_CMD (0 | USB_FAILURE_FLAG)
#define PROP_RELEASE_VERSION (0x79)
#define PROP_SERIAL_NUMBER (0x72)
#define PROP_BUILD_DATE (0x7A)
#define PROP_DEVICES (0x10000)

#define PROP_DEVICE_REG32 (0x10102)

namespace fluxeem
{

    class FLUXEEM_API CtrlFrame
    {
    public:
        struct CtrlFrameHeader
        {
            uint32_t property;
            uint32_t size;
        };

        CtrlFrame(uint32_t property);

        virtual ~CtrlFrame() = default;

        virtual uint32_t getProperty();
        virtual void updateSize();
        virtual uint8_t *frame()
        {
            updateSize();
            return vect.data();
        }
        virtual std::size_t frameSize();
        virtual bool swapReqAndAnswer(std::vector<uint8_t> &x);
        // virtual void swap_and_check_answer(std::vector<uint8_t> &x);

        void pushBack32(const uint32_t val);
        void pushBack32(const std::vector<uint32_t> &vals);

        uint32_t get32(std::size_t index);
		uint64_t get64(std::size_t index);
        std::size_t payloadSize();
        uint8_t *payload();

    protected:
        std::vector<uint8_t> vect;

    private:
        // void update_size(void);
    };

    class StringsCtrlFrame : public CtrlFrame
    {
    public:
        StringsCtrlFrame(uint32_t property, uint32_t device);
        std::vector<std::string> getStrings();
        void pushBack(const std::string &);
    };

} // namespace fluxeem

#endif // USB_DATA_STRUCTURE_HPP_
