/*
 * ARM Nested Vectored Interrupt Controller
 *
 * Copyright (c) 2006-2007 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the GPL.
 *
 * The ARMv7M System controller is fairly tightly tied in with the
 * NVIC.  Much of that is also implemented here.
 */

#include "hw/sysbus.h"
#include "qemu/timer.h"
#include "hw/arm/arm.h"
#include "target-arm/cpu.h"
#include "exec/address-spaces.h"

/*#define DEBUG_NVIC 0
 */
#ifdef DEBUG_NVIC
#define DPRINTF(LVL, fmt, ...) \
do { if ((LVL) <= DEBUG_NVIC) { \
    fprintf(stderr, "armv7m_nvic: " fmt , ## __VA_ARGS__); \
} } while (0)
#else
#define DPRINTF(LVL, fmt, ...) do {} while (0)
#endif

/* the number of IRQ lines in addition to the 16 internal
 * exception vectors.
 */
#define NVIC_MAX_IRQ 496

#define NVIC_MAX_VECTORS 512

struct VecInfo {
    uint16_t prio_sub; /* sub-group priority*512 + exception# */
    int8_t prio_group; /* group priority [-2, 0x7f] */
    uint8_t raw_prio; /* value writen by guest */
    uint8_t enabled;
    uint8_t pending;
    uint8_t active;
    uint8_t level;
    /* exceptions <=15 never set level */
};
typedef struct VecInfo VecInfo;

struct NVICState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    ARMCPU *cpu; /* NVIC is so closely tied to the CPU, just keep a ref */

    VecInfo vectors[NVIC_MAX_VECTORS];

    uint8_t prigroup;

    struct {
        uint32_t control;
        uint32_t reload;
        int64_t tick;
        QEMUTimer *timer;
    } systick;

    MemoryRegion iomem; /* system control space and NVIC */

    uint32_t num_irq;
    qemu_irq excpout;
    qemu_irq sysresetreq;
};
typedef struct NVICState NVICState;

#define TYPE_NVIC "armv7m_nvic"

#define NVIC(obj) \
    OBJECT_CHECK(NVICState, (obj), TYPE_NVIC)

static const uint8_t nvic_id[] = {
    0x00, 0xb0, 0x1b, 0x00, 0x0d, 0xe0, 0x05, 0xb1
};

/* qemu timers run at 1GHz.   We want something closer to 1MHz.  */
#define SYSTICK_SCALE 1000ULL

#define SYSTICK_ENABLE    (1 << 0)
#define SYSTICK_TICKINT   (1 << 1)
#define SYSTICK_CLKSOURCE (1 << 2)
#define SYSTICK_COUNTFLAG (1 << 16)

int system_clock_scale;

/* Conversion factor from qemu timer to SysTick frequencies.  */
static inline int64_t systick_scale(NVICState *s)
{
    if (s->systick.control & SYSTICK_CLKSOURCE)
        return system_clock_scale;
    else
        return 1000;
}

static void systick_reload(NVICState *s, int reset)
{
    /* The Cortex-M3 Devices Generic User Guide says that "When the
     * ENABLE bit is set to 1, the counter loads the RELOAD value from the
     * SYST RVR register and then counts down". So, we need to check the
     * ENABLE bit before reloading the value.
     */
    if ((s->systick.control & SYSTICK_ENABLE) == 0) {
        return;
    }

    if (reset)
        s->systick.tick = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    s->systick.tick += (s->systick.reload + 1) * systick_scale(s);
    timer_mod(s->systick.timer, s->systick.tick);
}

static void systick_timer_tick(void * opaque)
{
    NVICState *s = (NVICState *)opaque;
    s->systick.control |= SYSTICK_COUNTFLAG;
    if (s->systick.control & SYSTICK_TICKINT) {
        /* Trigger the interrupt.  */
        armv7m_nvic_set_pending(s, ARMV7M_EXCP_SYSTICK);
    }
    if (s->systick.reload == 0) {
        s->systick.control &= ~SYSTICK_ENABLE;
    } else {
        systick_reload(s, 0);
    }
}

static void systick_reset(NVICState *s)
{
    s->systick.control = 0;
    s->systick.reload = 0;
    s->systick.tick = 0;
    timer_del(s->systick.timer);
}

/* caller must call nvic_irq_update() after this */
static
void set_prio(NVICState *s, unsigned irq, uint8_t prio)
{
    unsigned submask = (1<<(s->prigroup+1))-1;

    assert(irq > 3); /* only use for configurable prios */
    assert(irq < NVIC_MAX_VECTORS);

    prio &= s->cpu->v7m_priority_mask;

    s->vectors[irq].raw_prio = prio;
    s->vectors[irq].prio_group = (prio>>(s->prigroup+1));
    s->vectors[irq].prio_sub = irq + (prio & submask) * NVIC_MAX_VECTORS;

    DPRINTF(0, "Set %u priority grp %d sub %u (prigroup = %u)\n", irq,
            s->vectors[irq].prio_group, s->vectors[irq].prio_sub,
            (unsigned)s->prigroup);
}

