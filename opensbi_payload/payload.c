#include "payload.h"

/* ── UART NS16550A ───────────────────────────────────── */
#define UART0            0x10000000UL
#define UART_TX  (*(volatile unsigned char*)(UART0 + 0x00))
#define UART_LSR (*(volatile unsigned char*)(UART0 + 0x05))
#define UART_THRE (1 << 5)

/* ── Read mtime via 'time' CSR (S-mode safe) ─────────── */
static inline unsigned long long get_time(void)
{
    unsigned long t;
    __asm__ volatile("csrr %0, time" : "=r"(t));
    return t;
}

/* ── Write stimecmp directly (sstc extension) ────────── */
/* stimecmp CSR number = 0x14D
 * Using raw CSR number avoids needing -march=..._sstc
 * which older GCC (10.2) does not support.               */
static inline void set_stimecmp(unsigned long long t)
{
    __asm__ volatile("csrw 0x14D, %0" :: "r"(t));
}

#define TIMER_INTERVAL  5000000ULL

static volatile int timer_count = 0;

/* ── UART output ─────────────────────────────────────── */
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

static void put_dec(unsigned long v)
{
    if (!v) { uart_putc('0'); return; }
    char b[20]; int i = 0;
    while (v) { b[i++] = '0' + v % 10; v /= 10; }
    while (i--) uart_putc(b[i]);
}

/* ── Core SBI ecall ──────────────────────────────────── */
static struct sbiret sbi_ecall(unsigned long eid, unsigned long fid,
    unsigned long a0, unsigned long a1, unsigned long a2,
    unsigned long a3, unsigned long a4, unsigned long a5)
{
    register unsigned long _a0 __asm__("a0") = a0;
    register unsigned long _a1 __asm__("a1") = a1;
    register unsigned long _a2 __asm__("a2") = a2;
    register unsigned long _a3 __asm__("a3") = a3;
    register unsigned long _a4 __asm__("a4") = a4;
    register unsigned long _a5 __asm__("a5") = a5;
    register unsigned long _a6 __asm__("a6") = fid;
    register unsigned long _a7 __asm__("a7") = eid;

    __asm__ volatile("ecall"
        : "+r"(_a0), "+r"(_a1)
        : "r"(_a2), "r"(_a3), "r"(_a4), "r"(_a5), "r"(_a6), "r"(_a7)
        : "memory");

    return (struct sbiret){ .error = _a0, .value = _a1 };
}

/* ── SBI BASE extension ──────────────────────────────── */
static void demo_sbi_base(void)
{
    struct sbiret ret;

    puts("\r\n[---- SBI Base Extension (0x10) ----]\r\n");

    ret = sbi_ecall(SBI_EXT_BASE, SBI_BASE_GET_SPEC_VERSION, 0,0,0,0,0,0);
    puts("  SBI Spec Version : ");
    put_dec((ret.value >> 24) & 0x7F);
    uart_putc('.');
    put_dec(ret.value & 0xFFFFFF);
    puts("\r\n");

    ret = sbi_ecall(SBI_EXT_BASE, SBI_BASE_GET_IMPL_ID, 0,0,0,0,0,0);
    puts("  Implementation   : ");
    if (ret.value == SBI_IMPL_OPENSBI) puts("OpenSBI");
    else put_dec(ret.value);
    puts("\r\n");

    ret = sbi_ecall(SBI_EXT_BASE, SBI_BASE_GET_IMPL_VERSION, 0,0,0,0,0,0);
    puts("  OpenSBI Version  : ");
    put_dec((ret.value >> 16) & 0xFFFF);
    uart_putc('.');
    put_dec(ret.value & 0xFFFF);
    puts("\r\n");

    ret = sbi_ecall(SBI_EXT_BASE, SBI_BASE_GET_MVENDORID, 0,0,0,0,0,0);
    puts("  mvendorid        : "); put_hex(ret.value); puts("\r\n");

    ret = sbi_ecall(SBI_EXT_BASE, SBI_BASE_GET_MARCHID, 0,0,0,0,0,0);
    puts("  marchid          : "); put_hex(ret.value); puts("\r\n");

    puts("  Extensions       : ");
    unsigned long exts[] = { SBI_EXT_TIME, SBI_EXT_IPI, SBI_EXT_RFENCE, SBI_EXT_HSM, SBI_EXT_SRST };
    const char *names[]  = { "TIME", "IPI", "RFENCE", "HSM", "SRST" };
    for (int i = 0; i < 5; i++) {
        ret = sbi_ecall(SBI_EXT_BASE, SBI_BASE_PROBE_EXTENSION, exts[i], 0,0,0,0,0);
        if (ret.value) { puts(names[i]); uart_putc(' '); }
    }
    puts("\r\n");
}

/* ── SBI TIME extension ──────────────────────────────── */
static void sbi_set_timer(unsigned long long t)
{
    sbi_ecall(SBI_EXT_TIME, SBI_TIME_SET_TIMER, t, 0,0,0,0,0);
}

