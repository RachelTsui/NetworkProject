
#include "ping_2.h"            
struct proto	proto_v4 = { proc_v4, send_v4, NULL, NULL, 0, IPPROTO_ICMP };
#ifdef	IPV6
struct proto	proto_v6 = { proc_v6, send_v6, NULL, NULL, 0, IPPROTO_ICMPV6 };
#endif
int	datalen = 56;		/* data that goes with ICMP echo request */  /*icmp�����غ��ֽ���*/
double rtt_min = INFINITY, rtt_max = -INFINITY, rtt_total = 0, rtt_sqr_total = 0;
long long send_count = 0, recv_count = 0;
int set_send_count = -1;  //����ָ�����������ݰ�
int set_recv_count = -1;  //����ָ�����������ݰ�
int ttl_flag = 0, broadcast_flag = 0;
//interrupt_flag
int interrupt_flag = 0;
int ttl = 0;
//���÷���ʱ����
int send_time_interval = 1; 
//���ÿ�ʼ���к�
nsent = 0; 
struct timeval tval_start;
// const char *usage = 
//   "usage: ping [-v] [-h] [-b] [-t ttl] [-q] [-c number] [-r number] [-i times] [-n seq_num] <hostname>\n"
//   "\t-h\t��ʾ������Ϣ\n"            //  ./ping_2 -h 
//   "\t-v\tNormal mode\n"           //  ./ping_2 -v www.baidu.com
//   "\t-b\tBroadcast\n"             //  ./ping_2 -b <�����ھ������Ĺ㲥��ַ> 
//   "\t-t ttl\tSet TTL(0-255)\n"    //  ./ping_2 -t 10 www.baidu.com
//   "\t-q\t����ģʽ\n"              //  ./ping_2 -q www.baidu.com
//   "\t-s\t����icmp���ĺ����ֽ���\n"//  ./ping_2 -s 60 www.baidu.com
//   "\t-c\t���÷��Ͱ��ĸ���\n"      //  ./ping_2 -c 5 www.baidu.com
//   "\t-r\t�����յ����ĸ���\n"      //  ./ping_2 -r 5 www.baidu.com
//   "\t-i\t���÷��Ͱ���ʱ����\n"  // ./ping_2 -i 5 www.baidu.com
//   "\t-n\t���ð��ĳ�ʼ���к�\n";   //./ping_2 -n 1 www.baidu.com

int main(int argc, char **argv)
{
	int				c;
	struct addrinfo	*ai;   // Ŀ��������Ϣ
	opterr = 0;		/* don't want getopt() writing to stderr */
	
	//���������в���
    //���磺./ping 127.0.0.1
        char *seg1,*seg2,*seg3;
	while ( (c = getopt(argc, argv, "vhbt:qs:c:r:i:n:")) != -1) {
		switch (c) {
		case 'v':
			verbose++;
			break;
		case 'h':
		  puts(usage);
		  return 0;
		case 'b':
		  broadcast_flag = 1;
		  verbose++; 
		  break;
		case 't':
		  ttl_flag = sscanf(optarg, "%d", &ttl) && ttl >= 0 && ttl < 256;
		  verbose++; 
		  break;
		case 'q':
		  verbose--;
		  break;
		//����ָ����С�����ݰ� ./ping_2 -s 60 www.baidu.com
		case 's':
		  sscanf(optarg, "%d", &datalen);
		  verbose++;
		  break;
		//����ָ�����������ݰ�
		case 'c':
		  sscanf(optarg, "%d", &set_send_count);
		  verbose++;
		  interrupt_flag = 1;
		  break;
		//����ָ�����������ݰ�
		 case 'r':
		  sscanf(optarg, "%d", &set_recv_count);
		  verbose++;
		  interrupt_flag = 1;
		  break;
		//���÷��Ͱ���ʱ����
		case 'i':
		  sscanf(optarg, "%d", &send_time_interval);
		  verbose++;
		  break;
		//���ó�ʼ���к�
		case 'n':
		  sscanf(optarg, "%d", &nsent);
		  verbose++;
		  break;
		case '?':
			err_quit("unrecognized option: %c", c, usage);}


if (optind != argc-1)
		err_quit(usage);
	
	//host����ip��ַ
	host = argv[optind];


    //��ȡ��ǰ�Ľ��̺�
	pid = getpid();  /* ICMP ID field is 16 bits */
	//�����źź���
	signal(SIGALRM, sig_alrm);
	signal(SIGINT, sig_int);
  
    //�������в����б�����һ����������ip��ַ������Host_serv�������д���
    //����addrinfo�ṹ
	ai = host_serv(host, NULL, 0, 0);  //��ȡĿ��������Ϣ

	printf("ping %s (%s): %d data bytes\n", ai->ai_canonname,
		   Sock_ntop_host(ai->ai_addr, ai->ai_addrlen), datalen);

		/* 4initialize according to protocol */
	//��ʼ��Э��ṹ��pr
    // IPv4 �� IPv6 ������Ҫ����, pr�����˰�����������, ���պͷ��͵�ַ�������Ϣ
	if (ai->ai_family == AF_INET) {
		pr = &proto_v4;
#ifdef	IPV6
	} else if (ai->ai_family == AF_INET6) {
		pr = &proto_v6;
		if (IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6 *)
								 ai->ai_addr)->sin6_addr)))
			err_quit("cannot ping IPv4-mapped IPv6 address");
