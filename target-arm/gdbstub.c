/*
 * ARM gdb server stub
 *
 * Copyright (c) 2003-2005 Fabrice Bellard
 * Copyright (c) 2013 SUSE LINUX Products GmbH
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"
#include "qemu-common.h"
#include "exec/gdbstub.h"

/* Old gdb always expect FPA registers.  Newer (xml-aware) gdb only expect
   whatever the target description contains.  Due to a historical mishap
   the FPA registers appear in between core integer regs and the CPSR.
   We hack round this by giving the FPA regs zero size when talking to a
   newer gdb.  */

int arm_cpu_gdb_read_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    ARMCPU *cpu = ARM_CPU(cs);
    CPUARMState *env = &cpu->env;

    if (n < 16) {
        /* Core integer register.  */
        return gdb_get_reg32(mem_buf, env->regs[n]);
    }
    if (n < 24) {
        /* FPA registers.  */
        if (gdb_has_xml) {
            return 0;
        }
        memset(mem_buf, 0, 12);
        return 12;
    }
    switch (n) {
    case 24:
        /* FPA status register.  */
        if (gdb_has_xml) {
            return 0;
        }
        return gdb_get_reg32(mem_buf, 0);
    case 25:
        if (IS_M(env)) {
            /* xPSR */
            return gdb_get_reg32(mem_buf, xpsr_read(env));
        }
        /* CPSR */
        return gdb_get_reg32(mem_buf, cpsr_read(env));
    }
    if (IS_M(env)) {
        switch( n ) {
        case 26:
            /* MSP */
            return gdb_get_reg32(mem_buf, !!(env->v7m.control & 2) ?
                env->v7m.other_sp : env->regs[13]);
        case 27:
            /* PSP */
            return gdb_get_reg32(mem_buf, !!(env->v7m.control & 2) ?
                env->regs[13] : env->v7m.other_sp );
        case 28:
            /* PRIMASK */
            return gdb_get_reg8(mem_buf, (env->daif & PSTATE_I) != 0);
        case 29:
            /* CONTROL */
            return gdb_get_reg8(mem_buf, env->v7m.control);
        case 30:
            /* BASEPRI */
            return gdb_get_reg8(mem_buf, env->v7m.basepri);
        case 31:
            /* FAULTMASK */
            return gdb_get_reg8(mem_buf, (env->daif & PSTATE_F) != 0);
        }
    }
    /* Unknown register.  */
    return 0;
}

int arm_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    ARMCPU *cpu = ARM_CPU(cs);
    CPUARMState *env = &cpu->env;
    uint32_t tmp;

    tmp = ldl_p(mem_buf);

    /* Mask out low bit of PC to workaround gdb bugs.  This will probably
       cause problems if we ever implement the Jazelle DBX extensions.  */
    if (n == 15) {
        tmp &= ~1;
    }

    if (n < 16) {
        /* Core integer register.  */
        env->regs[n] = tmp;
        return 4;
    }
    if (n < 24) { /* 16-23 */
        /* FPA registers (ignored).  */
        if (gdb_has_xml) {
            return 0;
        }
        return 12;
    }
    switch (n) {
    case 24:
        /* FPA status register (ignored).  */
        if (gdb_has_xml) {
            return 0;
        }
        return 4;
    case 25:
        if (IS_M(env)) {
            /* xPSR */
            xpsr_write(env, tmp, 0xffffffff);
            return 4;
        }
        /* CPSR */
        cpsr_write(env, tmp, 0xffffffff);
        return 4;
    }
    if (IS_M(env)) {
        switch( n ) {
        case 26:
            /* MSP */
            if ( !!(env->v7m.control & 2) ) {
                env->v7m.other_sp = tmp;
            } else {
                env->regs[13] = tmp;
            }
            return 4;
        case 27:
            /* PSP */
            if ( !!(env->v7m.control & 2) ) {
                env->regs[13] = tmp;
            } else {
                env->v7m.other_sp = tmp;
            }
            return 4;
        case 28:
            /* PRIMASK */
            if ( *mem_buf & PSTATE_I ) {
                env->daif |= PSTATE_I;
            } else {
                env->daif &= ~PSTATE_I;
            }
            return 1;
        case 29:
            /* CONTROL */
            env->v7m.control = *mem_buf & 0x7;
            return 1;
        case 30:
            /* BASEPRI */
            env->v7m.basepri = *mem_buf;
            return 1;
        case 31:
            /* FAULTMASK */
            if ( *mem_buf & PSTATE_F ) {
                env->daif |= PSTATE_F;
            } else {
                env->daif &= ~PSTATE_F;
            }
            return 1;
        }
    }
    /* Unknown register.  */
    return 0;
}
