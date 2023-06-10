#pragma once

#include "net_common.hpp"
#include "net_tsqueue.hpp"
#include "net_message.hpp"

namespace olc{
    namespace net{

        using namespace boost;

        // Forward declare
        template<typename T>
        class server_interface;

        // std::enable_shared_from_this will allow us to create a shared pointer to "this"
        // rather than a raw pointer
        template<typename T>
        class connection : public std::enable_shared_from_this<connection<T>>{
        public:
            enum class owner{
                server,
                client
            };

        protected:
            // Each connection has a unique socket to a remote
            asio::ip::tcp::socket m_socket;

            // Container of active validated connections
            std::deque<std::shared_ptr<connection<T>>> m_dequeConnections;

            // This io_service is shared with the whole asio instance for connection
            // In case of server this asio instance will be shared among all the connection
            // In case of client, each client will have one connection
            asio::io_service& m_ios;

            // This queue holds all messages to be sent to the remote side of this connection
            // NOTE: It is tsqueue of message<T>
            tsqueue<message<T>> m_qmessages_out;

            // This queue holds all messages that have been received from the remote side of the connection
            // NOTE: It is a reference as the "owner" of this connection is expected to provide a queue
            // NOTE: It is tsqueue of owned_message<T>
            tsqueue<owned_message<T>>& m_qmessages_in;

            // The "owner" decides how the connection class behaves
            owner m_OwnerType = owner::server;

            uint32_t id = 0;

            message<T> m_temp;

            // Handshake validation
            uint64_t m_handshakeout = 0;
            uint64_t m_handshakein = 0;
            uint64_t m_handshakecheck = 0;

        public:
            uint32_t getID() const{
                return id;
            }

            connection(owner parent,
                       asio::io_service& ios,
                       asio::ip::tcp::socket socket,
                       tsqueue<owned_message<T>>& qIncoming)
                : m_ios(ios),
                  m_socket(std::move(socket)),
                  m_qmessages_in(qIncoming),
                  m_OwnerType(parent)
            {
                if(m_OwnerType == owner::server){
                    // Server

                    // Connection is Server -> Client, construct random data for the client
                    // to transform and send back for validation
                    m_handshakeout = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

                    // Pre-Calculate the result for checking when the client responds
                    m_handshakecheck = scramble(m_handshakeout);

                }else{
                    // Client

                    m_handshakein = 0;
                    m_handshakecheck = 0;
                }
            }
            virtual ~connection(){}

            // This is for the client only, server does not need connectToServer function
            void connectToServer(const asio::ip::tcp::resolver::results_type& endpoints){
                // Only clients can connect to the server
                if(m_OwnerType == owner::client){
                    // asio attempts to connect to an endpoint
                    asio::async_connect(m_socket,endpoints,[this](system::error_code ec,
                                                                  asio::ip::tcp::endpoint endpoint){
                        if(!ec){
                            // Was: readHeader();

                            // First thing server will do is send packet to be validated
                            // so wait for that and respond
                            readValidation();
                        }
                    });
                }
            }
            void disconnect(){
                if(isConnected()){
                    asio::post(m_ios,[this](){
                        m_socket.close();
                    });
                }
            }
            bool isConnected() const{
                return m_socket.is_open();
            }

            void connectToClient(int IDofClient){
                if(m_OwnerType == owner::server){
                    if(m_socket.is_open()){
                        id = IDofClient;
                        // Was: readHeader();

                        // A client has attempted to connect to the server, but we wish
                        // the client to first validate itself, so first write out the
                        // handshake data to be validated
                        writeValidation();

                        // Next, issue a task to sit and wait asynchronously for precisely
                        // the validation data send back from the client
                        readValidation(server);
                    }
                }
            }

           void send(const message<T>& msg){
                // Submits a completion token or function object for execution
                asio::post(m_ios,[this,msg](){

                    bool not_empty_flag = !m_qmessages_out.empty();
                    m_qmessages_out.push_back(msg);

                    // Basically we want to avoid another writeHeader workload to the asio_io_service
                    // that already has writeHeader or writeBody workload
                    // So, checking the state of q_message_out queue before hand we can be sure
                    // that asio already busy doing writing messages or we need to restart WRITE messaging process
                    if(!not_empty_flag){
                        // Since our writeHeader relies on the m_qmessages_out
                        // Thats why we pushed the message first
                        writeHeader();

                    }
                });
            }

