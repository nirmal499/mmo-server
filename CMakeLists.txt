cmake_minimum_required(VERSION 3.22.1)

project(mmo_project)

set(Boost_ROOT "/home/nbaskey/my_opt/boost_libs/boost_1_84_0/")
find_package(Boost 1.84 REQUIRED COMPONENTS)

add_subdirectory(mmo)
