// SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
// SPDX-License-Identifier: Apache-2.0
#ifndef CAMERA_TOOL_BASE_HPP
#define CAMERA_TOOL_BASE_HPP

#include <string>
#include <map>
#include <optional>
#include <string_view>
#include <vector>
#include <fluxeem/base/utility/json/json.hpp>
#include <fluxeem/hal/tools/tool_info.h>
#include <fluxeem/hal/tools/param_descriptor.h>

namespace fluxeem
{

	/**
	 * \~english @brief Abstract base for camera configuration tools.
	 *                  A "tool" represents a logical control unit on the camera —
	 *                  bias adjustment, trigger configuration, region-of-interest,
	 *                  anti-flicker filter, etc. Each tool exposes a set of named
	 *                  parameters that can be queried or modified at runtime.
	 * \~chinese @brief 相机配置工具的抽象基类。
	 *                  "工具"代表相机上的一个逻辑控制单元——
	 *                  偏置调整、触发配置、感兴趣区域、抗闪烁滤波等。
	 *                  每个工具暴露一组命名参数，可在运行时查询或修改。
	 * @ingroup fluxeem_camera_tools
	 */
	class FLUXEEM_API CameraTool
	{
	public:
		/**
		 * \~english @brief Construct a tool with an optional register-name prefix.
		 * \~chinese @brief 构造工具，可指定寄存器名称前缀。
		 * \~english @param prefix Prefix string prepended to parameter names during HW access.
		 * \~chinese @param prefix 硬件访问时附加在参数名前的前缀字符串。
		 */
		CameraTool(const std::string& prefix = "") : prefix_(prefix) {}

		/**
		 * \~english @brief Virtual destructor.
		 * \~chinese @brief 虚析构函数。
		 */
		virtual ~CameraTool(){}

		/**
		 * \~english @brief Query the static metadata of this tool.
		 * \~chinese @brief 查询本工具的静态元数据。
		 * \~english @return A ToolInfo struct describing the tool's type, name and parameters.
		 * \~chinese @return 描述工具类型、名称和参数的 ToolInfo 结构体。
		 */
		virtual const ToolInfo getToolInfo() = 0;

		/**
		 * \~english @brief List basic metadata for every parameter owned by this tool.
		 *                  For full parameter details, see @ref CameraTool::getParamInfo.
		 * \~chinese @brief 列出本工具所有参数的基本元数据。
		 *                  如需获取参数的完整描述，参见 @ref CameraTool::getParamInfo。
		 * \~english @return A map from parameter name to its BasicParameterInfo.
		 * \~chinese @return 参数名到 BasicParameterInfo 的映射。
		 */
		virtual std::map<std::string, BasicParameterInfo> getAllParamInfo() = 0;

		/**
		 * \~english @brief Retrieve the full descriptor for a named parameter.
		 * \~chinese @brief 获取指定参数的完整描述信息。
		 *
		 * \~english @param name Parameter identifier (case-sensitive).
		 * \~chinese @param name 参数标识符（区分大小写）。
		 * \~english @param info [out] Receives the parameter descriptor on success.
		 * \~chinese @param info [out] 成功时接收参数描述符。
		 * \~english @return true if the parameter exists and its info was written; false otherwise.
		 * \~chinese @return 参数存在且信息已写入返回 true，否则返回 false。
		 */
		virtual bool getParamInfo(const std::string name, IntParameterInfo &info) = 0;
		virtual bool getParamInfo(const std::string name, FloatParameterInfo& info) = 0;
		virtual bool getParamInfo(const std::string name, BoolParameterInfo& info) = 0;
		virtual bool getParamInfo(const std::string name, EnumParameterInfo& info) = 0;
		virtual bool getParamInfo(const std::string name, StringParameterInfo &info) = 0;

		/**
		 * \~english @brief Read the current value of a named parameter from the camera.
		 * \~chinese @brief 从相机读取指定参数的当前值。
		 *
		 * \~english @param name Parameter identifier.
		 * \~chinese @param name 参数标识符。
		 * \~english @param value [out] Receives the parameter value on success.
		 * \~chinese @param value [out] 成功时接收参数值。
		 * \~english @return true if the parameter exists and its value was read; false otherwise.
		 * \~chinese @return 参数存在且值已读取返回 true，否则返回 false。
		 */
		virtual bool getParam(const std::string name, int& value);
		virtual bool getParam(const std::string name, float& value);
		virtual bool getParam(const std::string name, bool& value);
		virtual bool getParam(const std::string name, std::string& value);

		/**
		 * \~english @brief Write a new value to a named parameter on the camera.
		 * \~chinese @brief 向相机写入指定参数的新值。
		 *
		 * \~english @param name Parameter identifier.
		 * \~chinese @param name 参数标识符。
		 * \~english @param value The value to apply.
		 * \~chinese @param value 待写入的值。
		 * \~english @return true if the parameter exists and the value was applied; false otherwise.
		 * \~chinese @return 参数存在且值已应用返回 true，否则返回 false。
		 */
		virtual bool setParam(const std::string name, const int& value);
		virtual bool setParam(const std::string name, const float& value);
		virtual bool setParam(const std::string name, const double& value);
		virtual bool setParam(const std::string name, const bool& value);
		virtual bool setParam(const std::string name, const char* value);
		virtual bool setParam(const std::string name, const std::string& value);

