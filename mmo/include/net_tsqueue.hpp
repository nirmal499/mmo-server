#pragma once
#include "net_common.hpp"

namespace olc{
    namespace net{
        template<typename T>
        class tsqueue{

        protected:
            // std::mutex muxQueue;
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
                std::scoped_lock lock(mux);
                return deQueue.front();
            }

            const T& back(){
                std::scoped_lock lock(mux);
                return deQueue.back();
            }

            // It might take the ownership of item
            template<typename U>
            void push_back(U&& item){

                {
                    std::unique_lock<std::mutex> ul(mux);
                    deQueue.emplace_back(std::forward<U>(item));
                }
                
                cv.notify_one(); // Notifies using condition variable that item is available to be consumed
            }

            // It might take the ownership of item
            template<typename U>
            void push_front(U&& item){
                {
                    std::unique_lock<std::mutex> ul(mux);
                    deQueue.emplace_front(std::forward<U>(item));
                }

                cv.notify_one(); // Notifies using condition variable that item is available to be consumed
            }

            // Returns no.of items in Queue
            std::size_t count(){
                std::scoped_lock lock(mux);
                return deQueue.size();
            }

            // Clears Queue
            void clear(){
                std::scoped_lock lock(mux);
                deQueue.clear();
            }

            T pop_front(){
                std::unique_lock<std::mutex> gaurd{mux};

                cv.wait(gaurd, [&](){
                    return !deQueue.empty();
                }); /* wait till deQueue is not empty */

                auto t = std::move(deQueue.front());
                deQueue.pop_front();

                return t; // RVO
            }

            bool empty(){
                std::scoped_lock lock(mux);
                return deQueue.empty();
            }

            /*
            void wait(){
                // wait for item to be available in the queue
                // while loop is to prevent spurious wake
                while(empty()){
                    std::unique_lock<std::mutex> ul(mux);
                    cv.wait(ul);
                }
            }
            */

        };
    }
}
