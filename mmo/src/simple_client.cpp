#include <boost/asio.hpp>
#include <iostream>
#include <olc_net.hpp>

enum class CustomMsgTypes : uint32_t{
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage
};


class CustomClient : public olc::net::client_interface<CustomMsgTypes>{
public:
    void pingServer(){
        // std::cout << "PING!!!!!!!!\n";

        olc::net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::ServerPing;

        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

        msg << timeNow; // Here we are adding data into msg body

        send(msg);
    }

    void messageAll(){
        olc::net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::MessageAll;
        send(msg);
        // For messageAll we did not added any data into msg body
        // so, msg.header.size is 0
    }
};

int main(){
    using namespace boost::asio;

    CustomClient c;
    c.connect("127.0.0.1",60000);

    io_service ioservice;
    posix::stream_descriptor stream(ioservice, STDIN_FILENO);

    auto my_handler = [&](){
        if(c.isConnected()){
            // std::cout << c.Incoming().empty() << "\n";
            if(!c.Incoming().empty()){

                auto msg = c.Incoming().pop_front().msg;

                switch(msg.header.id){
                case CustomMsgTypes::ServerPing:
                    {
                        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
                        std::chrono::system_clock::time_point timeThen;

                        msg >> timeThen;

                        std::cout << "Ping: " <<
                            std::chrono::duration<double>(timeNow - timeThen).count() << " seconds\n";
                    }
                    break;

                case CustomMsgTypes::ServerAccept:
                    {
                        // Server has responded to a ping request
                        std::cout << "Server Accepted Connection\n";
                    }
                    break;
                case CustomMsgTypes::ServerMessage:
                    {
                        uint32_t clientID;
                        msg >> clientID;
                        std::cout << "Hello from [" << clientID << "]\n";
                    }
                    break;
                }

            }
        }
    };


    char buf[1] = {};
    std::function<void(boost::system::error_code, size_t)> read_handler;

    read_handler = [&](boost::system::error_code ec, size_t len) {
            if (ec) {
                std::cerr << "exit with " << ec.message() << std::endl;
            } else {
                if (len == 1 && buf[0] == 'a') {
                    // std::cout << "keyinput=" << buf[0] << std::endl;

                    c.pingServer();
                }else if(len == 1 && buf[0] == 'b'){
                    c.messageAll();
                }
                my_handler();
                async_read(stream, buffer(buf,sizeof(buf)), read_handler);
            }
        };

    async_read(stream, buffer(buf,sizeof(buf)), read_handler);

    // c.pingServer();

    /*
    bool quitFlag = false;
    while(!quitFlag){
        if(c.isConnected()){
            // std::cout << c.Incoming().empty() << "\n";
            if(!c.Incoming().empty()){

                auto msg = c.Incoming().pop_front().msg;

                switch(msg.header.id){
                case CustomMsgTypes::ServerPing:
                    {
                        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
                        std::chrono::system_clock::time_point timeThen;

                        msg >> timeThen;

                        std::cout << "Ping: " <<
                            std::chrono::duration<double>(timeNow - timeThen).count() << " seconds\n";
                    }
                    break;

                case CustomMsgTypes::ServerAccept:
                    {
                        // Server has responded to a ping request
                        std::cout << "Server Accepted Connection\n";
                    }
                    break;
                case CustomMsgTypes::ServerMessage:
                    {
                        uint32_t clientID;
                        msg >> clientID;
                        std::cout << "Hello from [" << clientID << "]\n";
                    }
                    break;
                }

            }
        }

    }
    */

    ioservice.run();

    return 0;
}
