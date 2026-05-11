#ifndef __EVT3_TYPES_H__
#define __EVT3_TYPES_H__
namespace fluxeem {
	namespace Evt3Raw {

		union Mask {
			uint32_t valid;
			struct Mask_Vect32 {
				uint32_t valid1 : 12;
				uint32_t valid2 : 12;
				uint32_t valid3 : 8;
			} m;
		};

		struct Evt3_Event_Type_4bits {
			uint16_t pad : 11;
			uint16_t p : 1;
			uint16_t type : 4;
		};

		struct Event_PosX {
			uint16_t x : 11;
			uint16_t pol : 1;
			uint16_t type : 4;
		};

		struct Event_Vect12_12_8 {
			uint16_t valid1 : 12;
			uint16_t type1 : 4;
			uint16_t valid2 : 12;
			uint16_t type2 : 4;
			uint16_t valid3 : 8;
			uint16_t unused3 : 4;
			uint16_t type3 : 4;
		};

		struct Event_Continue12_12_4 {
			uint16_t valid1 : 12;
			uint16_t type1 : 4;
			uint16_t valid2 : 12;
			uint16_t type2 : 4;
			uint16_t valid3 : 4;
			uint16_t unused3 : 8;
			uint16_t type3 : 4;
			static uint64_t decode(const Event_Continue12_12_4& e) {
				return static_cast<uint64_t>(e.valid1) |
					static_cast<uint64_t>(e.valid2) << 12 |
					static_cast<uint64_t>(e.valid3) << 24;
			}
		};

		struct Event_XBase {
			uint16_t x : 11;
			uint16_t pol : 1;
			uint16_t type : 4;
		};

		struct Event_Y {
			uint16_t y : 11;
			uint16_t orig : 1;
			uint16_t type : 4;
		};

		struct Event_ExtTrigger {
			uint16_t pol : 1;
			uint16_t unused : 7;
			uint16_t id : 4;
			uint16_t type : 4;
		};

		struct RawEvent {
			uint16_t content : 12;
			uint16_t type : 4;
		};

		struct Event_Time {
			uint16_t time : 12;
			uint16_t type : 4;
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

	enum class Evt3EventTypes_4bits : uint8_t {
		EVT_ADDR_Y = 0x0,
		EVT_ADDR_X = 0x2,
		VECT_BASE_X = 0x3,
		VECT_12 = 0x4,
		VECT_8 = 0x5,
		EVT_TIME_LOW = 0x6,
		CONTINUED_4 = 0x7,
		EVT_TIME_HIGH = 0x8,
		EXT_TRIGGER = 0xA,
		UNUSED_1 = 0xB,
		UNUSED_2 = 0xC,
		IMU = 0xD,
		OTHERS = 0xE,
		CONTINUED_12 = 0xF
	};

	struct RawData {
		uint16_t content : 12;
		uint16_t type : 4;
	};

}//namespace fluxeem


#endif// __EVT3_TYPES_H__