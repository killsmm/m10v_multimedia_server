#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <string>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;

class HttpConnection : public boost::enable_shared_from_this<HttpConnection> {
public:
    typedef boost::shared_ptr<HttpConnection> pointer;

    static pointer create(io_service& io_service, std::string path) {
        return pointer(new HttpConnection(io_service, path));
    }

    tcp::socket& socket() {
        return socket_;
    }

    void start() {
        boost::asio::async_read_until(socket_, request_, "\r\n",
            boost::bind(&HttpConnection::handle_read, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

private:
    HttpConnection(io_service& io_service, std::string path)
        : socket_(io_service), path_(path) {
    }

    void handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
        if (!error) {
            std::istream request_stream(&request_);
            std::string request_line;
            std::getline(request_stream, request_line);

            std::string http_version;
            request_stream >> http_version;

            std::string header;
            while (std::getline(request_stream, header) && header != "\r") {
            }

            std::string response_body = "Hello, world!";
            std::ostringstream response_stream;
            response_stream << "HTTP/1.1 200 OK\r\n";
            response_stream << "Content-Type: text/plain\r\n";
            response_stream << "Content-Length: " << response_body.length() << "\r\n";
            response_stream << "Connection: close\r\n";
            response_stream << "\r\n";
            response_stream << response_body;

            std::string response = response_stream.str();
            boost::asio::async_write(socket_, boost::asio::buffer(response),
                boost::bind(&HttpConnection::handle_write, shared_from_this(),
                    boost::asio::placeholders::error));
        }
    }

    void handle_write(const boost::system::error_code& error) {
        if (!error) {
            boost::system::error_code ignored_ec;
            socket_.shutdown(tcp::socket::shutdown_both, ignored_ec);
        }
    }

    tcp::socket socket_;
    boost::asio::streambuf request_;
    std::string path_;
};



class tcp_connection : public boost::enable_shared_from_this<tcp_connection> {
public:
    typedef boost::shared_ptr<tcp_connection> pointer;

    static pointer create(boost::asio::io_service& io_service, std::string path) {
        return pointer(new tcp_connection(io_service, path));
    }

    tcp::socket& socket() {
        return socket_;
    }

    void start() {
        HttpConnection::pointer new_connection = HttpConnection::create(io_service_, path_);
        socket_.async_read_some(boost::asio::buffer(buffer_),
            boost::bind(&tcp_connection::handle_read, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred,
                new_connection));
    }

private:
    tcp_connection(boost::asio::io_service& io_service, std::string path)
        : io_service_(io_service), socket_(io_service), path_(path) {
    }

    void handle_read(const boost::system::error_code& error, size_t bytes_transferred, HttpConnection::pointer new_connection) {
        if (!error) {
            boost::asio::async_write(socket_, boost::asio::buffer(buffer_, bytes_transferred),
                boost::bind(&tcp_connection::handle_write, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred,
                    new_connection));
        }
    }

    void handle_write(const boost::system::error_code& error, size_t bytes_transferred, HttpConnection::pointer new_connection) {
        if (!error) {
            new_connection->socket().async_read_some(boost::asio::buffer(buffer_),
                boost::bind(&tcp_connection::handle_read, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred,
                    new_connection));
        }
    }

    boost::asio::io_service& io_service_;
    tcp::socket socket_;
    std::string path_;
    enum { max_length = 1024 };
    char buffer_[max_length];
};



class HttpServer {
public:
    HttpServer(io_service& io_service, short port, std::string path)
        : io_service_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port)), path_(path) {
        start_accept();
    }

private:
    void start_accept() {
        tcp_connection::pointer new_connection = tcp_connection::create(io_service_, path_);
        acceptor_.async_accept(new_connection->socket(),
            boost::bind(&HttpServer::handle_accept, this, new_connection,
                boost::asio::placeholders::error));
    }

    void handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error) {
        if (!error) {
            new_connection->start();
        }

        start_accept();
    }

    io_service& io_service_;
    tcp::acceptor acceptor_;
    std::string path_;
};

