if (DARWIN)
    set(OPENSSL_CRYPTO_LIBRARY /usr/local/opt/openssl/lib/libcrypto.dylib)
    set(OPENSSL_SSL_LIBRARY /usr/local/opt/openssl/lib/libssl.dylib)                                                                 
    set(OPENSSL_INCLUDE_PATH  /usr/local/opt/openssl/include)

elseif(WIN32)
    # TODO: windows support - why dos find_path not work here?

    set(OPENSSL_INCLUDE_PATH "c:/local/OpenSSL-Win64/include")
    find_library(OPENSSL_CRYPTO_LIBRARY NAMES libcrypto 
        PATHS "c:/local/OpenSSL-Win64/lib")
    find_library(OPENSSL_SSL_LIBRARY NAMES libssl 
        PATHS "c:/local/OpenSSL-Win64/lib")

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


