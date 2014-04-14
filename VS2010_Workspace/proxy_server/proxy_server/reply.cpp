#pragma once
#include "reply.hpp"
#include <string>
#include <boost/lexical_cast.hpp>
namespace misc_strings 
{

	const char name_value_separator[] = { ':', ' ' };
	const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

std::vector<boost::asio::const_buffer> reply::to_buffers()
{
	std::vector<boost::asio::const_buffer> buffers;
	for (std::size_t i = 0; i < headers.size(); ++i)
	{
		header& h = headers[i];
		buffers.push_back(boost::asio::buffer(h.name));
		buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
		buffers.push_back(boost::asio::buffer(h.value));
		buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	}
	buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	buffers.push_back(boost::asio::buffer(content));
	return buffers;
}


std::vector<boost::asio::const_buffer> reply::simple_ready_buffers()
{
	std::vector<boost::asio::const_buffer> buffers;
	
	buffers.push_back(boost::asio::buffer(server_status));
	buffers.push_back(boost::asio::buffer(misc_strings::crlf));

	return buffers;
}

std::vector<boost::asio::const_buffer> reply::simple_location_buffers()
{
	std::vector<boost::asio::const_buffer> buffers;
	headers.resize(1);
	headers[0].name = "server_id:";
	headers[0].value = int_to_string(server_id);

	header& h = headers[0];
	buffers.push_back(boost::asio::buffer(h.name));
	buffers.push_back(boost::asio::buffer(h.value));
	buffers.push_back(boost::asio::buffer(misc_strings::crlf));

	return buffers;
}
