#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

constexpr double THRESHOLD = 10.0;

double compute_score(const std::vector<double> &values)
{
    double sum = 0.0;
    for (double v : values)
        sum += v;
    return sum;
}

std::vector<double> parse_input(const std::string &input)
{
    std::vector<double> values;
    std::stringstream ss(input);
    double value;

    while (ss >> value)
    {
        values.push_back(value);
    }

    return values;
}

int main()
{
    try
    {
        boost::asio::io_context io_context;

        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 9000));

        std::cout << "TCP Server listening on port 9000...\n";

        while (true)
        {
            tcp::socket socket(io_context);
            acceptor.accept(socket);

            std::cout << "Client connected.\n";

            boost::asio::streambuf buffer;
            boost::asio::read_until(socket, buffer, "\n");

            std::istream input_stream(&buffer);
            std::string data;
            std::getline(input_stream, data);

            std::cout << "Received: " << data << "\n";

            auto values = parse_input(data);
            double score = compute_score(values);

            std::string response;

            if (score > THRESHOLD)
                response = "ANOMALY\n";
            else
                response = "OK\n";

            boost::asio::write(socket, boost::asio::buffer(response));

            std::cout << "Response sent: " << response;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}