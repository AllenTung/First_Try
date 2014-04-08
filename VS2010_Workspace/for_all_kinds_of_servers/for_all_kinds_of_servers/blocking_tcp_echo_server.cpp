#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <time.h>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread/thread.hpp>

using namespace std;
using boost::asio::ip::tcp;
const int max_length = 1024;
typedef boost::shared_ptr<tcp::socket> socket_ptr;

void session(socket_ptr sock)
{
	try
	{
		for(;;)
		{
			char data[max_length];
			
			boost::system::error_code err_code;
			size_t length = sock->read_some(boost::asio::buffer(data), err_code);

			if(err_code == boost::asio::error::eof)
				break;
			else if (err_code)
			{
				throw boost::system::system_error(err_code);
			}

			boost::asio::write(*sock, boost::asio::buffer(data,length));
		}
	}
	catch (exception& e)
	{
		cerr << "Exception in thread:" << e.what() << endl;
	}
}

void server(boost::asio::io_service& io_service, unsigned short port)
{
	tcp::acceptor tcp_acceptor(io_service, tcp::endpoint(tcp::v4(), port));
	for(;;)
	{
		socket_ptr sock(new tcp::socket(io_service));
		tcp_acceptor.accept(*sock);
		boost::thread temp_thread(boost::bind(session, sock));
	}
}

int main()
{
	int argc = 2;
	int forcin = 0;
	char* argv[5];
	argv[0] = "localhost";
	argv[1] = "13";
	try
	{
		boost::asio::io_service io_service;

		server(io_service, atoi(argv[1]));
	}
	catch (exception& e)
	{
		cerr << "Exception:" << e.what() << endl;
	}

	cin >> forcin;

	return 0;
}