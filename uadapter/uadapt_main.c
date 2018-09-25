/**
 * Application for universal adapter capability, as well as others.
 * Implements two UNIX sockets for application interactions.
 * One to connect to the uadapter_daemon, and one to connect to the controlmqbroker.
 * Only IPv4.
 */

#include "uadapt_app.h"


int main(int argc, char *argv[])
{

    uadapt_app();

    return 0;
}
