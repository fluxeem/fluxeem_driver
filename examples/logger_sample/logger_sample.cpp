#include <fluxeem/base/logging/logger.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    // 测试不同的日志级别
    std::cout << "Testing logger levels..." << std::endl;
    
	LOGGER.setLogLevel(fluxeem::LogLevelType::LOG_DEBUG);
    std::cout << "Set log level to DEBUG" << std::endl;
    
    // 使用我们的logger单例，并初始化文件输出
	LOG_FATAL("Fatal message");
    LOG_ERROR("Error message");
    LOG_WARN("Warning message");
    LOG_INFO("Info message: Starting application");
    LOG_DEBUG("Debug message: initializing components");
    
    // 更多测试日志
    for (int i = 0; i < 2; ++i) {
        LOG_INFO("Test message %d", i);
        LOG_DEBUG("Debug info: iteration %d", i);
    }

    std::cout << "Logging example completed." << std::endl;

    return 0;
}
