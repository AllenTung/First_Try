#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

enum {max_length = 1024};

int main()
{
	int argc = 3;
	int forcin = 0;
	char* argv[6];
	argv[0] = "localhost";
	argv[1] = "localhost";
	argv[2] = "13";

	char request[max_length],reply[max_length];
	try
	{
		if (argc != 3)
		{
			std::cerr << "Usage: blocking_tcp_echo_client <host> <port>\n";
			return 1;
		}

		boost::asio::io_service io_service;

		tcp::resolver resolver(io_service);
		tcp::resolver::query query(tcp::v4(), argv[1], argv[2]);
		tcp::resolver::iterator iterator = resolver.resolve(query);

		tcp::socket client_proxy_socket(io_service);
		boost::asio::connect(client_proxy_socket, iterator);

		cout << "Enter Message:";
		cin.getline(request, max_length);
		size_t request_length = strlen(request);
		boost::asio::write(client_proxy_socket, boost::asio::buffer(request, request_length));

		size_t reply_length = boost::asio::read(client_proxy_socket, boost::asio::buffer(reply, request_length));

		cout<< "Reply is:";
		cout.write(reply, reply_length);
		cout<<endl;

	}
	catch(exception& e)
	{
		cerr << "Exception:" << e.what() << "\n";
	}


	cin >> forcin;

	return 0;
}