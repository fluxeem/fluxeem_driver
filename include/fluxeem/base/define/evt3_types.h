// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file evt3_types.h
 * @author Fluxeem
 * @since 1.0.0
 * @brief EVT3 原始事件流位域类型定义
 * @details 定义 EVT3 编码格式的位域结构体，包括事件类型、X/Y 坐标、
 *          向量事件、时间戳、外部触发和原始数据等。这些类型直接映射到
 *          传感器输出的 16 位字，采用位域以避免手动移位。
 * @ingroup fluxeem_event_types
 */
#ifndef __EVT3_TYPES_H__
#define __EVT3_TYPES_H__
namespace fluxeem {
	/// @brief EVT3 原始字内部的位域布局
	namespace Evt3Raw {

		/// @brief 向量有效位掩码（12+12+8 拆分）
		union Mask {
			uint32_t valid;
			struct Mask_Vect32 {
				uint32_t valid1 : 12; ///< 低 12 位有效位
				uint32_t valid2 : 12; ///< 中 12 位有效位
				uint32_t valid3 : 8;  ///< 高 8 位有效位
			} m;
		};

		/// @brief 4-bit 事件类型标记（含极性位）
		struct Evt3_Event_Type_4bits {
			uint16_t pad : 11;   ///< 保留位
			uint16_t p : 1;      ///< 极性
			uint16_t type : 4;   ///< 事件类型码
		};

		/// @brief 正极性 X 地址事件
		struct Event_PosX {
			uint16_t x : 11;     ///< X 坐标
			uint16_t pol : 1;    ///< 极性
			uint16_t type : 4;   ///< 事件类型码
		};

		/// @brief 向量事件（12+12+8 有效位布局）
		struct Event_Vect12_12_8 {
			uint16_t valid1 : 12;  ///< 第一段有效位
			uint16_t type1 : 4;    ///< 第一段类型码
			uint16_t valid2 : 12;  ///< 第二段有效位
			uint16_t type2 : 4;    ///< 第二段类型码
			uint16_t valid3 : 8;   ///< 第三段有效位
			uint16_t unused3 : 4;  ///< 保留
			uint16_t type3 : 4;    ///< 第三段类型码
		};

		/// @brief 续接向量事件（12+12+4 有效位布局）
		struct Event_Continue12_12_4 {
			uint16_t valid1 : 12;
			uint16_t type1 : 4;
			uint16_t valid2 : 12;
			uint16_t type2 : 4;
			uint16_t valid3 : 4;
			uint16_t unused3 : 8;
			uint16_t type3 : 4;
			/// @brief 将三段有效位拼接为 28 位无符号整数
			static uint64_t decode(const Event_Continue12_12_4& e) {
				return static_cast<uint64_t>(e.valid1) |
					static_cast<uint64_t>(e.valid2) << 12 |
					static_cast<uint64_t>(e.valid3) << 24;
			}
		};

		/// @brief X 基地址事件（向量起始 X）
		struct Event_XBase {
			uint16_t x : 11;     ///< X 基地址
			uint16_t pol : 1;    ///< 极性
			uint16_t type : 4;   ///< 事件类型码
		};

		/// @brief Y 地址事件
		struct Event_Y {
			uint16_t y : 11;     ///< Y 坐标
			uint16_t orig : 1;   ///< 原始标志
			uint16_t type : 4;   ///< 事件类型码
		};

		/// @brief 外部触发事件
		struct Event_ExtTrigger {
			uint16_t pol : 1;    ///< 触发极性
			uint16_t unused : 7; ///< 保留
			uint16_t id : 4;     ///< 触发通道 ID
			uint16_t type : 4;   ///< 事件类型码
		};

		/// @brief 通用原始 16 位事件（12 位内容 + 4 位类型）
		struct RawEvent {
			uint16_t content : 12; ///< 事件内容
			uint16_t type : 4;     ///< 事件类型码
		};

		/// @brief 时间戳事件（低 12 位或高 12 位）
		struct Event_Time {
			uint16_t time : 12;  ///< 12 位时间片
			uint16_t type : 4;   ///< 事件类型码
			/// @brief 解码时间高位：将 12 位写入时间戳的 [23:12]
			static size_t decode_time_high(const uint16_t* ev,
				Timestamp& cur_t) {
				const Event_Time* ev_timehigh =
					reinterpret_cast<const Event_Time*>(ev);
				cur_t =
					(cur_t & ~(0b111111111111ull << 12)) | (ev_timehigh->time << 12);
				return 1;
			}
		};
	}// namespace Evt3Raw

	/// @brief EVT3 4-bit 事件类型码枚举
	enum class Evt3EventTypes_4bits : uint8_t {
		EVT_ADDR_Y = 0x0,     ///< Y 地址事件
		EVT_ADDR_X = 0x2,     ///< X 地址事件
		VECT_BASE_X = 0x3,    ///< 向量基 X
		VECT_12 = 0x4,        ///< 12 位向量
		VECT_8 = 0x5,         ///< 8 位向量
		EVT_TIME_LOW = 0x6,   ///< 时间低位
		CONTINUED_4 = 0x7,    ///< 4 位续接
		EVT_TIME_HIGH = 0x8,  ///< 时间高位
		EXT_TRIGGER = 0xA,    ///< 外部触发
		UNUSED_1 = 0xB,      ///< 保留 1
		UNUSED_2 = 0xC,      ///< 保留 2
		IMU = 0xD,            ///< IMU 数据
		OTHERS = 0xE,         ///< 其他
		CONTINUED_12 = 0xF    ///< 12 位续接
	};

	/// @brief 通用 16 位原始数据（与 RawEvent 布局相同）
	struct RawData {
		uint16_t content : 12; ///< 数据内容
		uint16_t type : 4;     ///< 类型码
	};

}//namespace fluxeem


#endif// __EVT3_TYPES_H__
