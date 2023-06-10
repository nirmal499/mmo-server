#include <boost/asio.hpp>
#include <iostream>

using namespace boost::asio;

int main()
{
    io_service ioservice;
    posix::stream_descriptor stream(ioservice, STDIN_FILENO);

    char buf[1] = {};

    std::function<void(boost::system::error_code, size_t)> read_handler;

    int count = 0;
    read_handler = [&](boost::system::error_code ec, size_t len) {
            if (ec) {
                std::cerr << "exit with " << ec.message() << std::endl;
            } else {
                if (len == 1 && buf[0] == 'a') {
                    std::cout << "keyinput=" << buf[0] << std::endl;
                    std::cout << "RUN " << ++count << "\n";
                }
                async_read(stream, buffer(buf,sizeof(buf)), read_handler);
            }
        };


    async_read(stream, buffer(buf), read_handler);

    ioservice.run();
}
