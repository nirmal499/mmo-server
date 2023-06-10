#pragma once
#include "net_common.hpp"

namespace olc{
    namespace net{
        // Message Header is sent at the start of all the messages. The size will indicate the size of the message overall (msg header + msg)

        template<typename T>
        struct message_header{
            T id{};
            uint32_t size = 0;
        };

        template<typename T>
        struct message{
            message_header<T> header{};
            std::vector<uint8_t> body;

            // returns size of entire message packet in bytes
            std::size_t size() const{
                return sizeof(message_header<T>) + body.size();
            }

            friend std::ostream& operator<<(std::ostream& os, const message<T>& msg){
                os << "ID: " << int(msg.header.id) << " Size: " << msg.size();
                return os;
            }

            // Pushes any POD-like data into the message buffer
            template<typename DataType>
            friend message<T>& operator<<(message<T>& msg,const DataType& data){
                // Check that the type of the data being pushed is trivially copyable
                static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed");

                // Cache the current size of vector, as this will be the point we insert the data
                std::size_t i = msg.body.size();

                // Resize the vector by the size of the data being pushed
                msg.body.resize(msg.body.size()+sizeof(DataType));

                // Physically copy the data into the newly resized vector space
                // memcpy(dest, src, count)
                std::memcpy(msg.body.data()+i,&data,sizeof(DataType));

                // Recalculate the message size
                // msg.header.size = msg.size();
                msg.header.size = msg.body.size();

                return msg;

            }

            template<typename DataType>
            friend message<T>& operator>>(message<T>& msg,DataType& data){
                // Check that the type of the data being pushed is trivially copyable
                static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be poped");

                // Cache the location towards the end of the vector where the pulled data starts
                std::size_t i = msg.body.size() - sizeof(DataType);

                // Physically copy the data from the vector into the user variable
                // memcpy(dest, src, count)
                std::memcpy(&data, msg.body.data() + i ,sizeof(DataType));

                // Shrink the vector to remove read bytes
                msg.body.resize(i);

                // Recalculate the message size
                // msg.header.size = msg.size();
                msg.header.size = msg.body.size();

                return msg;

            }



        };

        template<typename T>
        class connection;

        // This owned_message is for the server
        // The server application needs to know where that msg came from, incase it needs to respond back to the same client
        template<typename T>
        struct owned_message{
            std::shared_ptr<connection<T>> remote = nullptr;
            message<T> msg;

            friend std::ostream& operator<<(std::ostream& os,const owned_message<T>& msg){
                os << msg.msg;
                return os;
            }

        };
    }
}