        private:
            // ASYNC - read a message header
            void readHeader(){
                asio::async_read(m_socket,asio::buffer(&m_temp.header,sizeof(message_header<T>)),
                                 [this](system::error_code ec,std::size_t length){
                                     if(!ec){
                                         if(m_temp.header.size > 0){
                                             // Here the header size if not 0.
                                             // Header size represents the size of the message body

                                             // Allocate enough space for message body
                                             m_temp.body.resize(m_temp.header.size);

                                             readBody();
                                         }
                                         else{
                                             // Here m_temp.header.size is 0
                                             // We m_temp.header.size denotes the size of the msg body
                                             // We can have msg which have NO body e.g CustomMsgTypes::ServerAccept
                                             addToIncomingMsgQ();
                                         }
                                     }else{
                                         std::cout << "[" << id << "] Read Header Fail\n";
                                         m_socket.close();
                                     }
                                 });
            }

            // ASYNC - read a message body
            void readBody(){
                asio::async_read(m_socket,asio::buffer(m_temp.body.data(),m_temp.body.size()),
                        [this](system::error_code ec,std::size_t length){
                            if(!ec){
                                addToIncomingMsgQ();
                            }else{
                                std::cout << "[" << id << "] Read Body Fail\n";
                                m_socket.close();
                            }
                        });
            }

            // ASYNC - write a message header
            void writeHeader(){
                asio::async_write(m_socket,asio::buffer(
                                                        &m_qmessages_out.front().header,
                                                        sizeof(message_header<T>)),
                                  [this](system::error_code ec,std::size_t length){
                    if(!ec){
                        // writeBody();
                        if(m_qmessages_out.front().body.size() > 0){
                            writeBody();
                        }else{

                            // Here the msg body is 0
                            // We can have a msg which have NO body e.g CustomMsgTypes::ServerAccept
                            m_qmessages_out.pop_front();
                            if(!m_qmessages_out.empty()){
                                writeHeader();
                            }
                        }
                    }else{
                        std::cout << "[" << id << "] Write Header Fail\n";
                        m_socket.close();
                    }
                });
            }

            // ASYNC - write a message body
            void writeBody(){
                asio::async_write(m_socket,asio::buffer(
                                                        m_qmessages_out.front().body.data(),
                                                        m_qmessages_out.front().body.size()),
                                  [this](system::error_code ec,std::size_t length){
                    if(!ec){
                        m_qmessages_out.pop_front();
                        if(!m_qmessages_out.empty()){
                            writeHeader();
                        }
                    }else{
                        std::cout << "[" << id << "] Wite Body Fail\n";
                        m_socket.close();
                    }
                });
            }

            void addToIncomingMsgQ(){
                if(m_OwnerType == owner::server){
                    // We are constructing owned_message while pushing
                    m_qmessages_in.push_back({ this->shared_from_this() , m_temp });
                }else{
                    m_qmessages_in.push_back({ nullptr , m_temp });
                }

                readHeader();
            }

            // Our function for validation
            uint64_t scramble(uint64_t nInput){
                uint64_t out = nInput ^ 0xIAMDEADFROMINSIDE;

                out = (out & 0xYOUWILLDIEALONE) >> 4 | (out & 0xDEALWITHITYOUDESERVERIT) << 4;
                return out ^ 0xUSELESSPEICEOFCRAP;
            }

            // ASYNC - Used by both client and server to write validation packet
            void writeValidation(){
                asio::async_write(m_socket,asio::buffer(&m_handshakeout,sizeof(uint64_t)),
                                  [this](boost::system::error_code ec,std::size_t length){
                                      if(!ec){
                                          // Validation data sent, clients should sit and wait
                                          // for a response
                                          if(m_OwnerType == owner::client){
                                              readHeader();
                                          }

                                          // You maybe wandering that for server also we need to do
                                          // readHeader or readValidation
                                      }else{
                                          m_socket.close();
                                      }
                                  });
            }

            // ASYNC -
            void readValidation(olc::net::server_interface<T>* server = nullptr){
                asio::async_read(m_socket,asio::buffer(&m_handshakein,sizeof(uint64_t)),
                                 [this](boost::system::error_code ec,std::size_t length){
                                     if(!ec){
                                         if(m_OwnerType == owner::server){
                                             if(m_handshakein == m_handshakecheck){
                                                 // Client has provided valid solution, so allow it connect properly
                                                 std::cout << "Client validated\n";
                                                 server->onClientValidated(this->shared_from_this());

                                                 // Sit waiting to receive data now.
                                                 readHeader();
                                             }else{
                                                 // Client gave incorrect data,so disconnect
                                                 std::cout << "Client Disconnected (Fail Validation)\n";
                                                 m_socket.close();
                                             }
                                         }else{
                                             // Connection is a client, so solve puzzle
                                             m_handshakeout = scramble(m_handshakein);

                                             // Write the result
                                             writeValidation();
                                         }
                                     }else{
                                         // Some failure has occurred.
                                         std::cout << "Client Disconnected (readValidation)\n";
                                         m_socket.close();
                                     }

                                 });
            }


        };
    }
}
