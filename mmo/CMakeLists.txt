
if(Boost_FOUND)

    # Your project setup
    # add_executable(main main.cpp)

    add_executable(simple_client src/simple_client.cpp)
    add_executable(simple_server src/simple_server.cpp)

    # https://www.boost.org/doc/libs/1_84_0/doc/html/boost_asio/using.html
    # Link against the Boost.System library
    target_link_libraries(simple_client 
        PRIVATE ${Boost_SYSTEM_LIBRARY}
    )

    target_include_directories(simple_client
        PRIVATE ${Boost_INCLUDE_DIRS}
        PRIVATE include
    )

    target_link_libraries(simple_server 
        PRIVATE ${Boost_SYSTEM_LIBRARY}
    )

    target_include_directories(simple_server
        PRIVATE ${Boost_INCLUDE_DIRS}
        PRIVATE include
    )
else()
    message(FATAL_ERROR "Boost not found. Please set Boost_ROOT to the correct path.")
endif()
