#pragma once

#include "net_common.hpp"
#include "net_message.hpp"
#include "net_tsqueue.hpp"
#include "net_connection.hpp"

namespace olc{
    namespace net{
        template<typename T>
        class client_interface{
        public:
            client_interface(): m_socket(m_ios){
                // Initialise the socket with the io_service
            }
            virtual ~client_interface(){
                // If the client is destroyed always try to disconnect from the server
                disconnect();
            }
        protected:

            // asio io_service handles the data transfer and for every client it will have it own io_service. It will not be shared like connection class
            asio::io_service m_ios;

            std::thread m_thread; // Reponsible for running the io_service

            asio::ip::tcp::socket m_socket;

            // The client has a single instance of a "connection" object, which handles data transfer
            std::unique_ptr<connection<T>> m_conn;

        public:
            // Connect to server with hostname/ip-address and port
            bool connect(const std::string& host, const uint16_t port){
                try{

                    // Resolve the hostname into IP address
                    asio::ip::tcp::resolver resolver(m_ios);
                    auto m_endpoints = resolver.resolve(host,std::to_string(port));

                    // Here the m_ios and m_qmessages_in are passed as reference
                    m_conn = std::make_unique<connection<T>>(connection<T>::owner::client,
                                                             m_ios,
                                                             asio::ip::tcp::socket(m_ios),
                                                             m_qmessages_in);
                    // Tell the connection object to connect to the server
                    m_conn->connectToServer(m_endpoints);

                    // Start the thread that will run io_service
                    m_thread = std::thread([this](){
                        m_ios.run();
                    });


                }catch(std::exception& e){
                    std::cerr << "Client Exception : " << e.what() << "\n";
                    return false;
                }

                return true;
            }

            // Disconnect from server
            void disconnect(){
                // If connection exists, and it's connected then...
                if(isConnected()){

                    // ....disconnect from server gracefully
                    m_conn->disconnect();
                }

                // We are done with asio io_service
                m_ios.stop();

                // and its thread which was responsible for running io_service
                if(m_thread.joinable()){
                    m_thread.join();
                }

                // Destroy the connection object
                m_conn.release();
            }

            bool isConnected(){
                if(m_conn){
                    return m_conn->isConnected();
                }

                return false;
            }

            // Retrieve queue of messages from the server
            tsqueue<owned_message<T>>& Incoming(){
                return m_qmessages_in;
            }

            // send message to server
            void send(const message<T>& msg){
                if(isConnected()){
                    m_conn->send(msg);
                }
            }
        private:

            // Thread safe queue of incoming messages from the server
            tsqueue<owned_message<T>> m_qmessages_in;
        };
    }
}
