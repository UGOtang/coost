#pragma once

#include "../def.h"
#include "../byte_order.h"
#include "../fastring.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h> // for inet_ntop...
#include <MSWSock.h>
#pragma comment(lib, "Ws2_32.lib")

typedef SOCKET sock_t;

#else
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>  // basic socket api, struct linger
#include <netinet/in.h>  // for struct sockaddr_in
#include <netinet/tcp.h> // for TCP_NODELAY...
#include <arpa/inet.h>   // for inet_ntop...
#include <netdb.h>       // getaddrinfo, gethostby...

typedef int sock_t;
#endif

namespace co {

/** 
 * create a socket suitable for coroutine programing
 * 
 * @param domain  address family, AF_INET, AF_INET6, etc.
 * @param type    socket type, SOCK_STREAM, SOCK_DGRAM, etc.
 * @param proto   protocol, IPPROTO_TCP, IPPROTO_UDP, etc.
 * 
 * @return        a non-blocking (also overlapped on windows) socket on success, 
 *                or -1 on error.
 */
sock_t socket(int domain, int type, int proto);

/**
 * create a TCP socket suitable for coroutine programing
 * 
 * @param domain  address family, AF_INET, AF_INET6, etc.
 *                default: AF_INET.
 * 
 * @return        a non-blocking (also overlapped on windows) socket on success, 
 *                or -1 on error.
 */
inline sock_t tcp_socket(int domain=AF_INET) {
    return co::socket(domain, SOCK_STREAM, IPPROTO_TCP);
}

/**
 * create a UDP socket suitable for coroutine programing
 * 
 * @param domain  address family, AF_INET, AF_INET6, etc.
 *                default: AF_INET.
 * 
 * @return        a non-blocking (also overlapped on windows) socket on success, 
 *                or -1 on error.
 */
inline sock_t udp_socket(int domain=AF_INET) {
    return co::socket(domain, SOCK_DGRAM, IPPROTO_UDP);
}

/**
 * close a socket 
 *   - A socket MUST be closed in the same thread that performed the I/O operation, 
 *     usually, in the coroutine where the user called recv(), send(), etc. 
 *   - EINTR has been handled internally. The user need not consider about it. 
 *     
 * @param fd  a non-blocking (also overlapped on windows) socket.
 * @param ms  if ms > 0, the socket will be closed ms milliseconds later. 
 *            default: 0.
 * 
 * @return    0 on success, -1 on error.
 */
int close(sock_t fd, int ms=0);

/**
 * shutdown a socket 
 *   - Like the close(), shutdown() MUST be called in the same thread that performed 
 *     the I/O operation. 
 * 
 * @param fd  a non-blocking (also overlapped on windows) socket.
 * @param c   'r' for SHUT_RD, 'w' for SHUT_WR, 'b' for SHUT_RDWR. 
 *            default: 'b'.
 * 
 * @return    0 on success, -1 on error.
 */
int shutdown(sock_t fd, char c='b');

/**
 * bind an address to a socket
 * 
 * @param fd       a non-blocking (also overlapped on windows) socket.
 * @param addr     a pointer to struct sockaddr, sockaddr_in or sockaddr_in6.
 * @param addrlen  size of the structure pointed to by addr.
 * 
 * @return         0 on success, -1 on error.
 */
int bind(sock_t fd, const void* addr, int addrlen);

/**
 * listen on a socket
 * 
 * @param fd       a non-blocking (also overlapped on windows) socket.
 * @param backlog  maximum length of the queue for pending connections.
 *                 default: 1024.
 * 
 * @return         0 on success, -1 on error.
 */
int listen(sock_t fd, int backlog=1024);

/**
 * accept a connection on a socket 
 *   - It MUST be called in a coroutine. 
 *   - accept() blocks until a connection was present or any error occured. 
 * 
 * @param fd       a non-blocking (also overlapped on windows) socket.
 * @param addr     a pointer to struct sockaddr, sockaddr_in or sockaddr_in6.
 * @param addrlen  the user MUST initialize it with the size of the structure pointed to 
 *                 by addr; on return it contains the actual size of the peer address.
 * 
 * @return         a non-blocking (also overlapped on windows) socket on success,  
 *                 or -1 on error.
 */
sock_t accept(sock_t fd, void* addr, int* addrlen);

/**
 * connect to an address 
 *   - It MUST be called in a coroutine. 
 *   - connect() blocks until the connection was done or timeout, or any error occured. 
 *   - The errno will be set to ETIMEDOUT on timeout, call co::error() to get the errno, 
 *     or simply call co::timeout() to check whether it has timed out. 
 * 
 * @param fd       a non-blocking (also overlapped on windows) socket.
 * @param addr     a pointer to struct sockaddr, sockaddr_in or sockaddr_in6.
 * @param addrlen  size of the structure pointed to by addr.
 * @param ms       timeout in milliseconds, if ms < 0, it will never time out. 
 *                 default: -1.
 * 
 * @return         0 on success, -1 on timeout or error.
 */
int connect(sock_t fd, const void* addr, int addrlen, int ms=-1);

/**
 * recv data from a socket 
 *   - It MUST be called in a coroutine. 
 *   - It blocks until any data recieved or timeout, or any error occured. 
 *   - The errno will be set to ETIMEDOUT on timeout, call co::error() to get the errno, 
 *     or simply call co::timeout() to check whether it has timed out. 
 * 
 * @param fd   a non-blocking (also overlapped on windows) socket.
 * @param buf  a pointer to the buffer to recieve the data.
 * @param n    size of the buffer.
 * @param ms   timeout in milliseconds, if ms < 0, it will never time out.
 *             default: -1.
 * 
 * @return     bytes recieved on success, -1 on timeout or error, 0 will be returned 
 *             if fd is a stream socket and the peer has closed the connection.
 */
int recv(sock_t fd, void* buf, int n, int ms=-1);

/**
 * recv n bytes from a socket 
 *   - It MUST be called in a coroutine. 
 *   - It blocks until all the n bytes are recieved or timeout, or any error occured. 
 *   - The errno will be set to ETIMEDOUT on timeout, call co::error() to get the errno, 
 *     or simply call co::timeout() to check whether it has timed out. 
 * 
 * @param fd   a non-blocking (also overlapped on windows) socket, 
 *             it MUST be a stream socket, usually a TCP socket.
 * @param buf  a pointer to the buffer to recieve the data.
 * @param n    bytes to be recieved.
 * @param ms   timeout in milliseconds, if ms < 0, it will never time out.
 *             default: -1.
 * 
 * @return     n on success, -1 on timeout or error, 0 will be returned if the peer 
 *             close the connection.
 */
int recvn(sock_t fd, void* buf, int n, int ms=-1);

/**
 * recv data from a socket 
 *   - It MUST be called in a coroutine. 
 *   - It blocks until any data recieved or timeout, or any error occured. 
 *   - The errno will be set to ETIMEDOUT on timeout, call co::error() to get the errno, 
 *     or simply call co::timeout() to check whether it has timed out. 
 *   - Set src_addr and addrlen to NULL if the user is not interested in the source address. 
 * 
 * @param fd        a non-blocking (also overlapped on windows) socket.
 * @param buf       a pointer to the buffer to recieve the data.
 * @param n         size of the buffer.
 * @param src_addr  a pointer to struct sockaddr, sockaddr_in or sockaddr_in6, the source address 
 *                  will be placed in the buffer pointed to by src_addr.
 * @param addrlen   it should be initialized to the size of the buffer pointed to by src_addr, on 
 *                  return, it contains the actual size of the source address.
 * @param ms        timeout in milliseconds, if ms < 0, it will never time out.
 *                  default: -1.
 * 
 * @return          bytes recieved on success, -1 on timeout or error, 0 will be returned 
 *                  if fd is a stream socket and the peer has closed the connection.
 */
int recvfrom(sock_t fd, void* buf, int n, void* src_addr, int* addrlen, int ms=-1);

/**
 * send n bytes on a socket 
 *   - It MUST be called in a coroutine. 
 *   - It blocks until all the n bytes are sent or timeout, or any error occured. 
 *   - The errno will be set to ETIMEDOUT on timeout, call co::error() to get the errno, 
 *     or simply call co::timeout() to check whether it has timed out. 
 * 
 * @param fd   a non-blocking (also overlapped on windows) socket.
 * @param buf  a pointer to a buffer of the data to be sent.
 * @param n    size of the data.
 * @param ms   timeout in milliseconds, if ms < 0, it will never time out. 
 *             default: -1.
 * 
 * @return     n on success, or -1 on error. 
 */
int send(sock_t fd, const void* buf, int n, int ms=-1);

/**
 * send n bytes on a socket 
 *   - It MUST be called in a coroutine. 
 *   - It blocks until all the n bytes are sent or timeout, or any error occured. 
 *   - The errno will be set to ETIMEDOUT on timeout, call co::error() to get the errno, 
 *     or simply call co::timeout() to check whether it has timed out. 
 * 
 * @param fd        a non-blocking (also overlapped on windows) socket.
 * @param buf       a pointer to a buffer of the data to be sent.
 * @param n         size of the data, n MUST be no more than 65507 if fd is an udp socket.
 * @param dst_addr  a pointer to struct sockaddr, sockaddr_in or sockaddr_in6, which contains 
 *                  the destination address, SHOULD be NULL if fd is a tcp socket.
 * @param addrlen   size of the destination address.
 * @param ms        timeout in milliseconds, if ms < 0, it will never time out. 
 *                  default: -1.
 * 
 * @return          n on success, -1 on timeout or error. 
 */
int sendto(sock_t fd, const void* buf, int n, const void* dst_addr, int addrlen, int ms=-1);

#ifdef _WIN32
/**
 * get options on a socket, man getsockopt for details.
 */
inline int getsockopt(sock_t fd, int lv, int opt, void* optval, int* optlen) {
    return ::getsockopt(fd, lv, opt, (char*)optval, optlen);
}

/**
 * set options on a socket, man setsockopt for details. 
 */
inline int setsockopt(sock_t fd, int lv, int opt, const void* optval, int optlen) {
    return ::setsockopt(fd, lv, opt, (const char*)optval, optlen);
}

#else
/**
 * get options on a socket, man getsockopt for details.
 */
inline int getsockopt(sock_t fd, int lv, int opt, void* optval, int* optlen) {
    return ::getsockopt(fd, lv, opt, optval, (socklen_t*)optlen);
}

/**
 * set options on a socket, man setsockopt for details. 
 */
inline int setsockopt(sock_t fd, int lv, int opt, const void* optval, int optlen) {
    return ::setsockopt(fd, lv, opt, optval, (socklen_t)optlen);
}
#endif

/**
 * set option SO_REUSEADDR on a socket
 */
inline void set_reuseaddr(sock_t fd) {
    int v = 1;
    co::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
}

/**
 * set send buffer size for a socket 
 *   - It MUST be called before the socket is connected. 
 */
inline void set_send_buffer_size(sock_t fd, int n) {
    co::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n));
}

