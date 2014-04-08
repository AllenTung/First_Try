#ifndef HTTP_REQUEST_HANDLER_HPP
#define HTTP_REQUEST_HANDLER_HPP

#include <string>
#include <boost/noncopyable.hpp>
/*#include "global_stuff.hpp"*/


class reply;
class request;

/// The common handler for all incoming requests.
class request_handler
{
public:
	/// Construct with a directory containing files to be served.
	int server_id;
	explicit request_handler(const std::string& doc_root, int s_id);

	/// Handle a request and produce a reply.
	void handle_get_request(const request& req, reply& rep);
	void handle_post_request(const request& req, reply& rep);

	string doc_root_;

};


#endif // HTTP_REQUEST_HANDLER_HPP