/* recompute highest pending */
static
void nvic_irq_update(NVICState *s)
{
    unsigned i;
    int lvl;
    CPUARMState *env = &s->cpu->env;
    int16_t pend_group = 0x100;
    uint16_t pend_sub = 0, pend_irq = 0;

    /* find highest priority */
    for (i = 1; i < s->num_irq; i++) {
        VecInfo *vec = &s->vectors[i];

        DPRINTF(2, " VECT %d %d:%u\n", i, vec->prio_group, vec->prio_sub);

        if (vec->enabled && vec->pending && ((vec->prio_group < pend_group)
                || (vec->prio_group == pend_group
                    && vec->prio_sub < pend_sub)))
        {
            pend_group = vec->prio_group;
            pend_sub = vec->prio_sub;
            pend_irq = i;
        }
    }

    env->v7m.pending = pend_irq;
    env->v7m.pending_prio = pend_group;

    /* Raise NVIC output even if pend_group is masked.
     * This is necessary as we get no notification
     * when PRIMASK et al. are changed.
     * As long as our output is high cpu_exec() will call
     * into arm_v7m_cpu_exec_interrupt() frequently, which
     * then tests to see if the pending exception
     * is permitted.
     */
    lvl = pend_irq > 0;
    DPRINTF(0, "IRQ %c highest pending %d %d:%u\n",
            lvl ? 'X' : '_',
            pend_irq, pend_group, pend_sub);

    qemu_set_irq(s->excpout, lvl);
}

static
void armv7m_nvic_clear_pending(void *opaque, int irq)
{
    NVICState *s = (NVICState *)opaque;
    VecInfo *vec;

    assert(irq >= 0);
    assert(irq < NVIC_MAX_VECTORS);

    vec = &s->vectors[irq];
    if (vec->pending) {
        vec->pending = 0;
        nvic_irq_update(s);
    }
}

int armv7m_nvic_get_active_prio(void *opaque)
{
    NVICState *s = (NVICState *)opaque;
    unsigned i;
    int16_t group = 0x100;
    uint16_t sub = 0xff;

    /* don't consider env->v7m.exception
     * as we are called while it is inconsistent
     */

    for (i = 1; i < s->num_irq; i++) {
        VecInfo *vec = &s->vectors[i];
        if (!vec->active) {
            continue;
        }
        if (vec->prio_group < group ||
                (vec->prio_group == group &&
                 vec->prio_sub < sub))
        {
            group = vec->prio_group;
            sub = vec->prio_sub;
        }
    }

    return group;
}

/* @returns the active (running) exception priority.
 *    only a higher (numerically lower) priority can preempt.
 */
int armv7m_excp_running_prio(ARMCPU *cpu)
{
    CPUARMState *env = &cpu->env;
    NVICState *s = env->nvic;
    int running;

    if (env->daif & PSTATE_F) { /* FAULTMASK */
        running = -1;
    } else if (env->daif & PSTATE_I) { /* PRIMASK */
        running = 0;
    } else if (env->v7m.basepri > 0) {
        /* BASEPRI==1 -> masks [1,255] (not same as PRIMASK==1) */
        running = env->v7m.basepri >> (s->prigroup+1);
    } else {
        running = 0x100; /* lower than any possible priority */
    }
    /* consider priority of active handler */
    return MIN(running, env->v7m.exception_prio);
}

void armv7m_nvic_set_pending(void *opaque, int irq)
{
    NVICState *s = (NVICState *)opaque;
    CPUARMState *env = &s->cpu->env;
    VecInfo *vec;
    int active = s->cpu->env.v7m.exception;

    assert(irq > 1); /* don't pend reset */
    assert(irq < s->num_irq);

    vec = &s->vectors[irq];

    if (irq < ARMV7M_EXCP_PENDSV
            && irq != ARMV7M_EXCP_DEBUG
            && irq != ARMV7M_EXCP_NMI)
    {
        int running = armv7m_excp_running_prio(s->cpu);
        /* test for exception escalation for vectors other than:
         * NMI (2), Debug (12), PendSV (14), SysTick (15),
         * and all external IRQs (>=16).
         * This assumes that all such exceptions are precise (sync.)
         * and that we don't simulate imprecise (async.) faults.
         * Some Debug exceptions should be escalated, however
         * this exception is presently unused.
         */
        unsigned escalate = 0;
        if (vec->prio_group >= running) {
            /* Trying to pend a fault which is not immediately
             * runnable due to masking by PRIMASK, FAULTMASK, BASEPRI,
             * or the priority of an active exception
             */
            DPRINTF(0, " Escalate, insufficient priority %d >= %d\n",
                    vec->prio_group, running);
            escalate = 1;

        } else if (!vec->enabled) {
            /* trying to pend a disabled fault
             * eg. UsageFault while USGFAULTENA in SHCSR is clear.
             */
            escalate = 1;
            DPRINTF(0, " Escalate, not enabled\n");

        } else if (vec->active) {
            /* This case should only be reached if some logic error
             * has caused env->exception_prio to get out of sync with
             * the active exception priorities.
             */
            hw_error("exception priorities are out of sync\n");
        }

        if (escalate) {
#ifdef DEBUG_NVIC
            int oldirq = irq;
#endif
            if (running < 0) {
                /* TODO: actual unrecoverable exception actions */
                cpu_abort(&s->cpu->parent_obj,
                          "%d in %d escalates to unrecoverable exception\n",
                          irq, active);
            }
            irq = ARMV7M_EXCP_HARD;
            vec = &s->vectors[irq];

            s->cpu->env.v7m.hfsr |= 1<<30; /* FORCED */
            DPRINTF(0, "Escalate %d to HardFault\n", oldirq);
        }
    }

    vec->pending = 1;
    if (vec->enabled && (vec->prio_group < env->v7m.pending_prio)) {
        env->v7m.pending_prio = vec->prio_group;
        env->v7m.pending = irq;
        qemu_set_irq(s->excpout, irq > 0);
    }
    DPRINTF(0, "Pending %d at %d%s running %d\n",
            irq, vec->prio_group,
            env->v7m.pending == irq ? " (highest)" : "",
            armv7m_excp_running_prio(s->cpu));
}

