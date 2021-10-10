
#include <gtest/gtest.h>

#include <boost/signals2/signal.hpp>

struct HelloWorld
{
	void operator()() const
	{
		std::cout << "Hello, World!" << std::endl;
	}
};

TEST(Signals2, SingleSlot)
{
	boost::signals2::signal<void()> sig;

	HelloWorld hello;
	sig.connect(hello);

	sig();
}

struct Hello
{
	void operator()() const
	{
		std::cout << "Hello";
	}
};


struct World
{
	void operator()() const
	{
		std::cout << ", World!" << std::endl;
	}
};

TEST(Signals2, MultiSlot)
{
	boost::signals2::signal<void()> sig;

	sig.connect(Hello());
	sig.connect(World());

	sig();
}