/* ── S-mode trap handler ─────────────────────────────── */
void payload_trap_handler(void)
{
    unsigned long scause, sepc;
    __asm__ volatile("csrr %0, scause" : "=r"(scause));
    __asm__ volatile("csrr %0, sepc"   : "=r"(sepc));

    if (scause & (1UL << 63)) {
        unsigned long code = scause & ~(1UL << 63);

        if (code == 5) {
            /* STI — Supervisor Timer Interrupt via stimecmp (sstc) */
            timer_count++;
            puts("[IRQ] Timer #");
            put_dec(timer_count);
            puts("  time=");
            put_hex((unsigned long)get_time());
            puts("\r\n");

            if (timer_count < 3) {
                set_stimecmp(get_time() + TIMER_INTERVAL);
            } else {
                set_stimecmp(0xFFFFFFFFFFFFFFFFULL);
                unsigned long sie;
                __asm__ volatile("csrr %0, sie" : "=r"(sie));
                sie &= ~(1UL << 5);
                __asm__ volatile("csrw sie, %0" :: "r"(sie));
            }
        } else if (code == 1) {
            /* SSI fallback — OpenSBI timer forwarding without sstc */
            timer_count++;
            puts("[IRQ] SSI Timer #");
            put_dec(timer_count);
            puts("\r\n");
            __asm__ volatile("csrc sip, %0" :: "r"(1UL << 1));
            if (timer_count < 3) {
                sbi_set_timer(get_time() + TIMER_INTERVAL);
            } else {
                sbi_set_timer(0xFFFFFFFFFFFFFFFFULL);
                unsigned long sie;
                __asm__ volatile("csrr %0, sie" : "=r"(sie));
                sie &= ~(1UL << 1);
                __asm__ volatile("csrw sie, %0" :: "r"(sie));
            }
        } else {
            puts("[IRQ] Unknown interrupt code="); put_dec(code); puts("\r\n");
        }
    } else {
        puts("[EXC] scause="); put_hex(scause);
        puts(" sepc=");        put_hex(sepc);
        puts("\r\n");
        __asm__ volatile("csrw sepc, %0" :: "r"(sepc + 4));
    }
}

/* ── Timer demo ──────────────────────────────────────── */
static void demo_timer(void)
{
    puts("\r\n[---- SBI Timer / sstc Extension Demo ----]\r\n");
    puts("  sstc present — writing stimecmp (CSR 0x14D) directly\r\n");
    puts("  Interrupt fires as STI (scause interrupt code 5)\r\n");

    unsigned long sstatus_val, sie_val, sip_val;
    __asm__ volatile("csrr %0, sstatus" : "=r"(sstatus_val));
    __asm__ volatile("csrr %0, sie"     : "=r"(sie_val));
    __asm__ volatile("csrr %0, sip"     : "=r"(sip_val));
    puts("  sstatus = "); put_hex(sstatus_val); puts("\r\n");
    puts("  sie     = "); put_hex(sie_val);     puts("\r\n");
    puts("  sip     = "); put_hex(sip_val);     puts("\r\n");

    puts("  Arming stimecmp...\r\n");
    set_stimecmp(get_time() + TIMER_INTERVAL);

    /* Enable STIE (bit 5) */
    unsigned long sie;
    __asm__ volatile("csrr %0, sie" : "=r"(sie));
    sie |= (1UL << 5);
    __asm__ volatile("csrw sie, %0" :: "r"(sie));

    /* Enable global S-mode interrupts (sstatus.SIE = bit 1) */
    unsigned long ss;
    __asm__ volatile("csrr %0, sstatus" : "=r"(ss));
    ss |= (1UL << 1);
    __asm__ volatile("csrw sstatus, %0" :: "r"(ss));

    puts("  Waiting for 3 timer interrupts...\r\n");

    while (1) {
        __asm__ volatile("wfi");
        if (timer_count >= 3) break;
    }

    puts("  3 timer interrupts received.\r\n");
}

/* ── Compare old vs new SBI calling convention ───────── */
static void demo_sbi_comparison(void)
{
    puts("\r\n[---- SBI Calling Convention Comparison ----]\r\n");
    puts("  Your zsbl.S (legacy):          a7=0x00, a0=timer_val\r\n");
    puts("  OpenSBI standard (TIME ext):   a7=0x54494D45, a6=0, a0=timer_val\r\n");
    puts("  Both do the same thing but:\r\n");
    puts("    Legacy (0x00) is deprecated in SBI v0.2+\r\n");
    puts("    TIME ext (0x54494D45) is the correct SBI v1.0 way\r\n");
    puts("  On sstc hardware: write stimecmp directly, no SBI call needed\r\n");
}

/* ── Entry point ─────────────────────────────────────── */
void payload_main(unsigned long hartid, unsigned long dtb)
{
    puts("\r\n########################################\r\n");
    puts("#   OpenSBI Payload Demo               #\r\n");
    puts("#   S-mode under real OpenSBI          #\r\n");
    puts("########################################\r\n");
    puts("  Hart ID : "); put_hex(hartid); puts("\r\n");
    puts("  DTB     : "); put_hex(dtb);    puts("\r\n");

    demo_sbi_base();
    demo_sbi_comparison();
    demo_timer();

    puts("\r\n########################################\r\n");
    puts("#   Demo complete. System halted.       #\r\n");
    puts("########################################\r\n");

    while (1) __asm__ volatile("wfi");
}