#endif
	} else
		err_quit("unknown address family %d", ai->ai_family);

	pr->sasend = ai->ai_addr;
	pr->sarecv = calloc(1, ai->ai_addrlen);
	pr->salen = ai->ai_addrlen;  //��ַ�ṹ�ֽ���
	
    gettimeofday(&tval_start, NULL);
	readloop();  //����ѭ��

	exit(0);
}

}









//IPv4���͵�ԭʼ�׽���, ���ǵõ������ݰ��ǰ���IPͷ��, ICMP��������IPͷ֮��
void proc_v4(char *ptr, ssize_t len, struct timeval *tvrecv)
{
	int				hlen1, icmplen;
	double			rtt;
	struct ip		*ip;
	struct icmp		*icmp;
	struct timeval	*tvsend;

	ip = (struct ip *) ptr;		/* start of IP header */
	hlen1 = ip->ip_hl << 2;		/* length of IP header */  /*ipͷ����*/  //icmp���ĳ���Ϊ16bits 

	icmp = (struct icmp *) (ptr + hlen1);	/* start of ICMP header */  /*icmp���ݱ�*/
	if ( (icmplen = len - hlen1) < 8)  //��֤ICMP������Ч��, ��Ϊһ��������ICMP�������а�ͷ��8�ֽ�
		err_quit("icmplen (%d) < 8", icmplen);

	if (icmp->icmp_type == ICMP_ECHOREPLY) {
		if (icmp->icmp_id != pid)  //�����յ������ݰ�, �ó���ֻ���������̷����İ�����Ӧ
			return;			/* not a response to our ECHO_REQUEST */
		if (icmplen < 16)
			err_quit("icmplen (%d) < 16", icmplen);

        //��������ʱ��
		tvsend = (struct timeval *) icmp->icmp_data;
		tv_sub(tvrecv, tvsend);
		rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;
		if (rtt < rtt_min) rtt_min = rtt;
		if (rtt > rtt_max) rtt_max = rtt;
		rtt_total += rtt;
		rtt_sqr_total += rtt * rtt;
		recv_count++;

		if (verbose > 0)
			printf("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
				icmplen, Sock_ntop_host(pr->sarecv, pr->salen),
				icmp->icmp_seq, ip->ip_ttl, rtt);

	} else if (verbose > 1) {
		printf("  %d bytes from %s: type = %d, code = %d\n",
				icmplen, Sock_ntop_host(pr->sarecv, pr->salen),
				icmp->icmp_type, icmp->icmp_code);
	}
}


//Pv6ԭʼ�׽����õ������ݰ��ǲ�����IP�ײ���, �����ǵ�Ӧ���õ�����ʱ, �ں��Ѿ����ײ�������, ���ԾͲ�����IPv4�汾�������ƶ�ָ����
void proc_v6(char *ptr, ssize_t len, struct timeval* tvrecv)
{
#ifdef	IPV6
	int					hlen1, icmp6len;
	double				rtt;
	struct ip6_hdr		*ip6;
	struct icmp6_hdr	*icmp6;
	struct timeval		*tvsend;

	ip6 = (struct ip6_hdr *) ptr;		/* start of IPv6 header */
	hlen1 = sizeof(struct ip6_hdr);
	if (ip6->ip6_nxt != IPPROTO_ICMPV6)
		err_quit("next header not IPPROTO_ICMPV6");

	icmp6 = (struct icmp6_hdr *) (ptr + hlen1);
	if ( (icmp6len = len - hlen1) < 8)
		err_quit("icmp6len (%d) < 8", icmp6len);

	if (icmp6->icmp6_type == ICMP6_ECHO_REPLY) {
		if (icmp6->icmp6_id != pid)
			return;			/* not a response to our ECHO_REQUEST */
		if (icmp6len < 16)
			err_quit("icmp6len (%d) < 16", icmp6len);

		tvsend = (struct timeval *) (icmp6 + 1);
		tv_sub(tvrecv, tvsend);
		rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;
		if (rtt < rtt_min) rtt_min = rtt;
		if (rtt > rtt_max) rtt_max = rtt;
		rtt_total += rtt;
		rtt_sqr_total += rtt * rtt;
		recv_count++;

		if (verbose > 0)
			printf("%d bytes from %s: seq=%u, hlim=%d, rtt=%.3f ms\n",
				icmp6len, Sock_ntop_host(pr->sarecv, pr->salen),
				icmp6->icmp6_seq, ip6->ip6_hlim, rtt);

	} else if (verbose > 1) {
		printf("  %d bytes from %s: type = %d, code = %d\n",
				icmp6len, Sock_ntop_host(pr->sarecv, pr->salen),
				icmp6->icmp6_type, icmp6->icmp6_code);
	}
#endif	/* IPV6 */
}

