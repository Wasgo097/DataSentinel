#include "ClientSession.hpp"

#include <boost/asio.hpp>

#include <string>

#include "InputParser.hpp"
#include "Logger.hpp"

using boost::asio::ip::tcp;

namespace ds
{
ClientSession::ClientSession(tcp::socket &socket,
                             AnomalyDetector &detector,
                             std::size_t expected_input_size)
    : socket_(socket),
      detector_(detector),
      expected_input_size_(expected_input_size)
{
}

void ClientSession::run()
{
    ds::log::info("Client connected: " + socket_.remote_endpoint().address().to_string());

    boost::asio::streambuf buffer;

    while (socket_.is_open())
    {
        try
        {
            buffer.consume(buffer.size());
            boost::asio::read_until(socket_, buffer, "\n");

            std::istream input_stream(&buffer);
            std::string raw_data;
            std::getline(input_stream, raw_data);

            ds::log::info("Received raw: " + raw_data);

            const auto values = parse_input_line(raw_data);

            if (values.size() != expected_input_size_)
            {
                ds::log::error("Invalid input size. Expected " + std::to_string(expected_input_size_) +
                               ", got " + std::to_string(values.size()));

                const std::string response = "ERROR: Invalid input size\n";
                boost::asio::write(socket_, boost::asio::buffer(response));
                continue;
            }

            const auto result = detector_.evaluate(values);
            ds::log::info("Reconstruction MSE: " + std::to_string(result.mse));

            const std::string response = result.response_line();
            ds::log::info("Sending response: " + response);

            boost::asio::write(socket_, boost::asio::buffer(response));
        }
        catch (const std::exception &ex)
        {
            ds::log::error(ex.what());
            break;
        }
    }

    if (socket_.is_open())
    {
        socket_.close();
    }

    ds::log::info("Client disconnected");
}
} // namespace ds
