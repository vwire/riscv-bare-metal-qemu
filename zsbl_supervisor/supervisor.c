#include "supervisor.h"

/* ── UART NS16550A at 0x10000000 ──────────────────────── */
#define UART0            0x10000000UL
#define UART_TX  (*(volatile unsigned char*)(UART0 + 0x00))
#define UART_LSR (*(volatile unsigned char*)(UART0 + 0x05))
#define UART_THRE (1 << 5)

/* ── CLINT ────────────────────────────────────────────── */
/* mtime is readable from S-mode on QEMU virt             */
#define CLINT_MTIME     (*(volatile unsigned long long*)(0x200BFF8UL))
#define TIMER_INTERVAL  5000000ULL      /* ~5 seconds on QEMU */

/* Interrupt counter — volatile because modified in trap handler */
static volatile int timer_count = 0;

/* ── SBI SET_TIMER ecall ──────────────────────────────── */
/* Calls M-mode zsbl.S SBI handler to write mtimecmp.     */
/* S-mode cannot write mtimecmp directly (M-mode resource).*/
static void sbi_set_timer(unsigned long long t)
{
    register unsigned long a0 __asm__("a0") = (unsigned long)t;
    register unsigned long a7 __asm__("a7") = 0UL;  /* ext 0 = SET_TIMER */
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
}

/* ── UART output ──────────────────────────────────────── */
static void uart_putc(char c)
{
    while (!(UART_LSR & UART_THRE));
    UART_TX = c;
}

static void puts(const char *s) { while (*s) uart_putc(*s++); }

static void put_hex(unsigned long v)
{
    const char *h = "0123456789ABCDEF";
    puts("0x");
    for (int i = 60; i >= 0; i -= 4)
        uart_putc(h[(v >> i) & 0xF]);
}

static void put_dec(unsigned int v)
{
    if (!v) { uart_putc('0'); return; }
    char b[12]; int i = 0;
    while (v) { b[i++] = '0' + v % 10; v /= 10; }
    while (i--) uart_putc(b[i]);
}

/* ── S-mode trap handler ──────────────────────────────── */
/* Called from _strap_vector in supervisor_entry.S        */
void supervisor_trap_handler(void)
{
    unsigned long scause, sepc;
    __asm__ volatile("csrr %0, scause" : "=r"(scause));
    __asm__ volatile("csrr %0, sepc"   : "=r"(sepc));

    if (scause & (1UL << 63)) {
        /* Interrupt path — top bit set */
        unsigned long code = scause & ~(1UL << 63);
        if (code == 5) {
            /* Supervisor Timer Interrupt */
            timer_count++;
            puts("[IRQ] Timer #");
            put_dec((unsigned int)timer_count);
            puts("  mtime=");
            put_hex((unsigned long)CLINT_MTIME);
            puts("\r\n");

            if (timer_count < 3) {
                /* Re-arm for next tick */
                sbi_set_timer(CLINT_MTIME + TIMER_INTERVAL);
            } else {
                /* Done — push mtimecmp far into future to stop M-mode firing */
                sbi_set_timer(0xFFFFFFFFFFFFFFFFULL);
                /* Disable STIE so no further S-mode timer interrupts */
                unsigned long sie;
                __asm__ volatile("csrr %0, sie" : "=r"(sie));
                sie &= ~(1UL << 5);
                __asm__ volatile("csrw sie, %0" :: "r"(sie));
            }
        } else {
            puts("[IRQ] Unknown interrupt code=");
            put_dec((unsigned int)code);
            puts("\r\n");
        }
    } else {
        /* Exception path */
        puts("[EXC] scause="); put_hex(scause);
        puts(" sepc=");        put_hex(sepc);
        puts("\r\n");
        /* Skip faulting instruction */
        __asm__ volatile("csrw sepc, %0" :: "r"(sepc + 4));
    }
}

/* ── Demo: arithmetic ─────────────────────────────────── */
static void demo_arithmetic(void)
{
    puts("\r\n[---- Arithmetic Demo ----]\r\n");
    volatile long a = 123456789L, b = 987654321L;
    puts("  a     = "); put_dec((unsigned int)a);         puts("\r\n");
    puts("  b     = "); put_dec((unsigned int)b);         puts("\r\n");
    puts("  a+b   = "); put_dec((unsigned int)(a + b));   puts("\r\n");
    puts("  b-a   = "); put_dec((unsigned int)(b - a));   puts("\r\n");
    puts("  a*3   = "); put_dec((unsigned int)(a * 3));   puts("\r\n");
}

/* ── Demo: memory read/write ──────────────────────────── */
static void demo_memory(void)
{
    puts("\r\n[---- Memory Read/Write Demo ----]\r\n");
    volatile unsigned long *p = (volatile unsigned long *)0x80400000UL;
    unsigned long pat[] = {
        0xDEADBEEFCAFEBABEUL,
        0xA5A5A5A5A5A5A5A5UL,
        0x0123456789ABCDEFUL,
        0xFFFFFFFFFFFFFFFFUL,
    };
    int pass = 0;
    for (int i = 0; i < 4; i++) {
        p[i] = pat[i];
        int ok = (p[i] == pat[i]);
        puts("  ["); put_hex((unsigned long)&p[i]); puts("] ");
        puts(ok ? "PASS" : "FAIL"); puts("\r\n");
        if (ok) pass++;
    }
    puts("  Result: "); put_dec(pass); puts("/4 passed\r\n");
}

/* ── Demo: timer interrupts via SBI ──────────────────── */
static void demo_timer(void)
{
    puts("\r\n[---- Timer Interrupt Demo ----]\r\n");
    puts("  Arming timer via SBI ecall...\r\n");

    /* Arm first tick */
    sbi_set_timer(CLINT_MTIME + TIMER_INTERVAL);

    /* Enable STIE (bit 5) in sie */
    unsigned long sie;
    __asm__ volatile("csrr %0, sie" : "=r"(sie));
    sie |= (1UL << 5);
    __asm__ volatile("csrw sie, %0" :: "r"(sie));

    /* Enable global S-mode interrupts (SIE bit 1) in sstatus */
    unsigned long ss;
    __asm__ volatile("csrr %0, sstatus" : "=r"(ss));
    ss |= (1UL << 1);
    __asm__ volatile("csrw sstatus, %0" :: "r"(ss));

    puts("  Waiting for 3 interrupts...\r\n");

    /* WFI loop — exits once trap handler increments timer_count to 3 */
    while (1) {
        __asm__ volatile("wfi");
        if (timer_count >= 3) break;
    }

    puts("  3 timer interrupts received.\r\n");
}

/* ── Entry point ──────────────────────────────────────── */
/* Called from _supervisor_start in supervisor_entry.S    */
/* a0 = hartid, a1 = DTB address (from QEMU reset vector) */
void supervisor_main(unsigned long hartid, unsigned long dtb)
{
    puts("\r\n########################################\r\n");
    puts("#   RISC-V ZSBL + Supervisor Demo      #\r\n");
    puts("########################################\r\n");
    puts("  Hart ID : "); put_hex(hartid); puts("\r\n");
    puts("  DTB     : "); put_hex(dtb);    puts("\r\n");

    demo_arithmetic();
    demo_memory();
    demo_timer();

    puts("\r\n########################################\r\n");
    puts("#   Demo complete. System halted.       #\r\n");
    puts("########################################\r\n");

    while (1) __asm__ volatile("wfi");
}
