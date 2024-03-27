//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket SSL client, asynchronous
//
//------------------------------------------------------------------------------

#include "root_certificates.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/json/src.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <fstream>
// #include <fcntl.h>
// #include <unistd.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace json = boost::json;           // from <boost/json/src.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
class session : public std::enable_shared_from_this<session>
{
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string text_;

public:
    // Resolver and socket require an io_context
    explicit
    session(net::io_context& ioc, ssl::context& ctx)
        : resolver_(net::make_strand(ioc))
        , ws_(net::make_strand(ioc), ctx)
    {
    }

    // Start the asynchronous operation
    void
    run(
        char const* host,
        char const* port,
        char const* text)
    {
        // Save these for later
        host_ = host;
        text_ = text;

        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                &session::on_resolve,
                shared_from_this()));
    }

    void
    on_resolve(
        beast::error_code ec,
        tcp::resolver::results_type results)
    {
        if(ec)
            return fail(ec, "resolve");

        // Set a timeout on the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                &session::on_connect,
                shared_from_this()));
    }

    void
    on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
    {
        if(ec)
            return fail(ec, "connect");

        // Set a timeout on the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(
                ws_.next_layer().native_handle(),
                host_.c_str()))
        {
            ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category());
            return fail(ec, "connect");
        }

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host_ += ':' + std::to_string(ep.port());
        
        // Perform the SSL handshake
        ws_.next_layer().async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(
                &session::on_ssl_handshake,
                shared_from_this()));
    }

    void
    on_ssl_handshake(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "ssl_handshake");

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws_).expires_never();

        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async-ssl");
            }));

        // Perform the websocket handshake
        ws_.async_handshake(host_, "/",
            beast::bind_front_handler(
                &session::on_handshake,
                shared_from_this()));
    }

    void
    on_handshake(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "handshake");

        // Send the message
        ws_.async_write(
            net::buffer(text_),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "read");

        if (buffer_.size() > 0) {
            // Process the data
            std::cout << beast::make_printable(buffer_.data()) << std::endl;

            // Consume the buffer to clear it for the next read
            buffer_.consume(buffer_.size());
        }

    // Initiate the next read operation
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_close(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "close");
        // If we get here then the connection is closed gracefully
    }
    
    void on_signal_close(
        beast::error_code ec,
        int signal_number
    ) {
        if (ec) {
            std::string fail_message = "Exited with code " + std::to_string(signal_number);
            return fail(ec, fail_message.c_str());
        }
        
        // Close the WebSocket connection
        ws_.async_close(websocket::close_code::normal,
            beast::bind_front_handler(
                &session::on_close,
                shared_from_this()));
       
        std::cout << "\nExited gracefully." << std::endl;
    }
};

//------------------------------------------------------------------------------

std::string build_json_message(std::string jwt) {

    // Get current time point
    auto now = std::chrono::system_clock::now();
    // Convert to time since epoch, then to seconds
    auto epoch = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);
    // Convert to string
    std::string timestamp = std::to_string(seconds.count());

    json::object message;
    message["type"] = "subscribe"; // TODO: fix hardcode
    message["product_ids"] = json::array{"BTC-USD"}; // TODO: fix hardcode
    message["channel"] = "market_trades";
    message["jwt"] = jwt;
    message["timestamp"] = timestamp; 
    return json::serialize(message);
}

std::string get_jwt_from_pipe(const char* pipe_path) {
    // throw exceptions
    int pipe_fd = open(pipe_path, O_RDONLY);
    std::string jwt;

    if (pipe_fd == -1) {
        std::cerr << "Failed to open pipe for reading." << std::endl;
        return "";
    }
    // Create a file stream and associate it with the file descriptor
    std::filebuf fb;
    fb.open(pipe_path, std::ios_base::in);

    // Associate the file stream with a stream object (e.g., std::istream)
    std::istream pipe_stream(&fb);

    // Read data from the pipe using the stream
    std::getline(pipe_stream, jwt);

    // Close the file stream
    fb.close();
    return jwt;
}


int main(int argc, char** argv)
{
    // Check command line arguments.
    std::cout << argc << std::endl;
    if(argc != 3)
    {
        std::cerr <<
            "Usage: websocket-client-async-ssl <host> <port> <text>\n" <<
            "Example:\n" <<
            "    websocket-client-async-ssl echo.websocket.org 443\n";
        return EXIT_FAILURE;
    }
    auto const host = argv[1];
    auto const port = argv[2];

    const char* pipe_path = "/tmp/jwt_pipe";
    std::string jwt = get_jwt_from_pipe(pipe_path);
    std::string text_ = build_json_message(jwt);
    const char* text = text_.c_str();

    std::cout << text << std::endl;

    // The io_context is required for all I/O
    net::io_context ioc;
    net::io_context::work work(ioc); // provides work for the ioc to keep it open
    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::tlsv12_client};

    auto ws_session = std::make_shared<session>(ioc, ctx);

    // Setup signal handling
    boost::asio::signal_set signals(ioc, SIGINT);
    signals.async_wait(
        [&ioc, ws_session](const beast::error_code& ec, int signal_number) {
            ws_session->on_signal_close(ec, signal_number);
            ioc.stop(); // Stop the io_context to end the program
        }
    );

    // This holds the root certificate used for verification
    load_root_certificates(ctx);
    // Launch the asynchronous operation
    ws_session->run(host, port, text);

    // Run the I/O service. The call will return when
    // the socket is closed.
    ioc.run();

    std::cout << "done" << std::endl;

    return EXIT_SUCCESS;
}