#include <endian.h>

struct msghdr {
	void *msg_name;
	socklen_t msg_namelen;
	struct iovec *msg_iov;
#if __BYTE_ORDER == __BIG_ENDIAN
	int __pad1, msg_iovlen;
#else
	int msg_iovlen, __pad1;
#endif
	void *msg_control;
#if __BYTE_ORDER == __BIG_ENDIAN
	int __pad2;
	socklen_t msg_controllen;
#else
	socklen_t msg_controllen;
	int __pad2;
#endif
	int msg_flags;
};

struct cmsghdr {
#if __BYTE_ORDER == __BIG_ENDIAN
	int __pad1;
	socklen_t cmsg_len;
#else
	socklen_t cmsg_len;
	int __pad1;
#endif
	int cmsg_level;
	int cmsg_type;
};

#define SOCK_STREAM    2
#define SOCK_DGRAM     1
#define SOL_SOCKET     65535
#define SO_DEBUG       1

#define SO_REUSEADDR    0x0004
#define SO_KEEPALIVE    0x0008
#define SO_DONTROUTE    0x0010
#define SO_BROADCAST    0x0020
#define SO_LINGER       0x0080
#define SO_OOBINLINE    0x0100
#define SO_REUSEPORT    0x0200
#define SO_SNDBUF       0x1001
#define SO_RCVBUF       0x1002
#define SO_SNDLOWAT     0x1003
#define SO_RCVLOWAT     0x1004
#define SO_RCVTIMEO     0x1006
#define SO_SNDTIMEO     0x1005
#define SO_ERROR        0x1007
#define SO_TYPE         0x1008
#define SO_ACCEPTCONN   0x1009
#define SO_PROTOCOL     0x1028
#define SO_DOMAIN       0x1029

#define SO_NO_CHECK     11
#define SO_PRIORITY     12
#define SO_BSDCOMPAT    14
#define SO_PASSCRED     17
#define SO_PEERCRED     18
#define SO_SNDBUFFORCE  31
#define SO_RCVBUFFORCE  33

#define SOCK_NONBLOCK     0200
#define SOCK_CLOEXEC  02000000
