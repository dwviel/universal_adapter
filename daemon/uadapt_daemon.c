/**
 *  Universal adapter daemon functionality
 */


#include "uadapt_daemon.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ether.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

// stub network (wire to device) socket file descriptor
int stub_sockfd;

// UNIX socket for local application interaction
int unix_sockfd;


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


int uadapt_daemon()
{
    // Instantiate RAW socket to listen on wire
    if ((stub_sockfd = socket(AF_INET, SOCK_RAW, htons(ETH_P_ALL))) == -1) //  or IPPROTO_RAW
    {
	// log this 
	return -1;;
    }

    // Don't need to bind, listen, connect, or accept.
    // Just grab packets off wire, and put them on the wire.
    // Set non-blocking????
    



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
    int flags = fcntl(unix_sockfd, F_GETFL, 0);
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

    FD_SET(stub_sockfd, &rset); /* add stub file descriptor to the set */
    FD_SET(unix_sockfd, &rset); /* add unix file descriptor to the set */
    rset_back = rset;


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
	  


	}

	if(FD_ISSET(stub_sockfd, &rset))
	{
	    
	}
	    

	rset = rset_back;  // reset the fds set
    }


    return 0;
}