//create checksum ����У��� 
unsigned short in_cksum(unsigned short *addr, int len)
{
        int                             nleft = len;
        int                             sum = 0;
        unsigned short  *w = addr;
        unsigned short  answer = 0;
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }
                /* 4mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(unsigned char *)(&answer) = *(unsigned char *)w ;
                sum += answer;
        }
                /* 4add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return(answer);
}

//send icmp data
void send_v4(void)
{
	int			len;
	struct icmp	*icmp;

    //init icmp datagram ��ʼ��icmp���ݱ��� 
	icmp = (struct icmp *) sendbuf;
	icmp->icmp_type = ICMP_ECHO;  //type
	icmp->icmp_code = 0;         //code 
	icmp->icmp_id = pid;   //���������õ������ĵĵ�5~6�ֽ��ǽ���id, �����ڵõ���Ӧ�Ժ�ȷ�������ĸ����̴���,
	icmp->icmp_seq = nsent++;
	gettimeofday((struct timeval *) icmp->icmp_data, NULL);   //get send time

	len = 8 + datalen;		/* checksum ICMP header and data */
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = in_cksum((u_short *) icmp, len);

	sendto(sockfd, sendbuf, len, 0, pr->sasend, pr->salen);  //send data
}





void send_v6()
{
#ifdef	IPV6
	int	len;
	struct icmp6_hdr	*icmp6;
	icmp6 = (struct icmp6_hdr *) sendbuf;
	icmp6->icmp6_type = ICMP6_ECHO_REQUEST;
	icmp6->icmp6_code = 0;
	icmp6->icmp6_id = pid;
	icmp6->icmp6_seq = nsent++;
	gettimeofday((struct timeval *) (icmp6 + 1), NULL);
	len = 8 + datalen;		/* 8-byte ICMPv6 header */
	sendto(sockfd, sendbuf, len, 0, pr->sasend, pr->salen);
#endif	/* IPV6 */
}

//���ڷ��ͺʹ������յ���ICMP���ݰ�
void readloop(void)
{	int				size;
	char			recvbuf[BUFSIZE];
	socklen_t		len;
	ssize_t			n;
	struct timeval	tval;
	sockfd = socket(pr->sasend->sa_family, SOCK_RAW, pr->icmpproto);
	setuid(getuid());		/* don't need special permissions any more */
	size = 60 * 1024;		/*���׽��ֽ��ջ�������С���ô��, ��ֹ��IPv4�㲥��ַ���߶ಥ��ַping*/
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));	
	if (ttl_flag)
       setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) ;
    if (broadcast_flag)
   setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_flag, sizeof(broadcast_flag)) ;
	sig_alrm(SIGALRM);		/* send first packet */
	
	//loop ping
	for ( ; ; ) {
		len = pr->salen;
		n = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, pr->sarecv, &len);  //receive data
		if (n < 0) {
			if (errno == EINTR)
				continue;
			else
				err_sys("recvfrom error");
		}
		gettimeofday(&tval, NULL);  //receive time  // ����ǰʱ����洢��tval��
		(*pr->fproc)(recvbuf, n, &tval);  //handle receive data
	}
}



//call send data func
void sig_alrm(int signo)
{
        (*pr->fsend)();
        send_count++;
        if( (interrupt_flag==1 && send_count == set_send_count) || (interrupt_flag==1 && recv_count == set_recv_count))  //����ѷ��͵İ��ĸ���==�趨���ͻ�������ݰ��ĸ������������� 
        interrupt();
        else
        alarm(send_time_interval);  //ÿ��1�봥��һ��send���� 
        return;         /* probably interrupts recvfrom() */
}

//���� ���� ���� echo request ���ݰ�������
void interrupt()
{
  struct timeval tval_end;
  double tval_total;
  gettimeofday(&tval_end, NULL);
  tv_sub(&tval_end, &tval_start);
  tval_total = tval_end.tv_sec * 1000.0 + tval_end.tv_usec / 1000.0;

  puts("---  ping  statistics ---");
  printf("%lld packets transmitted, %lld received, %.0lf%% packet loss, time %.2lfms\n",
    send_count, recv_count, (send_count - recv_count) * 100.0 / send_count, tval_total);
  double rtt_avg = rtt_total / recv_count;
  printf("rtt min/avg/max/mdev = %.3lf/%.3lf/%.3lf/%.3lf ms\n", rtt_min, 
    rtt_avg, rtt_max, rtt_sqr_total / recv_count - rtt_avg * rtt_avg);
  close(sockfd);
  exit(0);
}

