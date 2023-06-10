# cmake_minimum_required(VERSION 3.22.1)

# project(mmo_project)

# set(Boost_USE_STATIC_LIBS ON)

# https://stackoverflow.com/questions/33091417/could-not-find-the-following-boost-libraries-boost-asio
# find_package(Boost 1.54 REQUIRED COMPONENTS asio)

if(Boost_USE_STATIC_LIBS)
   message(STATUS "Linking against boost static libraries")
else()
   message(STATUS "Linking against boost dynamic libraries")
endif()

add_executable(main test3.cpp)

# target_link_libraries(main PUBLIC Boost::asio)