bool armv7m_nvic_is_active(void *opaque, int irq)
{
    NVICState *s = (NVICState *)opaque;

    assert(irq > 0 && irq < s->num_irq);
    return s->vectors[irq].active;
}

/* Make pending IRQ active.  */
void armv7m_nvic_acknowledge_irq(void *opaque)
{
    NVICState *s = (NVICState *)opaque;
    CPUARMState *env = &s->cpu->env;
    const int pending = env->v7m.pending;
    const int running = armv7m_excp_running_prio(s->cpu);
    VecInfo *vec;

    if (!pending) {
        hw_error("Interrupt but no vector\n");
    }

    assert(pending < s->num_irq);
    vec = &s->vectors[pending];

    assert(vec->enabled);

    assert(env->v7m.pending_prio == vec->prio_group);
    if (env->v7m.pending_prio >= running) {
        hw_error("Interrupt ack. while masked %d >= %d",
                 env->v7m.pending_prio, running);
    }

    DPRINTF(0, "ACT %d at %d\n", pending, vec->prio_group);

    assert(vec->pending);
    vec->active = 1;
    vec->pending = 0;

    env->v7m.exception = env->v7m.pending;
    env->v7m.exception_prio = env->v7m.pending_prio;

    nvic_irq_update(s); /* recalc pending */

    assert(env->v7m.exception > 0); /* spurious exception? */
}

bool armv7m_nvic_complete_irq(void *opaque, int irq)
{
    NVICState *s = (NVICState *)opaque;
    VecInfo *vec;

    assert(irq > 0);
    assert(irq < NVIC_MAX_VECTORS);

    vec = &s->vectors[irq];

    if (!vec->active) {
        return true;
    }

    vec->active = 0;
    vec->pending = vec->level;
    assert(!vec->level || irq >= 16);

    nvic_irq_update(s);
    DPRINTF(0, "EOI %d\n", irq);
    return false;
}

/* Only called for external interrupt (vector>=16) */
static
void set_irq_level(void *opaque, int n, int level)
{
    NVICState *s = opaque;
    VecInfo *vec;

    assert(n >= 0);
    assert(n < NVIC_MAX_IRQ);

    n += 16;

    if (n >= s->num_irq) {
        return;
    }

    /* The pending status of an external interrupt is
     * latched on rising edge and exception handler return.
     *
     * Pulsing the IRQ will always run the handler
     * once, and the handler will re-run until the
     * level is low when the handler completes.
     */
    vec = &s->vectors[n];
    vec->level = level;
    if (level) {
        DPRINTF(1, "assert IRQ %d\n", n-16);
        armv7m_nvic_set_pending(s, n-16);
    } else {
        DPRINTF(2, "deassert IRQ %d\n", n-16);
    }
}

