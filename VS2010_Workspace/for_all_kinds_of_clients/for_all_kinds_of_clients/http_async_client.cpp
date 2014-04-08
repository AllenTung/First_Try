#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using namespace std;
using boost::asio::ip::tcp;

class client
{
public:
	client(boost::asio::io_service& io_service,
		const string& server, const string& path):resolver_(io_service), socket_(io_service)
	{
		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		std::ostream request_stream(&request_);
		request_stream << "GET " << path << " HTTP/1.0\r\n";
		request_stream << "Host: " << server << "\r\n";
		request_stream << "Accept: */*\r\n";
		request_stream << "Connection: close\r\n\r\n"; 

		//Start an asynchronous resolve to translate the server and service names
		//into a list of endpoints
		tcp::resolver::query query(server, "http");
		resolver_.async_resolve(query,
			boost::bind(&client::handle_resolve, this, 
			boost::asio::placeholders::error,
			boost::asio::placeholders::iterator));

	}

private:
	tcp::resolver resolver_;
	tcp::socket socket_;
	boost::asio::streambuf request_;
	boost::asio::streambuf response_;

	void handle_resolve(const boost::system::error_code& err, 
		tcp::resolver::iterator endpoint_iterator)
	{
		if(!err)
		{
			boost::asio::async_connect(socket_, endpoint_iterator,
				boost::bind(&client::handle_connect, this, 
				boost::asio::placeholders::error));
		}
		else
		{
			cout << "Error:" << err.message() << endl;
		}
	}

	void handle_connect(const boost::system::error_code& err)
	{
		if (!err)
		{
			boost::asio::async_write(socket_, request_, 
				boost::bind(&client::handle_write_request, this,
				boost::asio::placeholders::error));
		}
		else
		{
			cout << "Error:" << err.message() <<endl;
		}
	}

	void handle_write_request(const boost::system::error_code& err)
	{
		if (!err)
		{
			boost::asio::async_read_until(socket_, response_, "\r\n", 
				boost::bind(&client::handle_read_status_line, this, 
				boost::asio::placeholders::error));
		}
		else
		{
			cout << "Error:" << err.message() << endl;
		}
	}

	void handle_read_status_line(const boost::system::error_code& err)
	{
		if (!err)
		{

			istream response_stream(&response_);
			string http_version;
			unsigned int status_code;
			string status_message;
			
			response_stream >> http_version;
			response_stream >> status_code;
			
			getline(response_stream, status_message);

			if(!response_stream || http_version.substr(0, 5) != "HTTP/")
			{
				cout << "Invalid response\n";
				return;
			}
			if (status_code != 200)
			{
				cout << "Response returned with status code " << status_code <<endl;
				return;
			}
			// Read the response headers, which are terminated by a blank line which means \r\n\r\n
			boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
				boost::bind(&client::handle_read_headers, this,
				boost::asio::placeholders::error));
			cout << "\n\n\n\nJust to tell I was here for status line!!!!\n\n\n\n\n";

		}
		else
		{
			cout << "Error:" << err << endl;
		}
	}

	void handle_read_headers(const boost::system::error_code& err)
	{
		if (!err)
		{
			istream response_stream(&response_);
			string header;

			while (getline(response_stream, header) && header != "\r")
			{
				cout << header << endl;
			}
			cout << "\n";

			if (response_.size() > 0)
			{
				cout << &response_;
			}

			//Start reading remaining data until EOF
			boost::asio::async_read(socket_, response_, boost::asio::transfer_at_least(1),
				boost::bind(&client::handle_read_content, this, boost::asio::placeholders::error));

			cout << "\n\n\n\nJust to tell I was here for headers!!!!\n\n\n\n\n";
			
		}
		else
		{
			cout << "Error:" << err << endl;
		}
	}

	void handle_read_content(const boost::system::error_code& err)
	{
		if (!err)
		{
			//write all of the data that has been read so far
			cout << &response_;
			boost::asio::async_read(socket_, response_,
				boost::asio::transfer_at_least(1),
				boost::bind(&client::handle_read_content, this,
				boost::asio::placeholders::error));

			cout << "\n\n\n\nJust to tell I was here for content !!!!\n\n\n\n\n";
		}
		else if (err != boost::asio::error::eof)
		{
			cout << "error:" << err << endl;
		}
	}
};

int main()
{
	int forcin = 0;
	int argc = 3;
	char* argv[5];
	argv[0] = "asnc_client";
// 	argv[1] = "www.boost.org";
// 	argv[2] = "/LICENSE_1_0.txt";
	argv[1] = "localhost";
	argv[2] = "/test.txt";

	try
	{
		if (argc != 3)
		{
			std::cout << "Usage: async_client <server> <path>\n";
			std::cout << "Example:\n";
			std::cout << "  async_client www.boost.org /LICENSE_1_0.txt\n";
			return 1;
		}

		boost::asio::io_service io_service;
		client temp_client(io_service, argv[1], argv[2]);
		io_service.run();
	}
	catch (exception& e)
	{
		cout << "Exception: " << e.what() << endl;
	}

	cin >> forcin;
	return 0;
}