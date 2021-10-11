
#include <gtest/gtest.h>

#include <boost/signals2/signal.hpp>

#include <algorithm>
#include <vector>

TEST(signals2, SingleSlot)
{
	boost::signals2::signal<void()> sig;

	sig.connect([] { std::cout << "Hello, world!\n"; });
	sig();
}

TEST(signals2, MultiSlot)
{
	boost::signals2::signal<void()> sig;

	sig.connect([] { std::cout << "Hello"; });
	sig.connect([] { std::cout << ", world!\n"; });

	sig();
}

TEST(signals2, WithOrder)
{
	boost::signals2::signal<void()> sig;

	sig.connect(1, [] { std::cout << ", world!\n"; });
	sig.connect(0, [] { std::cout << "Hello"; });

	sig();
}

void hello() { std::cout << "Hello"; }
void world() { std::cout << ", world!\n"; }

TEST(signals2, Disconnect)
{
	boost::signals2::signal<void()> sig;
	sig.connect(hello);
	sig.connect(world);

	EXPECT_EQ(2, sig.num_slots());

	sig.disconnect(world);

	EXPECT_EQ(1, sig.num_slots());

	sig();
}

TEST(signals2, ReturnValue)
{
	boost::signals2::signal<int()> sig;

	sig.connect([] { return 1; });
	sig.connect([] { return 2; });

	boost::optional<int> &opt 
		= sig();
	if (opt.has_value()) {
		EXPECT_EQ(2, opt.get());
	}
}

template <typename T>
struct min_element
{
	typedef T result_type;

	template <typename InputIterator>
	T operator()(InputIterator first, InputIterator last) const
	{
		std::vector<T> v(first, last);
		return *std::min_element(v.begin(), v.end());
	}
};

TEST(signals2, SelectReturnValue)
{
	boost::signals2::signal<int(), min_element<int>> sig;

	sig.connect([] { return 1; });
	sig.connect([] { return 2; });
	EXPECT_EQ(1, sig());
}

template <typename T>
struct return_all
{
	typedef T result_type;

	template <typename InputIterator>
	T operator()(InputIterator first, InputIterator last) const
	{
		return T(first, last);
	}
};

TEST(signals2, ReturnAll)
{
	boost::signals2::signal<int(), return_all<std::vector<int>>> sig;

	sig.connect([] { return 1; });
	sig.connect([] { return 2; });
	std::vector<int> v = sig();
	EXPECT_EQ(2, v.size());
	EXPECT_EQ(1, *std::min_element(v.begin(), v.end()));
}