static uint32_t nvic_readl(NVICState *s, uint32_t offset)
{
    ARMCPU *cpu = s->cpu;
    uint32_t val;
    int irq;

    switch (offset) {
    case 4: /* Interrupt Control Type.  */
        return ((s->num_irq - 16) / 32) - 1;
    case 0x10: /* SysTick Control and Status.  */
        val = s->systick.control;
        s->systick.control &= ~SYSTICK_COUNTFLAG;
        return val;
    case 0x14: /* SysTick Reload Value.  */
        return s->systick.reload;
    case 0x18: /* SysTick Current Value.  */
        {
            int64_t t;
            if ((s->systick.control & SYSTICK_ENABLE) == 0)
                return 0;
            t = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
            if (t >= s->systick.tick)
                return 0;
            val = ((s->systick.tick - (t + 1)) / systick_scale(s)) + 1;
            /* The interrupt in triggered when the timer reaches zero.
               However the counter is not reloaded until the next clock
               tick.  This is a hack to return zero during the first tick.  */
            if (val > s->systick.reload)
                val = 0;
            return val;
        }
    case 0x1c: /* SysTick Calibration Value.  */
        return 10000;
    case 0xd00: /* CPUID Base.  */
        return cpu->midr;
    case 0xd04: /* Interrupt Control State.  */
        /* VECTACTIVE */
        val = cpu->env.v7m.exception;
        /* VECTPENDING */
        val |= (cpu->env.v7m.pending & 0xff) << 12;
        /* ISRPENDING - Set it any externel IRQ pending (vector>=16) */
        for (irq = 16; irq < s->num_irq; irq++) {
            if (s->vectors[irq].pending) {
                val |= (1 << 22);
                break;
            }
        }
        /* RETTOBASE - Set if only one handler is active */
    {
        unsigned nhand = 0;
        for (irq = 1; irq < s->num_irq; irq++) {
            if (s->vectors[irq].active) {
                nhand++;
                if (nhand == 2) {
                    break;
                }
            }
        }
        val |= nhand == 1 ? (1<<11) : 0;
    }
        /* PENDSTSET */
        if (s->vectors[ARMV7M_EXCP_SYSTICK].pending) {
            val |= (1 << 26);
        }
        /* PENDSVSET */
        if (s->vectors[ARMV7M_EXCP_PENDSV].pending) {
            val |= (1 << 28);
        }
        /* NMIPENDSET */
        if (s->vectors[ARMV7M_EXCP_NMI].pending) {
            val |= (1 << 31);
        }
        /* ISRPREEMPT not implemented */
        return val;
    case 0xd08: /* Vector Table Offset.  */
        return cpu->env.v7m.vecbase;
    case 0xd0c: /* Application Interrupt/Reset Control.  */
        return 0xfa050000 | (s->prigroup<<8);
    case 0xd10: /* System Control.  */
        /* TODO: Implement SLEEPONEXIT.  */
        return 0;
    case 0xd14: /* Configuration Control.  */
        return cpu->env.v7m.ccr;
    case 0xd24: /* System Handler Status.  */
        val = 0;
        if (s->vectors[ARMV7M_EXCP_MEM].active) val |= (1 << 0);
        if (s->vectors[ARMV7M_EXCP_BUS].active) val |= (1 << 1);
        if (s->vectors[ARMV7M_EXCP_USAGE].active) val |= (1 << 3);
        if (s->vectors[ARMV7M_EXCP_SVC].active) val |= (1 << 7);
        if (s->vectors[ARMV7M_EXCP_DEBUG].active) val |= (1 << 8);
        if (s->vectors[ARMV7M_EXCP_PENDSV].active) val |= (1 << 10);
        if (s->vectors[ARMV7M_EXCP_SYSTICK].active) val |= (1 << 11);
        if (s->vectors[ARMV7M_EXCP_USAGE].pending) val |= (1 << 12);
        if (s->vectors[ARMV7M_EXCP_MEM].pending) val |= (1 << 13);
        if (s->vectors[ARMV7M_EXCP_BUS].pending) val |= (1 << 14);
        if (s->vectors[ARMV7M_EXCP_SVC].pending) val |= (1 << 15);
        if (s->vectors[ARMV7M_EXCP_MEM].enabled) val |= (1 << 16);
        if (s->vectors[ARMV7M_EXCP_BUS].enabled) val |= (1 << 17);
        if (s->vectors[ARMV7M_EXCP_USAGE].enabled) val |= (1 << 18);
        return val;
    case 0xd28: /* Configurable Fault Status.  */
        return cpu->env.v7m.cfsr;
    case 0xd2c: /* Hard Fault Status.  */
        return cpu->env.v7m.hfsr;
    case 0xd30: /* Debug Fault Status.  */
        qemu_log_mask(LOG_UNIMP, "Debug Fault status register unimplemented\n");
        return 0;
    case 0xd34: /* MMFAR MemManage Fault Address */
        return cpu->env.v7m.mmfar;
    case 0xd38: /* Bus Fault Address.  */
        return cpu->env.v7m.bfar;
    case 0xd3c: /* Aux Fault Status.  */
        /* TODO: Implement fault status registers.  */
        qemu_log_mask(LOG_UNIMP,
                      "Aux Fault status registers unimplemented\n");
        return 0;
    case 0xd40: /* PFR0.  */
        return 0x00000030;
    case 0xd44: /* PRF1.  */
        return 0x00000200;
    case 0xd48: /* DFR0.  */
        return 0x00100000;
    case 0xd4c: /* AFR0.  */
        return 0x00000000;
    case 0xd50: /* MMFR0.  */
        return 0x00000030;
    case 0xd54: /* MMFR1.  */
        return 0x00000000;
    case 0xd58: /* MMFR2.  */
        return 0x00000000;
    case 0xd5c: /* MMFR3.  */
        return 0x00000000;
    case 0xd60: /* ISAR0.  */
        return 0x01141110;
    case 0xd64: /* ISAR1.  */
        return 0x02111000;
    case 0xd68: /* ISAR2.  */
        return 0x21112231;
    case 0xd6c: /* ISAR3.  */
        return 0x01111110;
    case 0xd70: /* ISAR4.  */
        return 0x01310102;
    /* TODO: Implement debug registers.  */
    case 0xd90: /* MPU_TYPE */
        return cpu->has_mpu ? (cpu->pmsav7_dregion<<8) : 0;
        break;
    case 0xd94: /* MPU_CTRL */
        val = 0;
        /* We only use sctlr_el[1] since v7m has only two ELs unpriv. (0)
         * and priv. (1).  The "controlling" EL is always priv.
         */
        if (cpu->env.cp15.sctlr_el[1] & SCTLR_M) {
            val |= 1; /* ENABLE */
        }
        if (cpu->env.v7m.mpu_hfnmiena) {
            val |= 2; /* HFNMIENA */
        }
        if (cpu->env.cp15.sctlr_el[1] & SCTLR_BR) {
            val |= 4; /* PRIVDEFENA */
        }
        return val;
    case 0xd98: /* MPU_RNR */
        return cpu->env.cp15.c6_rgnr;
    case 0xd9c: /* MPU_RBAR */
    case 0xda4: /* MPU_RBAR_A1 */
    case 0xdac: /* MPU_RBAR_A2 */
    case 0xdb4: /* MPU_RBAR_A3 */
    {
        uint32_t range;
        if (offset == 0xd9c) {
            range = cpu->env.cp15.c6_rgnr;
        } else {
            range = (offset - 0xda4)/8;
        }

        if (range >= cpu->pmsav7_dregion) {
            return 0;
        } else {
            return (cpu->env.pmsav7.drbar[range] & (0x1f)) | (range & 0xf);
        }
    }
    case 0xda0: /* MPU_RASR */
    case 0xda8: /* MPU_RASR_A1 */
    case 0xdb0: /* MPU_RASR_A2 */
    case 0xdb8: /* MPU_RASR_A3 */
    {
        uint32_t range;

        if (offset == 0xda0) {
            range = cpu->env.cp15.c6_rgnr;
        } else {
            range = (offset - 0xda8)/8;
        }

        if (range >= cpu->pmsav7_dregion) {
            return 0;
        } else {
            return ((cpu->env.pmsav7.dracr[range] & 0xffff)<<16)
                    | (cpu->env.pmsav7.drsr[range] & 0xffff);
        }
    }
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "NVIC: Bad read offset 0x%x\n", offset);
        return 0;
    }
}

