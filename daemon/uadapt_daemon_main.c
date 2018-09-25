/**
 * Daemon for universal adapter capability, as well as others.
 * Implements RAW socket on stub-network to device to capture all 
 * packets on wire.
 * Implements UNIX socket for application interactions.
 *
 * Must run as ROOT.  Only IPv4.
 */

#include "uadapt_daemon.h"


int main(int argc, char *argv[])
{



    uadapt_daemon();

    return 0;
}
