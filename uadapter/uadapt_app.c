/**
 * Universal adapter application core functionality.
 */

/*  Design issues:
    Buffer the writes????  Or just try to do write and drop
    if fails?  Make unix socket non-blocking?
    Handle special???? ARP, ICMP
    Need allowed dest IP addrs????
    All controlMQ messages best effort as this is a network
    equivalent.
    
*/


#include "uadapt_app.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>


#include "../common/uadapt_common.h"


// UNIX sock fd to connect to uadapter daemon
int unix_uadapter_daemon_sockfd = -1;

// UNIX sock fd to connect to controMQ daemon
int unix_controlmq_sockfd = -1;

#define UADAPT_DAEMON_PATH "/tmp/uadapt_broker.sock"
#define CONTROLMQ_BROKER_PATH "/tmp/controlmqbroker.sock"

// Ethernet frame size (Ethernet II, no trailing crc)
#define ETH_BUF_SIZ	1514

// Max payload for ControMQ message data Check these values!!!!
#define MAX_CONTROLMQ_DATA_SIZE 1070 
#define MAX_CONTROLMQ_DATA_SIZE_TWO 1066 

uint32_t controlmq_nonce = 1;


int read_unix_uadapter(int unix_uadapter_daemon_sockfd)
{
    // Gets whole Ethernet frame to pass on through the ControlMQ network

    // We will only read Ethernet frames.  Anything larger will be discarded.
    char buf[ETH_BUF_SIZ]; // single Ethernet frame
    bzero(buf, ETH_BUF_SIZ);
    int numbytes = 0;
    //int numtotal = 0;

    numbytes = read(unix_uadapter_daemon_sockfd, buf, ETH_BUF_SIZ);
    if(numbytes == -1)
    {
	return -1;
    }

    // Create two sidl messages.
    // 1) if packet size <= max controlmq message size then
    // send in one message.
    // 2) if packet size > max controlmq message size then
    // send second message with remainder of packet
    // First part identified by full payload, second
    // by less than full payload.  Both have same nonce.
    // Timeout to receive both parts.

    // SIDL message args:
    // 1) packet data (uint8_t[maxsize])
    // 2) nonce (uint32_t), packet data part (uint8_t[maxsize - 4]

    if(numbytes > MAX_CONTROLMQ_DATA_SIZE)
    {
	// Two messages
	// First should just copy needed bytes
	// send_packet_data_two(controlmq_nonce, buf);
	uint8_t rembuf[MAX_CONTROLMQ_DATA_SIZE_TWO] = {0};
	memcpy(rembuf, (buf+MAX_CONTROLMQ_DATA_SIZE_TWO), 
	       (numbytes-MAX_CONTROLMQ_DATA_SIZE_TWO));
	// send_packet_data_two(controlmq_nonce, rembuf);
	controlmq_nonce++;
    }
    else
    {
	// send_packet_data(buf);
    }

    return 0;
}

int read_unix_controlmq_two(uint32_t nonce, uint8_t buf[])
{
    // Receive two parts of messages in two calls

}


int read_unix_controlmq(int unix_controlmq_sockfd)
{
    // We will only read Ethernet frames.  Anything larger will be discarded.
    char buf[ETH_BUF_SIZ]; // single Ethernet frame
    memset(buf, 0, ETH_BUF_SIZ);
    int numbytes = 0;
    //int numtotal = 0;

    numbytes = read(unix_controlmq_sockfd, buf, ETH_BUF_SIZ);
    if(numbytes == -1)
    {
	return -1;
    }

    // use sidl generate interfaces!!!!

    // One or Two messge version
    char ethbuf[ETH_BUF_SIZ];
    memset(ethbuf, 0, ETH_BUF_SIZ);
    // copy message data to ethbuf
    if(write(unix_uadapter_daemon_sockfd, ethbuf, ETH_BUF_SIZ) == -1)
    {
	return -1;
    }

    return 0;
}


int uadapt_app()
{
    int flags = 0;

    // Instantiate UNIX socket for uadapter daemon interaction
    if ((unix_uadapter_daemon_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
	// log this 
	return -1;;
    }

    struct sockaddr_un remote;
    //socklen_t remote_len = sizeof(remote);
    int len = 0;
    //int num_allowed_connections = 1; // only allow one for now
    
    /*local.sun_family = AF_UNIX; 
    strcpy(local.sun_path, UADAPT_DAEMON_PATH);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if(bind(unix_uadapter_daemon_sockfd, (struct sockaddr *)&local, len) == -1)
    {
	// log error
	return -1;
	}*/

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, UADAPT_DAEMON_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(unix_uadapter_daemon_sockfd, (struct sockaddr *)&remote, len) == -1) {
        // log error
        return -1;
    }

    // Set non blocking
    flags = fcntl(unix_uadapter_daemon_sockfd, F_GETFL, NULL);
    fcntl(unix_uadapter_daemon_sockfd, F_SETFL, flags | O_NONBLOCK);


    // Instantiate UNIX socket for controlMQ daemon interaction
    if ((unix_controlmq_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
	// log this 
	return -1;;
    }

    struct sockaddr_un remotec;
    //socklen_t remotec_len = sizeof(remotec);
    int lenc = 0;
    //int num_allowed_connections_c = 1; // only allow one for now
    
    /*localc.sun_family = AF_UNIX;  
    strcpy(localc.sun_path, CONTROLMQ_BROKER_PATH);
    unlink(localc.sun_path);
    lenc = strlen(localc.sun_path) + sizeof(localc.sun_family);
    if(bind(unix_controlmq_sockfd, (struct sockaddr *)&localc, lenc) == -1)
    {
	// log error
	return -1;
	}*/  

    remotec.sun_family = AF_UNIX;
    strcpy(remotec.sun_path, CONTROLMQ_BROKER_PATH);
    lenc = strlen(remotec.sun_path) + sizeof(remotec.sun_family);
    if (connect(unix_controlmq_sockfd, (struct sockaddr *)&remotec, lenc) == -1) {
        // log error
        return -1;
    }

    // Set non blocking
    flags = fcntl(unix_controlmq_sockfd, F_GETFL, NULL);
    fcntl(unix_controlmq_sockfd, F_SETFL, flags | O_NONBLOCK);

    // Done in ControlMQ framework!!!!

    // Block on the two fds and handle appropriately when something to read
    int nfds = 0;
    int ret = 0;
    fd_set rset, rset_back;
    FD_ZERO(&rset); /* clear the set */
    FD_ZERO(&rset_back); /* clear the set */

    FD_SET(unix_uadapter_daemon_sockfd, &rset_back); /* add adapter daemon fd to the set */
    FD_SET(unix_controlmq_sockfd, &rset_back); /* add unix listener fd to the set */
    rset = rset_back;


    nfds = int_max(nfds, unix_uadapter_daemon_sockfd);
    nfds = int_max(nfds, unix_controlmq_sockfd);
    nfds++;


    while(1)
    {
	// Handle signals !!!!
	ret = pselect(nfds, &rset, NULL, NULL, NULL, NULL);
	if(ret == -1)
	{
	    continue;
	}

	if(unix_uadapter_daemon_sockfd >= 0)
	{
	    if(FD_ISSET(unix_uadapter_daemon_sockfd, &rset))
	    {
		read_unix_uadapter(unix_uadapter_daemon_sockfd);
	    }
	}
	if(unix_controlmq_sockfd >= 0)
	{
	    if(FD_ISSET(unix_controlmq_sockfd, &rset))
	    {
		read_unix_controlmq(unix_controlmq_sockfd);
	    }
	}

	rset = rset_back;  // reset the fds set
    }



    return 0;
}