static void nvic_writel(NVICState *s, uint32_t offset, uint32_t value)
{
    ARMCPU *cpu = s->cpu;
    uint32_t oldval;
    switch (offset) {
    case 0x10: /* SysTick Control and Status.  */
        oldval = s->systick.control;
        s->systick.control &= 0xfffffff8;
        s->systick.control |= value & 7;
        if ((oldval ^ value) & SYSTICK_ENABLE) {
            int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
            if (value & SYSTICK_ENABLE) {
                if (s->systick.tick) {
                    s->systick.tick += now;
                    timer_mod(s->systick.timer, s->systick.tick);
                } else {
                    systick_reload(s, 1);
                }
            } else {
                timer_del(s->systick.timer);
                s->systick.tick -= now;
                if (s->systick.tick < 0)
                  s->systick.tick = 0;
            }
        } else if ((oldval ^ value) & SYSTICK_CLKSOURCE) {
            /* This is a hack. Force the timer to be reloaded
               when the reference clock is changed.  */
            systick_reload(s, 1);
        }
        break;
    case 0x14: /* SysTick Reload Value.  */
        s->systick.reload = value;
        break;
    case 0x18: /* SysTick Current Value.  Writes reload the timer.  */
        systick_reload(s, 1);
        s->systick.control &= ~SYSTICK_COUNTFLAG;
        break;
    case 0xd04: /* Interrupt Control State.  */
        if (value & (1 << 31)) {
            armv7m_nvic_set_pending(s, ARMV7M_EXCP_NMI);
        }
        if (value & (1 << 28)) {
            armv7m_nvic_set_pending(s, ARMV7M_EXCP_PENDSV);
        } else if (value & (1 << 27)) {
            armv7m_nvic_clear_pending(s, ARMV7M_EXCP_PENDSV);
        }
        if (value & (1 << 26)) {
            armv7m_nvic_set_pending(s, ARMV7M_EXCP_SYSTICK);
        } else if (value & (1 << 25)) {
            armv7m_nvic_clear_pending(s, ARMV7M_EXCP_SYSTICK);
        }
        break;
    case 0xd08: /* Vector Table Offset.  */
        cpu->env.v7m.vecbase = value & 0xffffff80;
        break;
    case 0xd0c: /* Application Interrupt/Reset Control.  */
        if ((value >> 16) == 0x05fa) {
            if (value & 4) {
                qemu_irq_pulse(s->sysresetreq);
            }
            if (value & 2) {
                qemu_log_mask(LOG_GUEST_ERROR,
                              "Setting VECTCLRACTIVE when not in DEBUG mode "
                              "is UNPREDICTABLE\n");
            }
            if (value & 1) {
                qemu_log_mask(LOG_GUEST_ERROR,
                              "Setting VECTRESET when not in DEBUG mode "
                              "is UNPREDICTABLE\n");
            }
            if (value & 0x700) {
                unsigned i;
                s->prigroup = (value>>8) & 0xf;
                /* recalculate priorities for exceptions w/ configurable prio */
                for (i = 4; i < s->num_irq; i++) {
                    set_prio(s, i, s->vectors[i].raw_prio);
                }
                nvic_irq_update(s);
                cpu->env.v7m.exception_prio = armv7m_nvic_get_active_prio(s);
            }
        }
        break;
    case 0xd10: /* System Control.  */
        /* TODO: Implement control registers.  */
        qemu_log_mask(LOG_UNIMP, "NVIC: SCR unimplemented\n");
        break;
    case 0xd14: /* Configuration Control.  */
        value &= 0x31b;
        if (value & 0x118) {
            qemu_log_mask(LOG_UNIMP, "CCR unimplemented bits"
                                     " BFHFNMIGN, DIV_0_TRP, UNALIGN_TRP");
            value &= ~0x118;
        }
        cpu->env.v7m.ccr = value;
        break;
    case 0xd24: /* System Handler Control.  */
        /* TODO: Real hardware allows you to set/clear the active bits
           under some circumstances.  We don't implement this.  */
        s->vectors[ARMV7M_EXCP_MEM].enabled = (value & (1 << 16)) != 0;
        s->vectors[ARMV7M_EXCP_BUS].enabled = (value & (1 << 17)) != 0;
        s->vectors[ARMV7M_EXCP_USAGE].enabled = (value & (1 << 18)) != 0;
        /* no need to call nvic_irq_update() since any pending while
         * disabled would have been escalated to HardFault
         */
        break;
    case 0xd28: /* Configurable Fault Status.  */
        cpu->env.v7m.cfsr &= ~value; /* W1C */
        break;
    case 0xd2c: /* Hard Fault Status.  */
        cpu->env.v7m.hfsr &= ~value; /* W1C */
        break;
    case 0xd30: /* Debug Fault Status.  */
        qemu_log_mask(LOG_UNIMP,
                      "NVIC: debug fault status register unimplemented\n");
        break;
    case 0xd34: /* Mem Manage Address.  */
        cpu->env.v7m.mmfar = value;
        return;
    case 0xd38: /* Bus Fault Address.  */
        cpu->env.v7m.bfar = value;
        return;
    case 0xd3c: /* Aux Fault Status.  */
        qemu_log_mask(LOG_UNIMP,
                      "NVIC: Aux fault status registers unimplemented\n");
        break;
    case 0xd90: /* MPU_TYPE (0xe000ed90) */
        return; /* RO */
    case 0xd94: /* MPU_CTRL */
    {
        if ((value & 3) == 2) {
            qemu_log_mask(LOG_GUEST_ERROR, "MPU_CTRL: HFNMIENA and !ENABLE is "
                          "UNPREDICTABLE\n");
            /* we choice to ignore HFNMIENA when the MPU
             * is not enabled.
             */
            value &= ~2;
        }
        if (value & 1) {
            cpu->env.cp15.sctlr_el[1] |= SCTLR_M;
        } else {
            cpu->env.cp15.sctlr_el[1] &= ~SCTLR_M;
        }
        cpu->env.v7m.mpu_hfnmiena = !!(value & 2);
        if (value & 4) {
            cpu->env.cp15.sctlr_el[1] |= SCTLR_BR;
        } else {
            cpu->env.cp15.sctlr_el[1] &= ~SCTLR_BR;
        }
        tlb_flush(CPU(cpu), 1);
    }
        break;
    case 0xd98: /* MPU_RNR */
        if (value >= cpu->pmsav7_dregion) {
            qemu_log_mask(LOG_GUEST_ERROR, "MPU region out of range %u/%u\n",
                          (unsigned)value, (unsigned)cpu->pmsav7_dregion);
        } else {
            cpu->env.cp15.c6_rgnr = value;
            DPRINTF(0, "MPU -> RGNR = %u\n", (unsigned)value);
        }
        tlb_flush(CPU(cpu), 1); /* necessary? */
        break;
    case 0xd9c: /* MPU_RBAR */
    case 0xda4: /* MPU_RBAR_A1 */
    case 0xdac: /* MPU_RBAR_A2 */
    case 0xdb4: /* MPU_RBAR_A3 */
    {
        uint32_t range;
        uint32_t base = value;

        if (offset == 0xd9c) {
            range = cpu->env.cp15.c6_rgnr;
        } else {
            range = (offset - 0xda4)/8;
        }

        if (value & (1<<4)) {
            range = value & 0xf;

            if (range >= cpu->pmsav7_dregion) {
                qemu_log_mask(LOG_GUEST_ERROR,
                              "MPU region out of range %u/%u\n",
                              (unsigned)range,
                              (unsigned)cpu->pmsav7_dregion);
                return;
            }
            cpu->env.cp15.c6_rgnr = range;
            base &= ~0x1f;

        } else if (range >= cpu->pmsav7_dregion) {
            return;
        }

        cpu->env.pmsav7.drbar[range] = base & ~0x3;
        DPRINTF(0, "MPU -> DRBAR[%u] = %08x\n", range,
                cpu->env.pmsav7.drbar[range]);
    }
        tlb_flush(CPU(cpu), 1);
        break;
    case 0xda0: /* MPU_RASR */
    case 0xda8: /* MPU_RASR_A1 */
    case 0xdb0: /* MPU_RASR_A2 */
    case 0xdb8: /* MPU_RASR_A3 */
    {
        uint32_t range;

        if (offset == 0xda0) {
            range = cpu->env.cp15.c6_rgnr;
        } else {
            range = (offset-0xda8)/8;
        }

        cpu->env.pmsav7.drsr[range] = value & 0xff3f;
        cpu->env.pmsav7.dracr[range] = (value>>16) & 0x173f;
        DPRINTF(0, "MPU -> DRSR[%u] = %08x DRACR[%u] = %08x\n",
                range, cpu->env.pmsav7.drsr[range],
                range, cpu->env.pmsav7.dracr[range]);
    }
        tlb_flush(CPU(cpu), 1);
        break;
    case 0xf00: /* Software Triggered Interrupt Register */
        /* STIR write allowed if privlaged or USERSETMPEND set */
        if ((arm_current_el(&cpu->env) || (cpu->env.v7m.ccr & 2))
            && ((value & 0x1ff) < NVIC_MAX_IRQ)) {
            armv7m_nvic_set_pending(s, (value & 0x1ff)+16);
        }
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "NVIC: Bad write offset 0x%x\n", offset);
    }
}

