/**
 *  Universal adapter daemon functionality
 */


#include "uadapt_daemon.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ether.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>

// stub network (wire to device) socket file descriptor
int stub_sockfd;

// UNIX socket to listen for local application connection
int unix_sockfd;

// UNIX socket connection to local application
int unix_conn_sockfd = -1;

// stub network interface name
char stub_if_name[IFNAMSIZ];

// Ethernet frame size
#define ETH_BUF_SIZ	1514



// Simple checksum function, may use others such as Cyclic Redundancy Check, CRC
unsigned short csum(unsigned short *buf, int len)
{
        unsigned long sum;
        for(sum=0; len>0; len--)
                sum += *buf++;

        sum = (sum >> 16) + (sum &0xffff);
        sum += (sum >> 16);

        return (unsigned short)(~sum);
}


int int_max(int x, int y)
{
    return ((x >= y) ? x : y);
}


int read_stub_net(int stub_sockfd)
{
    // We will only read Ethernet frames.  Anything larger will be discarded.
    char buf[ETH_BUF_SIZ]; // single Ethernet frame
    bzero(buf, ETH_BUF_SIZ);
    int numbytes = 0;
    int numtotal = 0;

    numbytes = recvfrom(stub_sockfd, buf, ETH_BUF_SIZ, 0, NULL, NULL);

    // Send to adapter application
    if(unix_conn_sockfd >= 0)
    {
	int numsent = write(unix_conn_sockfd, buf, numbytes);
	if(numsent == -1)
	{
	    return -1;
	}
	if(numsent < numbytes)
	{
	    // try sending rest of data one time
	    numtotal += numsent;
	    numsent = write(unix_conn_sockfd, (buf+numsent), (numbytes-numsent));
	    numtotal += numsent;
	}
	if(numtotal < numbytes)
	{
	    return -1;
	}
    }

    return 0;
}


int read_unix_conn(int unix_conn_sockfd)
{
    // We will only read Ethernet frames.  Anything larger will be discarded.
    char buf[ETH_BUF_SIZ]; // single Ethernet frame
    bzero(buf, ETH_BUF_SIZ);
    int numbytes = 0;

    if(unix_conn_sockfd >= 0)
    {
	numbytes = read(unix_conn_sockfd, buf, ETH_BUF_SIZ);
	if(numbytes == -1)
	{
	    return -1;
	}
	if(numbytes == 0)
	{
	    return -1;
	} 
	
	// Need to have a valid sockaddr to send
	// Extract the dest eth addr
	// Ethernet frame
	struct ethhdr *eth = (struct ether_header *) buf;
	
	struct ifreq ifreq_i;
	memset(&ifreq_i,0,sizeof(ifreq_i));
	strncpy(ifreq_i.ifr_name, stub_if_name, IFNAMSIZ-1); //giving name of Interface
	
	if((ioctl(stub_sockfd, SIOCGIFINDEX, &ifreq_i)) < 0)
	{
	    return -1;
	}
	
	struct sockaddr_ll sadr_ll;
	sadr_ll.sll_ifindex = ifreq_i.ifr_ifindex; // index of interface
	sadr_ll.sll_halen = ETH_ALEN; // length of destination mac address
	sadr_ll.sll_addr[0] = eth->h_dest[0];
	sadr_ll.sll_addr[1] = eth->h_dest[1];
	sadr_ll.sll_addr[2] = eth->h_dest[2];
	sadr_ll.sll_addr[3] = eth->h_dest[3];
	sadr_ll.sll_addr[4] = eth->h_dest[4];
	sadr_ll.sll_addr[5] = eth->h_dest[5];

	int sentbytes = sendto(stub_sockfd, buf, numbytes,
			       0, (struct sockaddr*)&sadr_ll, 
			       sizeof(struct sockaddr_ll));
	if(sentbytes == -1)
	{
	    return -1;
	}
	if(sentbytes == 0)
	{
	    return -1;
	}
    }
    

    return 0;
}


