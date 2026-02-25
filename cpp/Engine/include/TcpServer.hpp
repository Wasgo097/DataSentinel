#pragma once

#include <boost/asio.hpp>

#include "AnomalyDetector.hpp"

namespace ds
{
class TcpServer
{
public:
    TcpServer(std::uint16_t port,
              AnomalyDetector &detector,
              std::size_t expected_input_size);

    void run();

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    AnomalyDetector &detector_;
    std::size_t expected_input_size_;
};
} // namespace ds
