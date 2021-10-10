
#include <gtest/gtest.h>

#define BOOST_ASIO_ENABLE_HANDLER_TRACKING

#include <boost/asio.hpp>

using boost::asio::ip::udp;

class async_udp_server
{
public:
    async_udp_server(boost::asio::io_context& io_context, short port)
        : socket_(io_context, udp::endpoint(udp::v4(), port))
    {
        do_receive();
    }

    void do_receive()
    {
        socket_.async_receive_from(
            boost::asio::buffer(data_, max_length), sender_endpoint_,
            [this](boost::system::error_code ec, std::size_t bytes_recvd)
            {
                if (!ec && bytes_recvd > 0)
                {
                    do_send(bytes_recvd);
                }
                else
                {
                    do_receive();
                }
            });
    }

    void do_send(std::size_t length)
    {
        socket_.async_send_to(
            boost::asio::buffer(data_, length), sender_endpoint_,
            [this](boost::system::error_code /*ec*/, std::size_t /*bytes_sent*/)
            {
                do_receive();
            });
    }

private:
    udp::socket socket_;
    udp::endpoint sender_endpoint_;
    enum { max_length = 1024 };
    char data_[max_length];
};

TEST(asio, AsyncUdpServer)
{
    try
    {
        boost::asio::io_context io_context(8);
        async_udp_server s(io_context, 514U);
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

enum { max_length = 1024 };

void blocking_udp_server(boost::asio::io_context& io_context, unsigned short port)
{
    udp::socket sock(io_context, udp::endpoint(udp::v4(), port));
    for (;;)
    {
        char data[max_length];
        udp::endpoint sender_endpoint;
        size_t length = sock.receive_from(
            boost::asio::buffer(data, max_length), sender_endpoint);
        sock.send_to(boost::asio::buffer(data, length), sender_endpoint);
    }
}

TEST(asio, BlockingUdpServer)
{
    try
    {
        boost::asio::io_context io_context;
        blocking_udp_server(io_context, 514U);
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
