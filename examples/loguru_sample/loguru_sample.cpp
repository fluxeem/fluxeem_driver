#include "loguru.hpp" 

int main(int argc, char *argv[])
{
	loguru::init(argc, argv);
	LOG_F(INFO, "Hello from sample!");
	return 0;
}