static uint64_t nvic_sysreg_read(void *opaque, hwaddr addr,
                                 unsigned size)
{
    NVICState *s = (NVICState *)opaque;
    uint32_t offset = addr;
    unsigned i, end;
    uint32_t val;

    switch (offset) {
    /* reads of set and clear both return the status */
    case 0x100 ... 0x13c: /* NVIC Set enable */
        offset += 0x80;
        /* fall through */
    case 0x180 ... 0x1bc: /* NVIC Clear enable */
        val = 0;
        offset = offset-0x180+16; /* vector # */

        for (i = 0, end = size*8; i < end && offset+i < s->num_irq; i++) {
            if (s->vectors[offset+i].enabled) {
                val |= (1<<i);
            }
        }
        break;
    case 0x200 ... 0x23c: /* NVIC Set pend */
        offset += 0x80;
        /* fall through */
    case 0x280 ... 0x2bc: /* NVIC Clear pend */
        val = 0;
        offset = offset-0x280+16; /* vector # */

        for (i = 0, end = size*8; i < end && offset+i < s->num_irq; i++) {
            if (s->vectors[offset+i].pending) {
                val |= (1<<i);
            }
        }
        break;
    case 0x300 ... 0x37c: /* NVIC Active */
        val = 0;
        offset = offset-0x300+16; /* vector # */

        for (i = 0, end = size*8; i < end && offset+i < s->num_irq; i++) {
            if (s->vectors[offset+i].active) {
                val |= (1<<i);
            }
        }
        break;
    case 0x400 ... 0x7ec: /* NVIC Priority */
        val = 0;
        offset = offset-0x400+16; /* vector # */

        for (i = 0; i < size && offset+i < s->num_irq; i++) {
            val |= s->vectors[offset+i].raw_prio<<(8*i);
        }
        break;
    case 0xd18 ... 0xd23: /* System Handler Priority.  */
        val = 0;
        for (i = 0; i < size; i++) {
            val |= s->vectors[(offset - 0xd14) + i].raw_prio << (i * 8);
        }
        break;
    case 0xfe0 ... 0xfff: /* ID.  */
        if (offset & 3) {
            return 0;
        }
        val = nvic_id[(offset - 0xfe0) >> 2];
        break;
    default:
        if (size == 4) {
            val = nvic_readl(s, offset);
        } else {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "NVIC: Bad read of size %d at offset 0x%x\n",
                          size, offset);
            val = 0;
        }
    }

    DPRINTF(0, "sysreg read%u "TARGET_FMT_plx" -> %08x\n",
            size*8, addr, (unsigned)val);
    return val;
}

