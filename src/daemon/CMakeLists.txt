cmake_minimum_required(VERSION 3.13)
 
set(ORCA_DAEMON_SOURCES
    main.cpp                    
)
 
add_executable(orca-daemon ${ORCA_DAEMON_SOURCES}
 server.hpp
 server.cpp
 paths.hpp
 config.hpp
 config.cpp
 response.hpp
 data_interpreter.hpp
 data_interpreter.cpp
 orca_session.hpp
 orca_response.hpp
 orca_session.cpp
 nanosvg_impl.cpp
 utils/build_printer_preset_string.hpp
 utils/build_printer_preset_string.cpp
 utils/create_profiles_directories.hpp
 utils/create_profiles_directories.cpp
 )
 
target_compile_features(orca-daemon PRIVATE cxx_std_17)
 
target_include_directories(orca-daemon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src     # enables #include "libslic3r/..."
)
 
target_link_libraries(orca-daemon PRIVATE
    libslic3r
    boost_headeronly
    cereal::cereal
    nlohmann_json               # header-only, already vendored in deps_src
    libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
)
 
target_compile_definitions(orca-daemon PRIVATE
    BOOST_ALL_NO_LIB
    BOOST_USE_WINAPI_VERSION=0x602
    BOOST_SYSTEM_USE_UTF8
)
 
# Platform-specific bits (sockets, threads, dynamic loading).
if (WIN32)
    target_link_libraries(orca-daemon PRIVATE ws2_32)
elseif (APPLE)
    target_link_libraries(orca-daemon PRIVATE
        "-framework CoreFoundation"
        "-framework IOKit"
    )
else ()
    find_package(Threads REQUIRED)
    target_link_libraries(orca-daemon PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
endif ()
 
set_target_properties(orca-daemon PROPERTIES OUTPUT_NAME "orca-daemon")
 
install(TARGETS orca-daemon RUNTIME DESTINATION bin)
