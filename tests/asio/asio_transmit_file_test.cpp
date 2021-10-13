
#include <gtest/gtest.h>

#define UNICODE

#include <ctime>
#include <iostream>
#include <string>

#include <boost/asio.hpp>

#include <functional>
#include <memory>

#if BOOST_VERSION >= 107000
#define GET_IO_SERVICE(s) ((boost::asio::io_context&)(s).get_executor().context())
#else
#define GET_IO_SERVICE(s) ((s).get_io_service())
#endif

#if defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)

using boost::asio::ip::tcp;
using boost::asio::windows::overlapped_ptr;
using boost::asio::windows::random_access_handle;

// A wrapper for the TransmitFile overlapped I/O operation.
template <typename Handler>
void transmit_file(tcp::socket& socket,
    random_access_handle& file, Handler handler)
{
  // Construct an OVERLAPPED-derived object to contain the handler.
  overlapped_ptr overlapped(GET_IO_SERVICE(socket), handler);

  // Initiate the TransmitFile operation.
  BOOL ok = ::TransmitFile(socket.native_handle(),
      file.native_handle(), 0, 0, overlapped.get(), 0, 0);
  DWORD last_error = ::GetLastError();

  // Check if the operation completed immediately.
  if (!ok && last_error != ERROR_IO_PENDING)
  {
    // The operation completed immediately, so a completion notification needs
    // to be posted. When complete() is called, ownership of the OVERLAPPED-
    // derived object passes to the io_service.
    boost::system::error_code ec(last_error,
        boost::asio::error::get_system_category());
    overlapped.complete(ec, 0);
  }
  else
  {
    // The operation was successfully initiated, so ownership of the
    // OVERLAPPED-derived object has passed to the io_service.
    overlapped.release();
  }
}

class connection
  : public std::enable_shared_from_this<connection>
{
public:
  typedef std::shared_ptr<connection> pointer;

  static pointer create(boost::asio::io_service& io_service,
      const std::wstring& filename)
  {
    return std::make_shared<connection>(io_service, filename);
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    boost::system::error_code ec;
    file_.assign(::CreateFile(filename_.c_str(), GENERIC_READ, 0, 0,
          OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0), ec);
    if (file_.is_open())
    {
      transmit_file(socket_, file_,
          std::bind(&connection::handle_write, shared_from_this(),
            std::placeholders::_1,
              std::placeholders::_2));
    }
  }

public:
  connection(boost::asio::io_service& io_service, const std::wstring& filename)
    : socket_(io_service),
      filename_(filename),
      file_(io_service)
  {
  }

  void handle_write(const boost::system::error_code& /*error*/,
      size_t /*bytes_transferred*/)
  {
    boost::system::error_code ignored_ec;
    socket_.shutdown(tcp::socket::shutdown_both, ignored_ec);
  }

  tcp::socket socket_;
  std::wstring filename_;
  random_access_handle file_;
};

class server
{
public:
  server(boost::asio::io_service& io_service,
      unsigned short port, const std::wstring& filename)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
      filename_(filename)
  {
    start_accept();
  }

private:
  void start_accept()
  {
    connection::pointer new_connection =
      connection::create(GET_IO_SERVICE(acceptor_), filename_);

    acceptor_.async_accept(new_connection->socket(),
        std::bind(&server::handle_accept, 
            this, new_connection, std::placeholders::_1
            ));
  }

  void handle_accept(connection::pointer new_connection,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      new_connection->start();
    }

    start_accept();
  }

  tcp::acceptor acceptor_;
  std::wstring filename_;
};

void exec_name(std::wstring &process_name) 
{
    wchar_t* out_name = new wchar_t[MAX_PATH];
    DWORD out_len = MAX_PATH;
    DWORD pid = GetCurrentProcessId();
    HANDLE ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    BOOL ret = QueryFullProcessImageName(ph, NULL, out_name, &out_len);
    if (ret)
    {
        process_name = std::wstring(out_name);
    }
    delete []out_name;
}

TEST(asio, TransmitFile)
{
  try
  {
    boost::asio::io_service io_service;

    using namespace std; // For atoi.
    std::wstring process_name;
    exec_name(process_name);
    server s(io_service, 8080U, process_name);

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

#else // defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)
# error Overlapped I/O not available on this platform
#endif // defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)
