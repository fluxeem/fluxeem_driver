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
	 * \~english @brief The base class of camera tools. Tools refer to the abstract concept used to control the camera, which can be used to set various camera parameters, obtain camera parameter information, perform data processing, etc.
	 * \~chinese @brief 相机工具的基类。工具指的是用来控制相机的抽象概念，可以用来设置各种相机参数、获取相机参数信息、进行数据处理等。
	 * @ingroup camera_tools
	 */
	class FLUXEEM_API CameraTool
	{
	public:
		CameraTool(const std::string& prefix = "") : prefix_(prefix) {}

		virtual ~CameraTool(){}

		/**
		 * \~english @brief Get the information of the tool.
		 * \~chinese @brief 获取工具的信息。
		 * 
		 * \~english @return The information of the tool.
		 * \~chinese @return 工具的信息。
		 */
		virtual const ToolInfo getToolInfo() = 0;

		/**
		 * \~english @brief Get the basic information of all parameters. If you want to get the detailed information of a parameter, please refer to @ref CameraTool::getParamInfo .
		 * \~chinese @brief 获取所有参数的基本信息。如果想要获取某个参数的详细信息，请参考 @ref CameraTool::getParamInfo 。
		 * 
		 * \~english @return The information of all parameters.
		 * \~chinese @return 所有参数的基本信息。
		 */
		virtual std::map<std::string, BasicParameterInfo> getAllParamInfo() = 0;

		/**
		 * \~english @brief Get the detailed information of a parameter.
		 * \~chinese @brief 获取一个参数的详细信息。
		 * 
		 * \~english @param name The name of the parameter.
		 * \~chinese @param name 参数的名称。
		 * \~english @param info The detailed information of the parameter.
		 * \~chinese @param info 参数的详细信息。
		 * \~english @return True if the parameter exists, otherwise false.
		 * \~chinese @return 如果参数存在，则返回true，否则返回false。
		 */
		virtual bool getParamInfo(const std::string name, IntParameterInfo &info) = 0;
		virtual bool getParamInfo(const std::string name, FloatParameterInfo& info) = 0;
		virtual bool getParamInfo(const std::string name, BoolParameterInfo& info) = 0;
		virtual bool getParamInfo(const std::string name, EnumParameterInfo& info) = 0;
		virtual bool getParamInfo(const std::string name, StringParameterInfo &info) = 0;
		/**
		 * \~english @brief Get the value of a parameter.
		 * \~chinese @brief 获取一个参数的值。
		 * 
		 * \~english @param name The name of the parameter.
		 * \~chinese @param name 参数的名称。
		 * \~english @param value The value of the parameter.
		 * \~chinese @param value 参数的值。
		 * \~english @return True if the parameter exists, otherwise false.
		 * \~chinese @return 如果参数存在，则返回true，否则返回false。
		 */
		virtual bool getParam(const std::string name, int& value);
		virtual bool getParam(const std::string name, float& value);
		virtual bool getParam(const std::string name, bool& value);
		virtual bool getParam(const std::string name, std::string& value);
		/**
		 * \~english @brief Set the value of a parameter.
		 * \~chinese @brief 设置一个参数的值。
		 * 
		 * \~english @param name The name of the parameter.
		 * \~chinese @param name 参数的名称。
		 * \~english @param value The value of the parameter.
		 * \~chinese @param value 参数的值。
		 * \~english @return True if the parameter exists, otherwise false.
		 * \~chinese @return 如果参数存在，则返回true，否则返回false。
		 */
		virtual bool setParam(const std::string name, const int& value);
		virtual bool setParam(const std::string name, const float& value);
		virtual bool setParam(const std::string name, const double& value);
		virtual bool setParam(const std::string name, const bool& value);
		virtual bool setParam(const std::string name, const char* value);
		virtual bool setParam(const std::string name, const std::string& value);

		const std::vector<ParamDescriptor>& descriptors() const;
		std::optional<ParamValue> get(std::string_view name) const;
		bool set(std::string_view name, const ParamValue& value);
		nlohmann::json toJson() const;
		bool fromJson(const nlohmann::json& j);

	protected:
		void registerParam(ParamDescriptor desc);
		virtual bool onParamChanged(uint8_t idx, const ParamValue& old_val, const ParamValue& new_val) { return true; }
		virtual bool onParamRead(uint8_t idx, ParamValue& out_val) const { return false; }
		std::optional<uint8_t> findParamIndex(std::string_view name) const;

		std::string prefix_ = "";

	private:
		std::vector<ParamDescriptor> descriptors_;
		std::vector<ParamValue> values_;
	};
} // namespace fluxeem

#endif // CAMERA_TOOL_BASE_HPP