int uadapt_daemon()
{
    // Grab packets off the stub wire, and put them on the stub wire.
    // Instantiate RAW socket to listen on wire
    if ((stub_sockfd = socket(AF_INET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
    {
	// log this 
	return -1;;
    }

    // Check these options with broker code!!!!
    // Set non-blocking????
    // Set interface to promiscuous mode - do we need to do this every time?
    struct ifreq ifopts;  /* set promiscuous mode */
    //struct ifreq if_ip;   /* get ip addr */
    int sockopt = 1;
    strncpy(ifopts.ifr_name, stub_if_name, IFNAMSIZ-1);
    ioctl(stub_sockfd, SIOCGIFFLAGS, &ifopts);
    ifopts.ifr_flags |= IFF_PROMISC;
    ioctl(stub_sockfd, SIOCSIFFLAGS, &ifopts);

    // Allow the socket to be reused - incase connection is closed prematurely */
    if (setsockopt(stub_sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) 
    {
	// log perror("setsockopt");
	close(stub_sockfd);
	return -1;
    }

    int flags = 0;
    // Set the stub net socket non-blocking
    flags = fcntl(stub_sockfd, F_GETFL, 0);
    if (flags == -1) 
    {
	// log error
	return -1;
    }
    flags = flags | O_NONBLOCK;
    if(fcntl(stub_sockfd, F_SETFL, flags) == -1)
    {
	// log error
	return -1;
    }

    // Bind to device ????
    if (setsockopt(stub_sockfd, SOL_SOCKET, SO_BINDTODEVICE, stub_if_name, IFNAMSIZ-1) == -1)	
    {
	// log perror("SO_BINDTODEVICE");
	close(stub_sockfd);
	return -1;
    }



    // Instantiate UNIX socket for application interaction
    if ((unix_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
	// log this 
	return -1;;
    }

    struct sockaddr_un local, remote;
    socklen_t remote_len = sizeof(remote);
    int len = 0;
    int num_allowed_connections = 1; // only allow one for now
    
    local.sun_family = AF_UNIX;  /* local is declared before socket() ^ */
    strcpy(local.sun_path, "/tmp/uadapt_broker.sock");
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if(bind(unix_sockfd, (struct sockaddr *)&local, len) == -1)
    {
	// log error
	return -1;
    }

    if(listen(unix_sockfd, num_allowed_connections) == -1)
    {
	// log error
	return -1;
    }

    // Set the listening unix socket non-blocking
    flags = fcntl(unix_sockfd, F_GETFL, 0);
    if (flags == -1) 
    {
	// log error
	return -1;
    }
    flags = flags | O_NONBLOCK;
    if(fcntl(unix_sockfd, F_SETFL, flags) == -1)
    {
	// log error
	return -1;
    }

    // Block on the two fds and handle appropriately when something to read
    int nfds = 0;
    int ret = 0;
    fd_set rset, rset_back;
    FD_ZERO(&rset); /* clear the set */
    FD_ZERO(&rset_back); /* clear the set */

    FD_SET(stub_sockfd, &rset_back); /* add stub network fd to the set */
    FD_SET(unix_sockfd, &rset_back); /* add unix listener fd to the set */
    rset = rset_back;


    nfds = int_max(nfds, stub_sockfd);
    nfds = int_max(nfds, unix_sockfd);
    nfds++;

    while(1)
    {
	// Handle signals !!!!
	ret = pselect(nfds, &rset, NULL, NULL, NULL, NULL);

	// Handle to fd ready to read
	if(FD_ISSET(unix_sockfd, &rset))
	{
	    int conn_unix_fd = 
		accept(unix_sockfd, (struct sockaddr *)&remote, &remote_len);
	    if(conn_unix_fd < 0)
	    {
		// log err
		continue;  // problem could be recoverable
	    }

	    unix_conn_sockfd = conn_unix_fd;
	    FD_SET(unix_conn_sockfd, &rset_back); /* add unix conn fd to the set */
	    nfds = int_max(nfds, unix_conn_sockfd);
	}

	if(FD_ISSET(stub_sockfd, &rset))
	{
	    read_stub_net(stub_sockfd);
	}

	if(unix_conn_sockfd >= 0)
	{
	    if(FD_ISSET(unix_conn_sockfd, &rset))
	    {
		read_unix_conn(unix_conn_sockfd);
	    }
	}
	    

	rset = rset_back;  // reset the fds set
    }


    return 0;
}
