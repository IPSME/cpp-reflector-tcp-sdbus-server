//
// async_tcp_echo_server.cpp -> main.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <set>

#include "IPSME_MsgEnv.h"
#include "msg_cache-dedup.h"

duplicate g_duplicate;

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
// pragma mark asio -> sdbus

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
		
		if (true == g_duplicate.exists(str_msg))
			std::cerr << "async_read_some(): asio ->| *DUP -- [" << str_msg << "]\n";

		else
		{
			std::cerr << "handler_msg(kpks_recvd): asio -> sdbus -- [" << str_msg << "]\n";

			IPSME_MsgEnv::publish("s", str_msg.c_str());
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
// pragma mark asio <- sdbus

bool handler_str_(sd_bus_message* p_msg, std::string str_msg)
{
	std::cerr << __func__ << ": asio[" << g_set_sockets.size() << "] <- sdbus -- [" << str_msg.c_str() << "]\n";
	
	g_duplicate.cache(str_msg, t_entry_context(30s));
	
	for (auto it : g_set_sockets)
		do_write(it, str_msg);

    return true;
}

std::string bus_message_str(sd_bus_message* p_msg) {
    const char* psz= nullptr;
    int i_r = sd_bus_message_read_basic(p_msg, SD_BUS_TYPE_STRING, &psz);
    if (i_r < 0)
        throw i_r;

    return psz;
}

void handler_(sd_bus_message* p_msg)
{
    // printf("%s: \n", __func__);

    try {
        char ch_type;
        if (sd_bus_message_peek_type(p_msg, &ch_type, nullptr) && (ch_type=='s') && handler_str_(p_msg, bus_message_str(p_msg) ))
            return;

    }
    catch (int &i_r) {
        printf("ERR: error is message execution: %s \n", strerror(-i_r));
    }
    catch (...) {
        printf("ERR: error is message execution \n");
        return;
    }

    printf("%s: DROP! \n", __func__);
}

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
		
		std::cerr << __func__ << ": " << "Connecting ..." << "\n";

		IPSME_MsgEnv::subscribe(&handler_);

		// -----
		
		asio::io_context ioctx;

		int i_port= std::atoi(argv[1]);
		server s(ioctx, i_port);

		std::cout << __func__ << ": " << str_exec << " listening on [" << i_port << "]\n";

		std::cerr << __func__ << ": " << "Running ..." << "\n";

		// ioctx.run();
		while (true) 
		{
			ioctx.poll();
			IPSME_MsgEnv::process_requests();

			// if (ioctx.stopped() && ioctx.poll_one() == 0)
			// 	break;

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	
	return 0;
}

