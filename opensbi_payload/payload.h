#ifndef PAYLOAD_H
#define PAYLOAD_H

void payload_main(unsigned long hartid, unsigned long dtb);
void payload_trap_handler(void);

/* SBI return structure */
struct sbiret {
    long error;
    long value;
};

/* SBI error codes */
#define SBI_SUCCESS               0
#define SBI_ERR_FAILED           -1
#define SBI_ERR_NOT_SUPPORTED    -2
#define SBI_ERR_INVALID_PARAM    -3
#define SBI_ERR_DENIED           -4
#define SBI_ERR_INVALID_ADDRESS  -5

/* SBI Extension IDs */
#define SBI_EXT_BASE    0x10            /* Base extension */
#define SBI_EXT_TIME    0x54494D45      /* "TIME" */
#define SBI_EXT_IPI     0x735049        /* "sPI" */
#define SBI_EXT_RFENCE  0x52464E43      /* "RFNC" */
#define SBI_EXT_HSM     0x48534D        /* "HSM" */
#define SBI_EXT_SRST    0x53525354      /* "SRST" */

/* SBI Base function IDs */
#define SBI_BASE_GET_SPEC_VERSION   0
#define SBI_BASE_GET_IMPL_ID        1
#define SBI_BASE_GET_IMPL_VERSION   2
#define SBI_BASE_PROBE_EXTENSION    3
#define SBI_BASE_GET_MVENDORID      4
#define SBI_BASE_GET_MARCHID        5
#define SBI_BASE_GET_MIMPID         6

/* SBI TIME function IDs */
#define SBI_TIME_SET_TIMER          0

/* SBI implementation IDs */
#define SBI_IMPL_OPENSBI            1

#endif
