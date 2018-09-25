#ifndef UADAPT_COMMON
#define UADAPT_COMMON


/**
 * Returns the max of x or y.
 */
int int_max(int x, int y);


/**
 * Simple checksum function, may use others such as Cyclic Redundancy Check, CRC
 */
unsigned short csum(unsigned short *buf, int len);


#endif