static void nvic_sysreg_write(void *opaque, hwaddr addr,
                              uint64_t value, unsigned size)
{
    NVICState *s = (NVICState *)opaque;
    uint32_t offset = addr;
    unsigned i, end;
    unsigned setval = 0;

    DPRINTF(0, "sysreg write%u "TARGET_FMT_plx" <- %08x\n",
            size*8, addr, (unsigned)value);

    switch (offset) {
    case 0x100 ... 0x13c: /* NVIC Set enable */
        offset += 0x80;
        setval = 1;
        /* fall through */
    case 0x180 ... 0x1bc: /* NVIC Clear enable */
        offset = offset-0x180+16; /* vector # */

        for (i = 0, end = size*8; i < end && offset+i < s->num_irq; i++) {
            if (value & (1<<i)) {
                s->vectors[offset+i].enabled = setval;
            }
        }
        nvic_irq_update(s);
        return;
    case 0x200 ... 0x23c: /* NVIC Set pend */
        /* the special logic in armv7m_nvic_set_pending()
         * is not needed since IRQs are never escalated
         */
        offset += 0x80;
        setval = 1;
        /* fall through */
    case 0x280 ... 0x2bc: /* NVIC Clear pend */
        offset = offset-0x280+16; /* vector # */

        for (i = 0, end = size*8; i < end && offset+i < s->num_irq; i++) {
            if (value & (1<<i)) {
                s->vectors[offset+i].pending = setval;
            }
        }
        nvic_irq_update(s);
        return;
    case 0x300 ... 0x37c: /* NVIC Active */
        return; /* R/O */
    case 0x400 ... 0x7ec: /* NVIC Priority */
        offset = offset-0x400+16; /* vector # */

        for (i = 0; i < size; i++) {
            set_prio(s, offset+i, (value>>(i*8)) & 0xff);
        }
        nvic_irq_update(s);
        s->cpu->env.v7m.exception_prio = armv7m_nvic_get_active_prio(s);
        return;
    case 0xd18 ... 0xd23: /* System Handler Priority.  */
        for (i = 0; i < size; i++) {
            unsigned hdlidx = (offset - 0xd14) + i;
            set_prio(s, hdlidx, (value >> (i * 8)) & 0xff);
            DPRINTF(0, "Set Handler prio %u = %u\n",
                    (unsigned)hdlidx,
                    (unsigned)s->vectors[hdlidx].raw_prio);
        }
        nvic_irq_update(s);
        s->cpu->env.v7m.exception_prio = armv7m_nvic_get_active_prio(s);
        return;
    default:
        if (size == 4) {
            nvic_writel(s, offset, value);
            return;
        }
        qemu_log_mask(LOG_GUEST_ERROR,
                      "NVIC: Bad write of size %d at offset 0x%x\n",
                      size, offset);
    }
}

