#pragma once

#include "net_common.hpp"
#include "net_tsqueue.hpp"
#include "net_message.hpp"
#include "net_connection.hpp"

namespace olc{
    namespace net{
        template<typename T>
        class server_interface{
        public:
            server_interface(uint16_t port):
                m_acceptor(m_ios,asio::ip::tcp::endpoint(asio::ip::tcp::v4(),port))
            {}

            virtual ~server_interface(){
                stop();
            }

            bool start(){
                try{
                    waitForClientConnection();

                    m_thread = std::thread([this](){
                        m_ios.run();
                    });

                }catch(std::exception& e){
                    std::cerr << "[SERVER] Exception: " << e.what() << "\n";
                    return false;
                }

                std::cout << "[SERVER] started\n";
                return true;
            }

            void stop(){

                // Stop the io_service
                m_ios.stop();

                if(m_thread.joinable()){

                    // Join the Thread that was tasked to run io_service
                    m_thread.join();
                }

                std::cout << "[SERVER] Stopped\n";
            }

            // ASYNC - Instruct asio to wait for connection
            void waitForClientConnection(){

                // Whenever the async_accept is fired the passed lambda function is called
                m_acceptor.async_accept([this](system::error_code ec,asio::ip::tcp::socket socket){
                    if(!ec){
                        std::cout << "[SERVER] new connection : " << socket.remote_endpoint() << "\n";

                        std::shared_ptr<connection<T>> newconn =
                            std::make_shared<connection<T>>(connection<T>::owner::server,
                                                            m_ios,
                                                            std::move(socket),
                                                            m_qmessages_in);
                        // ios and m_qmessages_in are passed as reference

                        // Give the server a chance to deny connection
                        if(onClientConnect(newconn)){
                            // Connection allowed, so add to container of new connections
                            m_dequeConnections.push_back(std::move(newconn));

                            // m_dequeConnections.back()->connectToClient(nIDCounter++);
                            m_dequeConnections.back()->connectToClient(this,nIDCounter++);

                            std::cout << "[" << m_dequeConnections.back()->getID() << "] Connection Approved\n";
                        }else{
                            std::cout << "[-------------] Connection Denied\n";
                        }

                        // If the connection is rejected by the server, then the pointer will go out of scope
                        // and the memory will be deallocated as it a smart pointer
                    }else{
                        std::cout << "[SERVER] new connection error " << ec.message() << "\n";
                    }

                    // Simply wait for another connection
                    waitForClientConnection();
                });
            }

            // Send a message to a specific client
            void messageAClient(std::shared_ptr<connection<T>> client, const message<T>& msg){
                if(client && client->isConnected()){
                    client->send(msg);
                }else{
                    // On of the limitations using the TCP protocol is we don't necessarily know when a client
                    // has disconnected, so it is only when we try to manipulate the client and that manipulation
                    // fails then we know that client is disconnected

                    onClientDisconnect(client);
                    client.reset(); // Release the memory allocated
                    m_dequeConnections.erase(std::remove(m_dequeConnections.begin(),
                                                         m_dequeConnections.end(),
                                                         client),
                                             m_dequeConnections.end());
                }
            }

            // Send message to all clients
            void messageAllClients(const message<T>& msg,std::shared_ptr<connection<T>> pIgnoreClient = nullptr){
                bool bInvalidClientExists = false;

                for(auto& client: m_dequeConnections){

                    // Check if client is connected
                    if(client && client->isConnected()){

                        // Client is connected
                        if(client != pIgnoreClient){
                            client->send(msg);
                        }
                    }else{

                        // Client couldn't be contacted, so assume is has disconnected
                        onClientDisconnect(client);
                        client.reset(); // Here the client is being set to nullptr

                        bInvalidClientExists = true;

                    }
                }

                if(bInvalidClientExists){
                    m_dequeConnections.erase(std::remove(m_dequeConnections.begin(),
                                                         m_dequeConnections.end(),
                                                         nullptr),
                                             m_dequeConnections.end());
                }
            }

            // This function gives client the ability to handle specific no.of messages
            // Force server to respond to incoming messages
            void update(std::size_t nMaxMessages = -1){
                /* No need to wait becoz we are apply condition variable in tsqueue for waiting during pop_front() */
                
                /*
                    if(wait){
                        // std::cout << "Waiting START\n";
                        m_qmessages_in.wait(); // wait for the item to be pushed in the queue
                        // std::cout << "Waiting DONE\n";
                    }
                */

                // Setting nMaxMessages to maximum number becoz it a unsigned variable


                // Process as many requests as you can up to the value specified
                std::size_t nMessagesCount = 0;
                while(nMessagesCount < nMaxMessages && !m_qmessages_in.empty()){
                    // Grab the front message
                    auto msg = m_qmessages_in.pop_front();

                    // Pass to message handler
                    // std::cout << "Message arrived from client\n";
                    onMessageArrival(msg.remote,msg.msg);

                    nMessagesCount++;
                }
            }

        protected:
            // Called when a client connects, you can veto the connection by returning false
            virtual bool onClientConnect(std::shared_ptr<connection<T>> client){
                return true;
            }

            // Called when a client appears to have disconnected
            virtual void onClientDisconnect(std::shared_ptr<connection<T>> client){}

            // Called when message arrives
            virtual void onMessageArrival(std::shared_ptr<connection<T>> client,const message<T>& msg){}

        public:
            // Called when a client is validated
            virtual void onClientValidated(std::shared_ptr<connection<T>> client){
                
            }

        protected:
            // Thread safe queue for incoming message packets
            tsqueue<owned_message<T>> m_qmessages_in;

            asio::io_service m_ios;
            std::thread m_thread; // It will run the io_service

            std::deque<std::shared_ptr<connection<T>>> m_dequeConnections;
            asio::ip::tcp::acceptor m_acceptor;

            // Clients will be identified in the "wider system" via an ID
            uint32_t nIDCounter = 10000;
        };
    }
}