		/**
		 * \~english @brief Return the read-only list of all registered parameter descriptors.
		 * \~chinese @brief 返回所有已注册参数描述符的只读列表。
		 */
		const std::vector<ParamDescriptor>& descriptors() const;

		/**
		 * \~english @brief Look up a parameter value by name using the descriptor-based interface.
		 * \~chinese @brief 通过描述符接口按名称查找参数值。
		 * \~english @param name Parameter name.
		 * \~chinese @param name 参数名称。
		 * \~english @return The parameter value if found, std::nullopt otherwise.
		 * \~chinese @return 找到则返回参数值，否则返回 std::nullopt。
		 */
		std::optional<ParamValue> get(std::string_view name) const;

		/**
		 * \~english @brief Update a parameter value by name using the descriptor-based interface.
		 * \~chinese @brief 通过描述符接口按名称更新参数值。
		 * \~english @param name Parameter name.
		 * \~chinese @param name 参数名称。
		 * \~english @param value The new value wrapped in ParamValue.
		 * \~chinese @param value 包装在 ParamValue 中的新值。
		 * \~english @return true on success, false if the parameter was not found.
		 * \~chinese @return 成功返回 true，参数未找到返回 false。
		 */
		bool set(std::string_view name, const ParamValue& value);

		/**
		 * \~english @brief Serialize all parameter names and values to a JSON object.
		 * \~chinese @brief 将所有参数名称和值序列化为 JSON 对象。
		 */
		nlohmann::json toJson() const;

		/**
		 * \~english @brief Deserialize parameter values from a JSON object, applying each value through @ref set.
		 * \~chinese @brief 从 JSON 对象反序列化参数值，依次通过 @ref set 应用。
		 * \~english @param j JSON object containing parameter key-value pairs.
		 * \~chinese @param j 包含参数键值对的 JSON 对象。
		 * \~english @return true if all parameters were applied successfully, false on any failure.
		 * \~chinese @return 所有参数均成功应用返回 true，任一失败返回 false。
		 */
		bool fromJson(const nlohmann::json& j);

	protected:
		/**
		 * \~english @brief Register a parameter descriptor. Called by subclasses during initialization.
		 * \~chinese @brief 注册参数描述符。子类在初始化期间调用。
		 * \~english @param desc The descriptor to register.
		 * \~chinese @param desc 待注册的描述符。
		 */
		void registerParam(ParamDescriptor desc);

		/**
		 * \~english @brief Hook called after a parameter value has been changed via @ref set.
		 *                  Subclasses may override to perform hardware writes or side effects.
		 * \~chinese @brief 通过 @ref set 修改参数值后的回调钩子。
		 *                  子类可覆写以执行硬件写入或副作用操作。
		 * \~english @param idx Index of the changed parameter in the descriptor list.
		 * \~chinese @param idx 被修改参数在描述符列表中的索引。
		 * \~english @param old_val The previous value.
		 * \~chinese @param old_val 修改前的值。
		 * \~english @param new_val The newly assigned value.
		 * \~chinese @param new_val 新赋的值。
		 * \~english @return true to accept the change, false to reject and roll back.
		 * \~chinese @return 接受修改返回 true，拒绝并回滚返回 false。
		 */
		virtual bool onParamChanged(uint8_t idx, const ParamValue& old_val, const ParamValue& new_val) { return true; }

		/**
		 * \~english @brief Hook called when a parameter value is read via @ref get.
		 *                  Subclasses may override to fetch the latest value from hardware.
		 * \~chinese @brief 通过 @ref get 读取参数值时的回调钩子。
		 *                  子类可覆写以从硬件获取最新值。
		 * \~english @param idx Index of the requested parameter.
		 * \~chinese @param idx 请求参数的索引。
		 * \~english @param out_val [out] Receives the value read from hardware.
		 * \~chinese @param out_val [out] 接收从硬件读取的值。
		 * \~english @return true if the read succeeded, false to fall back to the cached value.
		 * \~chinese @return 读取成功返回 true，失败则回退到缓存值并返回 false。
		 */
		virtual bool onParamRead(uint8_t idx, ParamValue& out_val) const { return false; }

		/**
		 * \~english @brief Look up the internal index of a parameter by name.
		 * \~chinese @brief 按名称查找参数的内部索引。
		 */
		std::optional<uint8_t> findParamIndex(std::string_view name) const;

		/// \~english @brief Register-name prefix used for hardware access.
		/// \~chinese @brief 硬件访问时使用的寄存器名称前缀。
		std::string prefix_ = "";

	private:
		/// \~english @brief Ordered list of parameter descriptors.
		/// \~chinese @brief 有序参数描述符列表。
		std::vector<ParamDescriptor> descriptors_;

		/// \~english @brief Cached parameter values, parallel to descriptors_.
		/// \~chinese @brief 缓存的参数值，与 descriptors_ 一一对应。
		std::vector<ParamValue> values_;
	};
} // namespace fluxeem

#endif // CAMERA_TOOL_BASE_HPP
