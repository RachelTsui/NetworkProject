#include 	<netinet/in_systm.h>
#include	<netinet/ip.h>
#include	<netinet/ip_icmp.h>
#include	<netinet/icmp6.h>
#include	<netinet/ip6.h>
#include	<sys/types.h>	/* basic system data types */
#include	<sys/socket.h>	/* basic socket definitions */
#include	<sys/time.h>	/* timeval{} for select() */
#include	<time.h>		/* timespec{} for pselect() */
#include	<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include 	<pwd.h>
#include	<unistd.h>
#include	<sys/un.h>		/* for Unix domain sockets */
#include	<sys/ioctl.h>
#include	<net/if.h>
#include <stdarg.h>
#include <syslog.h>
#ifdef  HAVE_SOCKADDR_DL_STRUCT
# include       <net/if_dl.h>
#endif

#define BUFSIZE		1500
#define MAXLINE         4096

/* globals */
char	 recvbuf[BUFSIZE];
char	 sendbuf[BUFSIZE]; //ICMP报文缓冲区

int    datalen;	/* #bytes of data, following ICMP header */ //ICMP报文数据长度（不包含ICMP首部）
char	*host; //目的主机IP地址
int	 nsent;			/* add 1 for each sendto() */ //序列号
pid_t pid;			/* our PID */ //进程号
int	 sockfd; //套接字描述符
int	 verbose;
int    daemon_proc;            /* set nonzero by daemon_init() */

/* function prototypes */
void	 proc_v4(char *, ssize_t, struct timeval *);
void	 proc_v6(char *, ssize_t, struct timeval *);
void	 send_v4(void);
void	 send_v6(void);
void	 readloop(void);
void	 sig_alrm(int);
void	 tv_sub(struct timeval *, struct timeval *);
void   print_help();

char * Sock_ntop_host(const struct sockaddr *sa, socklen_t salen);
struct addrinfo* host_serv(const char *host, const char *serv, int family, int socktype);
static void err_doit(int errnoflag, int level, const char *fmt, va_list ap);
void err_quit(const char *fmt, ...);
void err_sys(const char *fmt, ...);

struct proto {
  void	 (*fproc)(char *, ssize_t, struct timeval *); /*接收处理函数*/
  void	 (*fsend)(void); /*发送函数*/
  struct sockaddr  *sasend;	/* sockaddr{} for send, from getaddrinfo */ /*发送端套接字地址结构*/
  struct sockaddr  *sarecv;	/* sockaddr{} for receiving */ /*接收端套接字地址结构*/
  socklen_t	    salen;		/* length of sockaddr{}s */ /*套接字地址结构长度*/
  int	   	    icmpproto;	/* IPPROTO_xxx value for ICMP */ /*ICMP协议版本*/
} *pr;

