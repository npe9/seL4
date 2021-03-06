/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#ifndef __ARCH_FASTPATH_H
#define __ARCH_FASTPATH_H

#include <util.h>
#include <arch/linker.h>
#include <api/types.h>
#include <api/syscall.h>

/* When building the fastpath the assembler in traps.S makes these
 * assumptions. Because compile_asserts are hard to do in assembler,
 * we place them here */
compile_assert(SysCall_Minus1, SysCall == -1)
compile_assert(SysReplyWait_Minus2, SysReplyWait == -2)

/** DONT_TRANSLATE */
static inline void
clearExMonitor_fp(void)
{
    uint32_t temp1 = 0;
    uint32_t temp2;
    asm volatile (
        "strex %[output], %[mem], [%[mem]]"
        : [output]"+r"(temp1)
        : [mem]"r"(&temp2)
    );
}

static inline void FORCE_INLINE
switchToThread_fp(tcb_t *thread, pde_t *cap_pd, pde_t stored_hw_asid)
{
    hw_asid_t hw_asid;

    hw_asid = pde_pde_invalid_get_stored_hw_asid(stored_hw_asid);

    armv_contextSwitch_HWASID(cap_pd, hw_asid);

    *armKSGlobalsFrame = thread->tcbIPCBuffer;
    ksCurThread = thread;
    clearExMonitor_fp();
}

static inline void
thread_state_ptr_set_blockingIPCDiminish_np(thread_state_t *ts_ptr, word_t dim)
{
    ts_ptr->words[2] = dim;
}

static inline void
mdb_node_ptr_mset_mdbNext_mdbRevocable_mdbFirstBadged(
    mdb_node_t *node_ptr, word_t mdbNext,
    word_t mdbRevocable, word_t mdbFirstBadged)
{
    node_ptr->words[1] = mdbNext | (mdbRevocable << 1) | mdbFirstBadged;
}

static inline void
mdb_node_ptr_set_mdbPrev_np(mdb_node_t *node_ptr, word_t mdbPrev)
{
    node_ptr->words[0] = mdbPrev;
}

static inline bool_t
isValidVTableRoot_fp(cap_t pd_cap)
{
    return (pd_cap.words[0] & MASK(5)) ==
           (BIT(4) | cap_page_directory_cap);
}

/* This is an accelerated check that msgLength, which appears
   in the bottom of the msgInfo word, is <= 4 and that msgExtraCaps
   which appears above it is zero. We are assuming that n_msgRegisters == 4
   for this check to be useful. By masking out the bottom 3 bits, we are
   really checking that n + 3 <= MASK(3), i.e. n + 3 <= 7 or n <= 4. */
compile_assert (n_msgRegisters_eq_4, n_msgRegisters == 4)
static inline int
fastpath_mi_check(word_t msgInfo)
{
    return ((msgInfo & MASK(seL4_MsgLengthBits + seL4_MsgExtraCapBits))
            + 3) & ~MASK(3);
}

static inline void
fastpath_copy_mrs(unsigned int length, tcb_t *src, tcb_t *dest)
{
    unsigned int i;
    register_t reg;

    /* assuming that length < n_msgRegisters */
    for (i = 0; i < length; i ++) {
        /* assuming that the message registers simply increment */
        reg = msgRegisters[0] + i;
        setRegister(dest, reg, getRegister(src, reg));
    }
}

static inline int
fastpath_reply_cap_check(cap_t cap)
{
    return (cap.words[0] & MASK(5)) == cap_reply_cap;
}

void slowpath(syscall_t syscall)
VISIBLE NORETURN;

void fastpath_restore(word_t badge, word_t msgInfo, tcb_t *cur_thread)
VISIBLE NORETURN;

void fastpath_call(word_t cptr, word_t r_msgInfo)
VISIBLE NORETURN SECTION(".vectors.fastpath_call");

void fastpath_reply_wait(word_t cptr, word_t r_msgInfo)
VISIBLE NORETURN SECTION(".vectors.fastpath_reply_wait");

#endif

