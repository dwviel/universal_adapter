/**
 * Daemon for universal adapter capability, as well as others.
 * Implements RAW socket on stub-network to device to capture all 
 * packets on wire.
 * Implements UNIX socket for application interactions.
 *
 * Must run as ROOT.  Only IPv4.
 */

#include <unistd.h>
#include <stdlib.h>

#include "uadapt_daemon.h"


int main(int argc, char *argv[])
{

    int ret = daemon(0, 0);   // check these args!!!!
    if(ret == -1)
    {
	// log problem
	exit(0);  // set different exit status????
    }

    uadapt_daemon();

    return 0;
}
