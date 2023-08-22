if (TARGET openssl)
    return()
endif()
if (DARWIN)
    find_path(OPENSSL_INCLUDE_PATH NAMES openssl/sha.h
        PATHS $ENV{OPENSSL_ROOT_DIR}/include /usr/include /usr/local/include /usr/local/opt/openssl/include)
    find_library(OPENSSL_CRYPTO_LIBRARY NAMES crypto 
        PATHS $ENV{OPENSSL_ROOT_DIR}/lib /usr/local/opt/openssl/lib /usr/lib /usr/local/lib)
    find_library(OPENSSL_SSL_LIBRARY NAMES ssl 
        PATHS $ENV{OPENSSL_ROOT_DIR}/lib /usr/local/opt/openssl/lib /usr/lib /usr/local/lib)
elseif(WIN32)
    find_path(OPENSSL_INCLUDE_PATH NAMES openssl/sha.h
        PATHS "c:/local/OpenSSL-Win64/include"
              "c:/Program Files/OpenSSL-Win64/include")
    find_library(OPENSSL_CRYPTO_LIBRARY NAMES libcrypto 
        PATHS "c:/local/OpenSSL-Win64/lib"
              "c:/Program Files/OpenSSL-Win64/lib")
    find_library(OPENSSL_SSL_LIBRARY NAMES libssl 
        PATHS "c:/local/OpenSSL-Win64/lib"
              "c:/Program Files/OpenSSL-Win64/lib")

else()
    find_path(OPENSSL_INCLUDE_PATH NAMES openssl/sha.h
        PATHS /usr/include /usr/local/include /usr/local/opt/openssl/include)
    find_library(OPENSSL_CRYPTO_LIBRARY NAMES crypto 
        PATHS /usr/local/opt/openssl/lib /usr/lib /usr/local/lib)
    find_library(OPENSSL_SSL_LIBRARY NAMES ssl 
        PATHS /usr/local/opt/openssl/lib /usr/lib /usr/local/lib)
endif()

if (OPENSSL_CRYPTO_LIBRARY STREQUAL "OPENSSL_CRYPTO_LIBRARY-NOTFOUND")
    message(FATAL_ERROR "Could not find libcrypto")
endif()
if (OPENSSL_SSL_LIBRARY STREQUAL "OPENSSL_SSL_LIBRARY-NOTFOUND")
    message(FATAL_ERROR "Could not find libssl")
endif()


add_library(openssl INTERFACE)
target_include_directories(openssl INTERFACE ${OPENSSL_INCLUDE_PATH})
target_link_libraries(openssl INTERFACE ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
target_compile_definitions(openssl INTERFACE -DOPENSSL_SUPPRESS_DEPRECATED)

