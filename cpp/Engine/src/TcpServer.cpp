#include "TcpServer.hpp"

#include "ClientSession.hpp"
#include "Logger.hpp"

using boost::asio::ip::tcp;

namespace ds
{
TcpServer::TcpServer(std::uint16_t port,
                     AnomalyDetector &detector,
                     std::size_t expected_input_size)
    : io_context_(),
      acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)),
      detector_(detector),
      expected_input_size_(expected_input_size)
{
}

void TcpServer::run()
{
    ds::log::info("Server listening on port " + std::to_string(acceptor_.local_endpoint().port()));

    while (true)
    {
        tcp::socket socket(io_context_);
        acceptor_.accept(socket);

        ClientSession session(socket, detector_, expected_input_size_);
        session.run();
    }
}
} // namespace ds
