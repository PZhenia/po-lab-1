cmake_minimum_required(VERSION 3.10.0)
project(lab1 VERSION 0.1.0 LANGUAGES C CXX)

add_executable(lab1 main.cpp)

set(HTML_FILES 
    "index.html" 
    "page2.html"
)

foreach(HTML_FILE ${HTML_FILES})
    configure_file(${HTML_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${HTML_FILE} COPYONLY)
endforeach()