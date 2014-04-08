#include <stdlib.h>
#include <stdio.h>
#include <ctime>
#include <vector>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/asio/basic_socket.hpp>

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
	overlapped_ptr overlapped(socket.get_io_service(), handler);
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

class connection: public boost::enable_shared_from_this<connection>
{
public:
	typedef boost::shared_ptr<connection> pointer;
	tcp::socket socket_;
	std::string filename_;
	random_access_handle file_;

	static pointer create(boost::asio::io_service& io_service,
		const std::string& filename)
	{

		return pointer(new connection(io_service, filename));
	}

	tcp::socket& socket()
	{
		return socket_;
	}

	void start()
	{
		boost::system::error_code ec;
		file_.assign(::CreateFile(filename_.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0), ec);
		if (file_.is_open())
		{
			std::cout << "\nAbout To Transmit the file!!!!!!!!!!!!!!\n";

			transmit_file(socket_, file_,
				boost::bind(&connection::handle_write, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
	}

	connection(boost::asio::io_service& io_service, const std::string& filename)
		: socket_(io_service),
		filename_(filename),
		file_(io_service)
	{
		std::cout << "\nConnection Construcetd!!!!!!!!!\n";
	}

	void handle_write(const boost::system::error_code& /*error*/,size_t /*bytes_transferred*/)
	{
		std::cout << "\nTransmit Completed!!!!!!!!!\n";
		boost::system::error_code ignored_ec;
		socket_.shutdown(tcp::socket::shutdown_both, ignored_ec);
	}
};

class server
{
public:
	server(boost::asio::io_service& io_service,	const std::string& address, const std::string& port, const std::string& filename)
		: acceptor_(io_service),
		filename_(filename)
	{
		tcp::resolver resolver(acceptor_.get_io_service());
		tcp::resolver::query query(address, port);
		tcp::endpoint endpoint = *resolver.resolve(query);

		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(tcp::acceptor::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen();

		start_accept();
	}


	void start_accept()
	{
		connection::pointer new_connection =
			connection::create(acceptor_.get_io_service(), filename_);

		acceptor_.async_accept(new_connection->socket_,
			boost::bind(&server::handle_accept, this, new_connection,
			boost::asio::placeholders::error));
	}

	void handle_accept(connection::pointer new_connection,
		const boost::system::error_code& error)
	{
		std::cout << "\n Begin to handle_accept!\n";
		if (!error)
		{
			new_connection->start();
		}

		start_accept();
	}

	tcp::acceptor acceptor_;
	std::string filename_;
};

int main()
{
	int forcin = 0;
	int argc = 3;
	char* argv[5];

	argv[0] = "transmit_file";
	argv[1] = "80";
	argv[2] = "I:/test_transmitfile2.txt";
	argv[3] = "localhost";

	try
	{
		if (argc != 3)
		{
			std::cerr << "Usage: transmit_file <port> <filename>\n";
			return 1;
		}

		boost::asio::io_service io_service;

		using namespace std; // For atoi.
		server s(io_service, argv[3], argv[1], argv[2]);

		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	std::cin >> forcin;

	return 0;
}

#else // defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)
# error Overlapped I/O not available on this platform
#endif // defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)