//�жϴ��� 
void sig_int(int signo)
{
  struct timeval tval_end;
  double tval_total;
  gettimeofday(&tval_end, NULL);
  tv_sub(&tval_end, &tval_start);
  tval_total = tval_end.tv_sec * 1000.0 + tval_end.tv_usec / 1000.0;

  puts("---  ping  statistics ---");
  printf("%lld packets transmitted, %lld received, %.0lf%% packet loss, time %.2lfms\n",
    send_count, recv_count, (send_count - recv_count) * 100.0 / send_count, tval_total);
  double rtt_avg = rtt_total / recv_count;
  printf("rtt min/avg/max/mdev = %.3lf/%.3lf/%.3lf/%.3lf ms\n", rtt_min, 
    rtt_avg, rtt_max, rtt_sqr_total / recv_count - rtt_avg * rtt_avg);
  close(sockfd);
  exit(0);
}

//get rtt �õ�����ʱ��RTT(Round-Trip Time) 
void tv_sub(struct timeval *out, struct timeval *in)  //time,tv_sev is microseconds
{
	if ( (out->tv_usec -= in->tv_usec) < 0) {	/* out -= in */
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}











char * sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128];               /* Unix domain is largest */

        switch (sa->sa_family) {
        case AF_INET: {
                struct sockaddr_in      *sin = (struct sockaddr_in *) sa;

                if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
                        return(NULL);
                return(str);
        }

#ifdef  IPV6
        case AF_INET6: {
                struct sockaddr_in6     *sin6 = (struct sockaddr_in6 *) sa;

                if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL)
                        return(NULL);
                return(str);
        }
#endif

#ifdef  HAVE_SOCKADDR_DL_STRUCT
        case AF_LINK: {
                struct sockaddr_dl      *sdl = (struct sockaddr_dl *) sa;

                if (sdl->sdl_nlen > 0)
                        snprintf(str, sizeof(str), "%*s",
                                         sdl->sdl_nlen, &sdl->sdl_data[0]);
                else
                        snprintf(str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
                return(str);
        }
#endif
        default:
                snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d",
                                 sa->sa_family, salen);
                return(str);
        }
    return (NULL);
}



char * Sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
        char    *ptr;
        if ( (ptr = sock_ntop_host(sa, salen)) == NULL)
                err_sys("sock_ntop_host error");        /* inet_ntop() sets errno */
        return(ptr);
}

struct addrinfo *host_serv(const char *host, const char *serv, int family, int socktype)
{        int   n;
        struct addrinfo hints, *res;
        //����
        bzero(&hints, sizeof(struct addrinfo));
        //���ڷ��������Ĺ淶����
        hints.ai_flags = AI_CANONNAME;  /* always return canonical name */
        //��ֵΪ0������Э���޹�
        hints.ai_family = family;               /* AF_UNSPEC, AF_INET, AF_INET6, etc. */
        hints.ai_socktype = socktype;   /* 0, SOCK_STREAM, SOCK_DGRAM, etc. */
        if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
                return(NULL);

        return(res);    /* return pointer to first on linked list */  //���������б��ϵĵ�һ��ָ��
}
/* end host_serv */

static void err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{       int             errno_save, n;
        char    buf[MAXLINE];
        errno_save = errno;             /* value caller might want printed */
#ifdef  HAVE_VSNPRINTF
        vsnprintf(buf, sizeof(buf), fmt, ap);   /* this is safe */
#else
        vsprintf(buf, fmt, ap);                                 /* this is not safe */
#endif
        n = strlen(buf);
        if (errnoflag)
                snprintf(buf+n, sizeof(buf)-n, ": %s", strerror(errno_save));
        strcat(buf, "\n");
        if (daemon_proc) {
                syslog(level, buf);
        } else {
                fflush(stdout);         /* in case stdout and stderr are the same */
                fputs(buf, stderr);
                fflush(stderr);
        }
        return;
}



/* Fatal error unrelated to a system call.
 * Print a message and terminate. */

void err_quit(const char *fmt,...)
{
        va_list         ap;

        va_start(ap, fmt);
        err_doit(0, LOG_ERR, fmt, ap);
        va_end(ap);
        exit(1);
}

/* Fatal error related to a system call.
 * Print a message and terminate. */

void err_sys(const char *fmt,...)
{
        va_list         ap;

        va_start(ap, fmt);
        err_doit(1, LOG_ERR, fmt, ap);
        va_end(ap);
        exit(1);
}



