/**
 * set recv buffer size for a socket 
 *   - It MUST be called before the socket is connected. 
 */
inline void set_recv_buffer_size(sock_t fd, int n) {
    co::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n));
}

/**
 * set option TCP_NODELAY on a TCP socket  
 */
inline void set_tcp_nodelay(sock_t fd) {
    int v = 1;
    co::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v));
}

/**
 * set option SO_KEEPALIVE on a TCP socket 
 */
inline void set_tcp_keepalive(sock_t fd) {
    int v = 1;
    co::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &v, sizeof(v));
}

/**
 * reset a TCP connection 
 *   - It MUST be called in the same thread that performed the IO operation. 
 *   - It is usually used in a server to avoid TIME_WAIT status. 
 *     
 * @param fd  a non-blocking (also overlapped on windows) socket.
 * @param ms  if ms > 0, the socket will be closed ms milliseconds later.
 *            default: 0.
 * 
 * @return    0 on success, -1 on error.
 */
inline int reset_tcp_socket(sock_t fd, int ms=0) {
    struct linger v = { 1, 0 };
    co::setsockopt(fd, SOL_SOCKET, SO_LINGER, &v, sizeof(v));
    return co::close(fd, ms);
}

#ifdef _WIN32
/**
 * set option O_NONBLOCK on a socket 
 */
