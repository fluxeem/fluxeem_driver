// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef IMX636_REGISTER_OPERATION_SEQUENCE_HH
#define IMX636_REGISTER_OPERATION_SEQUENCE_HH

// WARNING, AUTO-GENERATED FILE WAS EDITED TO ADD WRITEFIELD OPERATIONS

#include <vector>

namespace fluxeem {

static const std::vector<RegisterOperation> issd_evk3_imx636_init = {
    RegisterOperation::Write(0x0000001C, 0x00000001),
    RegisterOperation::Delay(1000000),
    RegisterOperation::Write(0x00400004, 0x00000001),
    RegisterOperation::Delay(500000),
    RegisterOperation::Write(0x00400004, 0x00000000),
    RegisterOperation::Delay(1000000),
    RegisterOperation::Write(0x0000B000, 0x00000158),
    RegisterOperation::Delay(300),
    RegisterOperation::Write(0x0000B044, 0x00000000),
    RegisterOperation::Write(0x0000B004, 0x0000000A),
    RegisterOperation::Write(0x0000B040, 0x00000000),
    RegisterOperation::Write(0x0000B0C8, 0x00000000),
    RegisterOperation::Write(0x0000B040, 0x00000000),
    RegisterOperation::Write(0x0000B040, 0x00000000),
    RegisterOperation::Write(0x00000000, 0x4F006442),
    RegisterOperation::Write(0x00000000, 0x0F006442),
    RegisterOperation::Write(0x000000B8, 0x00000400),
    RegisterOperation::Write(0x000000B8, 0x00000400),
    RegisterOperation::Write(0x0000B07C, 0x00000000),
    RegisterOperation::Write(0x0000B074, 0x00000002),
    RegisterOperation::Write(0x0000B078, 0x000000A0),
    RegisterOperation::Write(0x000000C0, 0x00000110),
    RegisterOperation::Write(0x000000C0, 0x00000210),
    RegisterOperation::Write(0x0000B120, 0x00000001),
    RegisterOperation::Write(0x0000E120, 0x00000000),
    RegisterOperation::Write(0x0000B068, 0x00000004),
    RegisterOperation::Write(0x0000B07C, 0x00000001),
    RegisterOperation::Delay(10),
    RegisterOperation::Write(0x0000B07C, 0x00000003),
    RegisterOperation::Delay(1000),
    RegisterOperation::Write(0x000000B8, 0x00000401),
    RegisterOperation::Write(0x000000B8, 0x00000409),
    RegisterOperation::Write(0x00000000, 0x4F006442),
    RegisterOperation::Write(0x00000000, 0x4F00644A),
    RegisterOperation::Write(0x0000B080, 0x00000077),
    RegisterOperation::Write(0x0000B084, 0x0000000F),
    RegisterOperation::Write(0x0000B088, 0x00000037),
    RegisterOperation::Write(0x0000B08C, 0x00000037),
    RegisterOperation::Write(0x0000B090, 0x000000DF),
    RegisterOperation::Write(0x0000B094, 0x00000057),
    RegisterOperation::Write(0x0000B098, 0x00000037),
    RegisterOperation::Write(0x0000B09C, 0x00000067),
    RegisterOperation::Write(0x0000B0A0, 0x00000037),
    RegisterOperation::Write(0x0000B0A4, 0x0000002F),
    RegisterOperation::Write(0x0000B0AC, 0x00000028),
    RegisterOperation::Write(0x0000B0CC, 0x00000001),
    RegisterOperation::Write(0x0000B000, 0x000002F8),
    RegisterOperation::Write(0x0000B004, 0x0000008A),
    RegisterOperation::Write(0x0000B01C, 0x00000030),
    RegisterOperation::Write(0x0000B020, 0x00002000),
    RegisterOperation::Write(0x0000B02C, 0x000000FF),
    RegisterOperation::Write(0x0000B030, 0x00003E80),
    RegisterOperation::Write(0x0000B028, 0x00000FA0),
    RegisterOperation::Write(0x0000A000, 0x000B0501),
    RegisterOperation::Delay(200),
    RegisterOperation::Write(0x0000A008, 0x00002405),
    RegisterOperation::Delay(200),
    RegisterOperation::Write(0x0000A004, 0x000B0501),
    RegisterOperation::Delay(200),
    RegisterOperation::Write(0x0000A020, 0x00000150),
    RegisterOperation::Delay(200),
    RegisterOperation::Write(0x0000B040, 0x00000007),
    RegisterOperation::Write(0x0000B064, 0x00000006),
    RegisterOperation::Write(0x0000B040, 0x0000000F),
    RegisterOperation::Delay(100),
    RegisterOperation::Write(0x0000B004, 0x0000008A),
    RegisterOperation::Delay(200),
    RegisterOperation::Write(0x0000B0C8, 0x00000003),
    RegisterOperation::Delay(200),
    RegisterOperation::Write(0x0000B044, 0x00000001),
    RegisterOperation::Write(0x0000B000, 0x000002F9),
    RegisterOperation::Write(0x00007008, 0x00000001),
    RegisterOperation::Write(0x00007000, 0x00070001),
    RegisterOperation::Write(0x00008000, 0x0001E085),
    RegisterOperation::Write(0x00009008, 0x00000644),
    RegisterOperation::Write(0x00000004, 0xF0005042),
    RegisterOperation::Write(0x00000018, 0x00000200),
    RegisterOperation::Write(0x00001014, 0x11A1504D),
    RegisterOperation::Write(0x00009004, 0x00000000),
    RegisterOperation::Delay(1000),
    RegisterOperation::Write(0x00009000, 0x00000200)
};

static const std::vector<RegisterOperation> issd_evk3_imx636_start = {
    RegisterOperation::Write(0x0000B000, 0x000002F9),
    RegisterOperation::Write(0x00009028, 0x00000000),
    RegisterOperation::WriteField(0x00009008, 0x645, 0x00000001),

    // Analog START
    RegisterOperation::Write(0x0000002C, 0x0022C724),
    RegisterOperation::WriteField(0x00000004, 0xF0005442, 0x00000400)
};

static const std::vector<RegisterOperation> issd_evk3_imx636_stop = {
    // Analog STOP
    RegisterOperation::WriteField(0x00000004, 0xF0005042, 0x00000400),
    RegisterOperation::Write(0x0000002C, 0x0022C324),
    // Digital STOP
    RegisterOperation::Write(0x00009028, 0x00000002),
    RegisterOperation::Delay(1000),
    RegisterOperation::WriteField(0x00009008, 0x00000644, 0x00000001),
    RegisterOperation::Write(0x0000B000, 0x000002F8),
    RegisterOperation::Delay(300)
};

static const std::vector<RegisterOperation> issd_evk3_imx636_destroy = {
    // Analog DESTROY
    RegisterOperation::Write(0x00000070, 0x00400008),
    RegisterOperation::Write(0x0000006C, 0x0EE47114),
    RegisterOperation::Delay(500),
    RegisterOperation::Write(0x0000A00C, 0x00020400),
    RegisterOperation::Delay(500),
    RegisterOperation::Write(0x0000A010, 0x00008068),
    RegisterOperation::Delay(200),
    RegisterOperation::Write(0x00001104, 0x00000000),
    RegisterOperation::Delay(200),
    RegisterOperation::Write(0x0000A020, 0x00000050),
    RegisterOperation::Delay(200),
    RegisterOperation::Write(0x0000A004, 0x000B0500),
    RegisterOperation::Delay(200),
    RegisterOperation::Write(0x0000A008, 0x00002404),
    RegisterOperation::Delay(200),
    RegisterOperation::Write(0x0000A000, 0x000B0500),
    // Digital DESTROY
    RegisterOperation::Write(0x0000B044, 0x00000000),
    RegisterOperation::Write(0x0000B004, 0x0000000A),
    RegisterOperation::Write(0x0000B040, 0x0000000E),
    RegisterOperation::Write(0x0000B0C8, 0x00000000),
    RegisterOperation::Write(0x0000B040, 0x00000006),
    RegisterOperation::Write(0x0000B040, 0x00000004),
    RegisterOperation::Write(0x00000000, 0x4F006442),
    RegisterOperation::Write(0x00000000, 0x0F006442),
    RegisterOperation::Write(0x000000B8, 0x00000401),
    RegisterOperation::Write(0x000000B8, 0x00000400),
    RegisterOperation::Write(0x0000B07C, 0x00000000)
};

} // namespace fluxeem

#endif // REGISTER_OPERATION_HH
