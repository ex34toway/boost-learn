
#include <gtest/gtest.h>

#include <iostream>

#include <boost/noncopyable.hpp>
#include <boost/aligned_storage.hpp>

#include <boost/asio.hpp>

using namespace boost::asio;

class handler_memory
{
public:
    handler_memory()
        : in_use_(false)
    {
    }

    handler_memory(const handler_memory&) = delete;
    handler_memory& operator=(const handler_memory&) = delete;

    void* allocate(std::size_t size)
    {
        if (!in_use_ && size < sizeof(storage_))
        {
            in_use_ = true;
            return &storage_;
        }
        else
        {
            return ::operator new(size);
        }
    }

    void deallocate(void* pointer)
    {
        if (pointer == &storage_)
        {
            in_use_ = false;
        }
        else
        {
            ::operator delete(pointer);
        }
    }

private:
    // Storage space used for handler-based custom memory allocation.
    typename std::aligned_storage<1024>::type storage_;

    // Whether the handler-based custom allocation storage has been used.
    bool in_use_;
};


// The allocator to be associated with the handler objects. This allocator only
// needs to satisfy the C++11 minimal allocator requirements.
template <typename T>
class handler_allocator
{
public:
    using value_type = T;

    explicit handler_allocator(handler_memory& mem)
        : memory_(mem)
    {
    }

    template <typename U>
    handler_allocator(const handler_allocator<U>& other) noexcept
        : memory_(other.memory_)
    {
    }

    bool operator==(const handler_allocator& other) const noexcept
    {
        return &memory_ == &other.memory_;
    }

    bool operator!=(const handler_allocator& other) const noexcept
    {
        return &memory_ != &other.memory_;
    }

    T* allocate(std::size_t n) const
    {
        return static_cast<T*>(memory_.allocate(sizeof(T) * n));
    }

    void deallocate(T* p, std::size_t /*n*/) const
    {
        return memory_.deallocate(p);
    }

private:
    template <typename> friend class handler_allocator;

    // The underlying memory.
    handler_memory& memory_;
};

template <typename Handler>
class custom_alloc_handler
{
public:
    using allocator_type = handler_allocator<Handler>;

    explicit custom_alloc_handler(handler_memory& m, Handler h) 
        : memory_(m), handler_(h)
    {
    }

    allocator_type get_allocator() const noexcept
    {
        return allocator_type(memory_);
    }

    template <typename ...Args>
    void operator()(Args&&... args)
    {
        handler_(std::forward<Args>(args)...);
    }

private:
    handler_memory& memory_;
    Handler handler_;
};

// Helper
template <typename Handler>
inline custom_alloc_handler<Handler> make_custom_alloc_handler(
    handler_memory& m, Handler h)
{
    return custom_alloc_handler<Handler>(m, h);
}

class session
    : public std::enable_shared_from_this<session>
{

public:
    session(ip::tcp::socket socket)
        : socket_(std::move(socket))
    {
    }

    void start()
    {
        do_read();
    }

    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_),
            make_custom_alloc_handler(handler_memory_,
                [this, self](boost::system::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        do_write(length);
                    }
                }));
    }

    void do_write(std::size_t length)
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
            make_custom_alloc_handler(handler_memory_,
                [this, self](boost::system::error_code ec, std::size_t /*length*/)
                {
                    if (!ec)
                    {
                        do_read();
                    }
                }));
    }
private:
    // The socket used to communicate with the client.
    ip::tcp::socket socket_;

    // Buffer used to store data received from the client.
    std::array<char, 1024> data_;

    // The memory to use for handler-based custom memory allocation.
    handler_memory handler_memory_;
};

class server
{
public:
    server(boost::asio::io_service& io_service, short port)
        : acceptor_(io_service, ip::tcp::endpoint(ip::tcp::v4(), port))
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, ip::tcp::socket socket)
            {
                if (!ec)
                {
                    std::make_shared<session>(std::move(socket))->start();
                }

                do_accept();
            });
    }

    ip::tcp::acceptor acceptor_;
};


TEST(asio, Allocator)
{
    boost::asio::io_service io_service;
    server s(io_service, 8080);
    io_service.run();
}


class shared_const_buffer
{
public:
    // Construct from a std::string.
    explicit shared_const_buffer(const std::string& data)
        : data_(new std::vector<char>(data.begin(), data.end())),
        buffer_(boost::asio::buffer(*data_))
    {
    }

    // Implement the ConstBufferSequence requirements.
    typedef boost::asio::const_buffer value_type;
    typedef const boost::asio::const_buffer* const_iterator;
    const const_iterator begin() const { return &buffer_; }
    const const_iterator end() const { return &buffer_ + 1; }

private:
    std::shared_ptr<std::vector<char> > data_;
    boost::asio::const_buffer buffer_;
};


TEST(asio, Buffer)
{
    class session
        : public std::enable_shared_from_this<session>
    {
    public:
        session(boost::asio::ip::tcp::socket socket)
            : socket_(std::move(socket))
        {
        }

        void start()
        {
            do_write();
        }

    private:
        void do_write()
        {
            std::time_t now = std::time(0);
            shared_const_buffer buffer(std::ctime(&now));

            auto self(shared_from_this());
            boost::asio::async_write(socket_, buffer,
                [self](boost::system::error_code /*ec*/, std::size_t /*length*/)
                {
                });
        }

        boost::asio::ip::tcp::socket socket_;
    };

    class server
    {
    public:
        server(boost::asio::io_context& io_context, short port)
            : acceptor_(io_context, 
                boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
        {
            do_accept();
        }

    private:
        void do_accept()
        {
            acceptor_.async_accept(
                [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
                {
                    if (!ec)
                    {
                        std::make_shared<session>(std::move(socket))->start();
                    }

                    do_accept();
                });
        }

        boost::asio::ip::tcp::acceptor acceptor_;
    };

    boost::asio::io_service io_service;
    server s(io_service, 8080);
    io_service.run();
}
