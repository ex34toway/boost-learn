
#include <gtest/gtest.h>

#include <functional>

void f(int x, int y, int z)
{
	std::cout << x << " "  << y << " " << z << std::endl;
}

TEST(std, BindPlaceholders)
{
	// Placeholders 从非默认参数开始 index
	auto func = std::bind(f, 1, std::placeholders::_1, std::placeholders::_2);
	func(2, 3);
}