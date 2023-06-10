#include<iostream>
#include<boost/asio.hpp>
#include<boost/asio/ts/buffer.hpp>
#include<boost/asio/ts/internet.hpp>
#include<string>
#include<vector>

std::vector<char> vBuffer(20*1024); //20KB

// Please note that read_handler() calls async_read_some() again after data has been written to std::cout. 
// This is required because you can’t be sure that the entire homepage was downloaded and copied into bytes
// in a single asynchronous operation. The repeated calls to async_read_some() followed by the repeated calls
// to read_handler() only end when the connection is closed, which happens when the web server has sent the
// entire homepage. Then read_handler() reports an error in ec. At this point, no further data is written
// to std::cout and async_read() is not called on the socket. Because there are no pending asynchronous
// operations, the program exits.
void grab_some_data(boost::asio::ip::tcp::socket& socket){
    socket.async_read_some(boost::asio::buffer(vBuffer.data(),vBuffer.size()),
        // This is read_handler function
        [&](boost::system::error_code ec,std::size_t length){
            if(!ec){
                std::cout << "\n\nRead " << length << " bytes\n\n";

                for(int i=0;i<length;++i){
                    std::cout << vBuffer[i];
                }

                grab_some_data(socket);
            }
        }
    );
}

int main(int argc, char const *argv[])
{
    boost::system::error_code ec;

    // Create a "context"- essentially the platform specific interface
    boost::asio::io_context context;

    // Get the address of somewhere we wish to connect to
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("93.184.216.34",ec),80);

    // Create a socket, the context will deliver the implementation
    boost::asio::ip::tcp::socket socket(context);

    socket.connect(endpoint,ec);

    if(!ec){
        // No error
        std::cout << "Connected\n";
    }else{
        std::cout << "Failed to connect\n";
    }

    if(socket.is_open()){
        grab_some_data(socket);

        std::string sRequest = 
            "GET /index.html HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Connection: close\r\n\r\n";

            socket.write_some(boost::asio::buffer(sRequest.data(),sRequest.size()),ec);

            // size_t bytes = socket.available();
            // std::cout << "Bytes Available: " << bytes << "\n";
            // if(bytes > 0){
            //     std::vector<char> vBuffer(bytes);

            //     socket.read_some(boost::asio::buffer(vBuffer.data(),vBuffer.size()),ec);

            //     for(auto c:vBuffer){
            //         std::cout << c;
            //     }
            // }

            grab_some_data(socket);
    }


    return 0;
}
