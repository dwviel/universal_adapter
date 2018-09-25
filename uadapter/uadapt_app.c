/**
 * Universal adapter application core functionality.
 */

#include "uadapt_app.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../common/uadapt_common.h"


// UNIX sock fd to connect to uadapter daemon
int unix_uadapter_daemon_sockfd = -1;

// UNIX sock fd to connect to controMQ daemon
int unix_controlmq_sockfd = -1;

#define UADAPT_DAEMON_PATH "/tmp/uadapt_broker.sock"
#define CONTROLMQ_BROKER_PATH "/tmp/controlmqbroker.sock"



int read_unix_uadapter(unix_uadapter_daemon_sockfd)
{

    return 0;
}


int read_unix_controlmq(unix_controlmq_sockfd)
{

    return 0;
}


int uadapt_app()
{

    // Instantiate UNIX socket for uadapter daemon interaction
    if ((unix_uadapter_daemon_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
	// log this 
	return -1;;
    }

    struct sockaddr_un local, remote;
    socklen_t remote_len = sizeof(remote);
    int len = 0;
    int num_allowed_connections = 1; // only allow one for now
    
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




    // Instantiate UNIX socket for controlMQ daemon interaction
    if ((unix_controlmq_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
	// log this 
	return -1;;
    }

    struct sockaddr_un localc, remotec;
    socklen_t remotec_len = sizeof(remotec);
    int lenc = 0;
    int num_allowed_connections_c = 1; // only allow one for now
    
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
