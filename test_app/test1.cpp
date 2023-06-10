#include<iostream>
#include<boost/bind/bind.hpp>
#include<boost/asio.hpp>
#include<boost/asio/ts/buffer.hpp>
#include<boost/asio/ts/internet.hpp>
#include<string>
#include<vector>
#include<functional>

std::vector<char> vBuffer(20*1024); //20KB

/*
void call_back(const boost::system::error_code& ec,
                    std::size_t bytes_transferred
                  ){

    if(!ec){
       std::cout << "\n\nRead " << bytes_transferred << " bytes\n\n";
       for(int i=0; i<bytes_transferred ; ++i){
           std::cout << vBuffer[i];
       }

       socket.async_read_some(boost::asio::buffer(vBuffer.data(),vBuffer.size()),call_back);
    }
}
*/

int main(int argc,char **argv){
   boost::system::error_code ec;

    // Create a "context"- essentially the platform specific interface
    // boost::asio::io_context context;
    boost::asio::io_service ios;

    // Get the address of somewhere we wish to connect to
    // boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("93.184.216.34",ec),80);
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("51.38.81.49",ec),80);

    // Create a socket, the context will deliver the implementation
    // boost::asio::ip::tcp::socket socket(context);
    boost::asio::ip::tcp::socket socket(ios);

    socket.connect(endpoint,ec);

    if(!ec){
        // No error
        std::cout << "Connected\n";
    }else{
        std::cout << "Failed to connect\n";
    }

    std::function<void(const boost::system::error_code&,std::size_t)> call_back = [&socket,&call_back](const boost::system::error_code& ec,std::size_t bytes_transferred){
        if(!ec){
            std::cout << "\n\nRead " << bytes_transferred << " bytes\n\n";
            for(int i=0; i<bytes_transferred ; ++i){
                std::cout << vBuffer[i];
            }

            socket.async_read_some(boost::asio::buffer(vBuffer.data(),vBuffer.size()),call_back);
        }
    };

    if(socket.is_open()){
        // grab_some_data(socket);

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

            // grab_some_data(socket);
            socket.async_read_some(boost::asio::buffer(vBuffer.data(),vBuffer.size()),call_back);
        }


    ios.run();
    return 0;
}
