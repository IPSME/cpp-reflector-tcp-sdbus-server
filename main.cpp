//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <set>

//#include "duplicate.h"

//duplicate g_dup_asio;
//duplicate g_dup_zmq;

#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

using asio::ip::tcp;
using namespace std::chrono_literals;

typedef std::shared_ptr<tcp::socket> t_ptr_socket;
typedef std::set<t_ptr_socket> t_set_sockets;
t_set_sockets g_set_sockets;

enum { max_length = 0xffff };
char msg_[max_length];

//----------------------------------------------------------------------------------------------------------------
// pragma mark asio -> zmq

void do_read(t_ptr_socket ptr_socket)
{
	// std::cerr << "do_read(): \n";
	
	ptr_socket->async_read_some(asio::buffer(msg_, max_length), [ptr_socket](std::error_code ec, std::size_t length)
	{
		if (ec) {
			std::cerr << "async_read_some(): ERROR! " << ec << "\n";

			if (ptr_socket->is_open()) {
				ptr_socket->close();
				g_set_sockets.erase(ptr_socket);
			}
			return;
		}
		
		// std::cerr << "async_read_some(): " << msg_ << "\n";
		
		std::string str_msg= std::string(msg_, length);
		
		// if (true == g_dup_asio.exists(str_msg))
		if (false)
			std::cerr << "async_read_some(): asio ->| *DUP -- [" << str_msg << "]\n";

		else
		{
			std::cerr << "handler_msg(kpks_recvd): asio -> zmq -- [" << str_msg << "]\n";

			//g_dup_zmq.cache(str_msg, t_entry_context(30s));
			
			//msg::pubsub->publish(str_msg.c_str(), str_msg.length());
		}

		do_read(ptr_socket);
	});
}

void do_write(t_ptr_socket ptr_socket, std::string str_msg)
{
	asio::async_write(*ptr_socket, asio::buffer(str_msg), [ptr_socket](std::error_code ec, std::size_t /*length*/)
	{
		if (ec) {
			std::cerr << "async_write(): ERROR! " << ec << "\n";

			if (ptr_socket->is_open()) {
				ptr_socket->close();
				g_set_sockets.erase(ptr_socket);
			}
			return;
		}
	});
}

//----------------------------------------------------------------------------------------------------------------
// pragma mark asio <- zmq

/*
static void handler_(IMessenger* msgr, const char * const kpks_recvd, size_t zt_len)
{
	std::string str_recvd= std::string(kpks_recvd, zt_len);
	
	//TODO: What encoding does nodejs use? should the code check for NSUTF16StringEncoding ?1
	
	if (true == g_dup_zmq.exists(str_recvd)) {
		// NSLog(@"handler_msg(kpks_recvd): *DUP |<- zmw -- [%s]", str_recvd.c_str());
		return;
	}
	
	std::cerr << "handler_msg(kpks_recvd): asio[" << g_set_sockets.size() << "] <- zmq -- [" << str_recvd.c_str() << "]\n";
	
	g_dup_asio.cache(str_recvd, t_entry_context(30s));
	
	for (auto it : g_set_sockets)
		do_write(it, str_recvd);
}
*/

//----------------------------------------------------------------------------------------------------------------
// pragma mark server

class server
{
	tcp::acceptor acceptor_;
	tcp::socket socket_;

	void do_accept()
	{
		acceptor_.async_accept(socket_, [this](std::error_code ec)
		{
			if (!ec)
			{
				t_ptr_socket ptr_socket= std::make_shared<tcp::socket>(std::move(socket_));
				g_set_sockets.insert(ptr_socket);

				tcp::endpoint r_ep= ptr_socket->remote_endpoint();
				tcp::endpoint l_ep= ptr_socket->local_endpoint();
				
				std::cerr << "do_accept(): client [" << r_ep.address() << ":" << r_ep.port() << "] connected on port [" << l_ep.port() << "]\n";
				
				do_read(ptr_socket);
			}
			
			do_accept();
		});
	}


public:
  	server(asio::io_context& io_context, short port)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), socket_(io_context)
	{
		do_accept();
	}
};

#ifdef _WIN32
const char k_sep_ = '\\';
#else
const char k_sep_ = '/';
#endif

int main(int argc, char* argv[])
{
	std::string str_exec= argv[0];

	const size_t i = str_exec.rfind(k_sep_, str_exec.length());
	if (i != std::string::npos)
		str_exec= str_exec.substr(i+1, str_exec.length());

	if (argc != 2) {
		std::cerr << "Usage: " << str_exec << " <port>\n";
		return 1;
	}
	
	try
	{
		// -----
		
//		std::cerr << "main(): " << "Connecting ..." << "\n";

//		std::shared_ptr<MsgQ> mq= std::make_shared<MsgQ>(eeid_ID_);
//		
//		t_port_pair port_pair(2323, 2322);
//		msg::pubsub->connect(mq, "ipsme-trivial", port_pair); // "localhost", "iotasrv1.local"
//		
//		MsgQ::t_Messengers messengers= { msg::pubsub };
//		mq->poll_messengers(messengers, handler_);
//		
//		msg::pubsub->subscribe("");

		// -----
		
		asio::io_context ioctx;

		int i_port= std::atoi(argv[1]);
		server s(ioctx, i_port);

		std::cout << "main(): " << str_exec << " listening on [" << i_port << "]\n";

		std::cerr << "main(): " << "Running ..." << "\n";

		ioctx.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	
	return 0;
}

