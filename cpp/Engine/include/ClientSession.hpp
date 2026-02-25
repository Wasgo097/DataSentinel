#pragma once

#include <boost/asio/ip/tcp.hpp>

#include "AnomalyDetector.hpp"

namespace ds
{
class ClientSession
{
public:
    ClientSession(boost::asio::ip::tcp::socket &socket,
                  AnomalyDetector &detector,
                  std::size_t expected_input_size);

    void run();

private:
    boost::asio::ip::tcp::socket &socket_;
    AnomalyDetector &detector_;
    std::size_t expected_input_size_;
};
} // namespace ds
