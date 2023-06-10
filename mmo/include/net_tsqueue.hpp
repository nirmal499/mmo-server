#pragma once
#include "net_common.hpp"

namespace olc{
    namespace net{
        template<typename T>
        class tsqueue{

        protected:
            std::mutex muxQueue;
            std::deque<T> deQueue;
            std::condition_variable cv;
            std::mutex mux;

        public:
            tsqueue() = default;
            tsqueue(const tsqueue<T>&) = delete;
            virtual ~tsqueue(){
                clear();
            }

        public:
            const T& front(){
                std::scoped_lock lock(muxQueue);
                return deQueue.front();
            }

            const T& back(){
                std::scoped_lock lock(muxQueue);
                return deQueue.back();
            }

            // It might take the ownership of item
            void push_back(const T& item){
                std::scoped_lock lock(muxQueue);
                deQueue.emplace_back(std::move(item));

                // std::cout << "Data Pushed Start\n";
                std::unique_lock<std::mutex> ul(mux);
                cv.notify_one(); // Notifies using condition variable that item is available to be consumed
                // std::cout << "Data Pushed DONE\n";

            }

            // It might take the ownership of item
            void push_front(const T& item){
                std::scoped_lock lock(muxQueue);
                deQueue.emplace_front(std::move(item));

                std::unique_lock<std::mutex> ul(mux);
                cv.notify_one(); // Notifies using condition variable that item is available to be consumed
            }

            // Returns no.of items in Queue
            std::size_t count(){
                std::scoped_lock lock(muxQueue);
                return deQueue.size();
            }

            // Clears Queue
            void clear(){
                std::scoped_lock lock(muxQueue);
                deQueue.clear();
            }

            T pop_front(){
                std::scoped_lock lock(muxQueue);
                auto t = std::move(deQueue.front());
                deQueue.pop_front();

                return t; // RVO
            }

            bool empty(){
                std::scoped_lock lock(muxQueue);
                return deQueue.empty();
            }

            void wait(){
                // wait for item to be available in the queue
                // while loop is to prevent spurious wake
                while(empty()){
                    std::unique_lock<std::mutex> ul(mux);
                    cv.wait(ul);
                }
            }

        };
    }
}