static const MemoryRegionOps nvic_sysreg_ops = {
    .read = nvic_sysreg_read,
    .write = nvic_sysreg_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static
int nvic_post_load(void *opaque, int version_id)
{
    NVICState *s = opaque;
    unsigned i;

    /* recalculate priorities */
    for (i = 4; i < s->num_irq; i++) {
        set_prio(s, i, s->vectors[i].raw_prio);
    }

    nvic_irq_update(s);
    s->cpu->env.v7m.exception_prio = armv7m_nvic_get_active_prio(s);

    return 0;
}

static const VMStateDescription vmstate_VecInfo = {
    .name = "armv7m_nvic_info",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT16(prio_sub, VecInfo),
        VMSTATE_INT8(prio_group, VecInfo),
        VMSTATE_UINT8(raw_prio, VecInfo),
        VMSTATE_UINT8(enabled, VecInfo),
        VMSTATE_UINT8(pending, VecInfo),
        VMSTATE_UINT8(active, VecInfo),
        VMSTATE_UINT8(level, VecInfo),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription vmstate_nvic = {
    .name = "armv7m_nvic",
    .version_id = 2,
    .minimum_version_id = 2,
    .post_load = &nvic_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_STRUCT_ARRAY(vectors, NVICState, NVIC_MAX_VECTORS, 1,
                             vmstate_VecInfo, VecInfo),
        VMSTATE_UINT8(prigroup, NVICState),
        VMSTATE_UINT32(systick.control, NVICState),
        VMSTATE_UINT32(systick.reload, NVICState),
        VMSTATE_INT64(systick.tick, NVICState),
        VMSTATE_TIMER_PTR(systick.timer, NVICState),
        VMSTATE_UINT32(num_irq, NVICState),
        VMSTATE_END_OF_LIST()
    }
};

static Property props_nvic[] = {
    DEFINE_PROP_UINT32("num-irq", NVICState, num_irq, 64),
    DEFINE_PROP_END_OF_LIST()
};

static void armv7m_nvic_reset(DeviceState *dev)
{
    NVICState *s = NVIC(dev);

    s->vectors[ARMV7M_EXCP_NMI].enabled = 1;
    s->vectors[ARMV7M_EXCP_HARD].enabled = 1;
    s->vectors[ARMV7M_EXCP_SVC].enabled = 1;
    s->vectors[ARMV7M_EXCP_DEBUG].enabled = 1;
    s->vectors[ARMV7M_EXCP_PENDSV].enabled = 1;

    s->vectors[ARMV7M_EXCP_RESET].prio_group = -3;
    s->vectors[ARMV7M_EXCP_NMI].prio_group = -2;
    s->vectors[ARMV7M_EXCP_HARD].prio_group = -1;

    /* strictly speaking the reset handler should be enabled.
     * However, we don't simulate soft resets through the NVIC,
     * and the reset vector should never be pended.
     * So don't enabled to catch logic errors.
    s->vectors[ARMV7M_EXCP_RESET].enabled = 1;
     */

    systick_reset(s);
}

static void armv7m_nvic_realize(DeviceState *dev, Error **errp)
{
    NVICState *s = NVIC(dev);

    s->cpu = ARM_CPU(first_cpu);

    if (s->num_irq > NVIC_MAX_IRQ) {
        error_setg(errp, TYPE_NVIC " num-irq too large");
        return;

    } else if (s->num_irq & 0x1f) {
        error_setg(errp, TYPE_NVIC " num-irq must be a multiple of 32");
        return;
    }

    qdev_init_gpio_in(dev, set_irq_level, s->num_irq);

    s->num_irq += 16; /* include space for internal exception vectors */

    /* The NVIC and system controller register area starts at 0xe000e000
     * and looks like this:
     *  0x004 - ICTR
     *  0x010 - 0x1c - systick
     *  0x100..0x7ec - NVIC
     *  0x7f0..0xcff - Reserved
     *  0xd00..0xd3c - SCS registers
     *  0xd40..0xeff - Reserved or Not implemented
     *  0xf00 - STIR
     */

    memory_region_init_io(&s->iomem, OBJECT(s), &nvic_sysreg_ops, s,
                          "nvic_sysregs", 0x1000);

    /* Map the whole thing into system memory at the location required
     * by the v7M architecture.
     */
    memory_region_add_subregion(get_system_memory(), 0xe000e000, &s->iomem);
    s->systick.timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, systick_timer_tick, s);
}

static void armv7m_nvic_instance_init(Object *obj)
{
    /* We have a different default value for the num-irq property
     * than our superclass. This function runs after qdev init
     * has set the defaults from the Property array and before
     * any user-specified property setting, so just modify the
     * value in the GICState struct.
     */
    DeviceState *dev = DEVICE(obj);
    NVICState *nvic = NVIC(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    sysbus_init_irq(sbd, &nvic->excpout);
    qdev_init_gpio_out_named(dev, &nvic->sysresetreq, "SYSRESETREQ", 1);
}

static void armv7m_nvic_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd  = &vmstate_nvic;
    dc->props = props_nvic;
    dc->reset = armv7m_nvic_reset;
    dc->realize = armv7m_nvic_realize;
}

static const TypeInfo armv7m_nvic_info = {
    .name          = TYPE_NVIC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_init = armv7m_nvic_instance_init,
    .instance_size = sizeof(NVICState),
    .class_init    = armv7m_nvic_class_init,
    .class_size    = sizeof(SysBusDeviceClass),
};

static void armv7m_nvic_register_types(void)
{
    type_register_static(&armv7m_nvic_info);
}

type_init(armv7m_nvic_register_types)