inline void set_nonblock(sock_t fd) {
   unsigned long mode = 1;
   if (ioctlsocket(fd, FIONBIO, &mode) != 0) {
       printf("set nonblock failed\n");
   }
}
#else
/**
 * set option O_NONBLOCK on a socket 
 */
inline void set_nonblock(sock_t fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

/**
 * set option FD_CLOEXEC on a socket 
 */
inline void set_cloexec(sock_t fd) {
    fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
}
#endif

/**
 * fill in an ipv4 address with ip & port 
 * 
 * @param addr  a pointer to an ipv4 address.
 * @param ip    string like: "127.0.0.1".
 * @param port  a value from 1 to 65535.
 * 
 * @return      true on success, otherwise false.
 */
inline bool init_ip_addr(struct sockaddr_in* addr, const char* ip, int port) {
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = hton16((uint16)port);
    return inet_pton(AF_INET, ip, &addr->sin_addr) == 1;
}

/**
 * fill in an ipv6 address with ip & port
 * 
 * @param addr  a pointer to an ipv6 address.
 * @param ip    string like: "::1".
 * @param port  a value from 1 to 65535.
 * 
 * @return      true on success, otherwise false.
 */
inline bool init_ip_addr(struct sockaddr_in6* addr, const char* ip, int port) {
    memset(addr, 0, sizeof(*addr));
    addr->sin6_family = AF_INET6;
    addr->sin6_port = hton16((uint16) port);
    return inet_pton(AF_INET6, ip, &addr->sin6_addr) == 1;
}

/**
 * get ip string of an ipv4 address 
 */
inline fastring ip_str(const struct sockaddr_in* addr) {
    char s[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &addr->sin_addr, s, sizeof(s));
    return fastring(s);
}

/**
 * get ip string of an ipv6 address
 */
inline fastring ip_str(const struct sockaddr_in6* addr) {
    char s[INET6_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET6, &addr->sin6_addr, s, sizeof(s));
    return fastring(s);
}

/**
 * convert an ipv4 address to a string 
 *
 * @return  a string in format "ip:port"
 */
inline fastring to_string(const struct sockaddr_in* addr) {
    char s[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &addr->sin_addr, s, sizeof(s));
    const size_t n = strlen(s);
    return std::move(fastring(n + 8).append(s, n).append(':') << ntoh16(addr->sin_port));
}

/**
 * convert an ipv6 address to a string 
 *
 * @return  a string in format: "ip:port"
 */
inline fastring to_string(const struct sockaddr_in6* addr) {
    char s[INET6_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET6, &addr->sin6_addr, s, sizeof(s));
    const size_t n = strlen(s);
    return std::move(fastring(n + 8).append(s, n).append(':') << ntoh16(addr->sin6_port));
}

/**
 * convert an ip address to a string 
 * 
 * @param addr  a pointer to struct sockaddr.
 * @param len   length of the addr, sizeof(sockaddr_in) or sizeof(sockaddr_in6).
 */
inline fastring to_string(const void* addr, int len) {
    if (len == sizeof(sockaddr_in)) return to_string((const sockaddr_in*)addr);
    return to_string((const struct sockaddr_in6*)addr);
}

/**
 * get peer address of a connected socket 
 * 
 * @return  a string in format: "ip:port", or an empty string on error.
 */
inline fastring peer(sock_t fd) {
    union {
        struct sockaddr_in  v4;
        struct sockaddr_in6 v6;
    } addr;
    int addrlen = sizeof(addr);
    const int r = getpeername(fd, (sockaddr*)&addr, (socklen_t*)&addrlen);
    if (r == 0) {
        if (addrlen == sizeof(addr.v4)) return co::to_string(&addr.v4);
        if (addrlen == sizeof(addr.v6)) return co::to_string(&addr.v6);
    }
    return fastring();
}

#ifdef _WIN32
/**
 * get the last error number
 */
inline int error() { return WSAGetLastError(); }

/**
 * set the last error number
 */
inline void set_last_error(int err) { WSASetLastError(err); }
#else
/**
 * get the last error number
 */
inline int error() { return errno; }

/**
 * set the last error number
 */
inline void set_last_error(int err) { errno = err; }
#endif

/**
 * get string of a error number 
 *   - It is thread-safe. 
 */
const char* strerror(int err);

/**
 * get string of the current error number 
 *   - It is thread-safe. 
 */
inline const char* strerror() {
    return co::strerror(co::error());
}

} // co
