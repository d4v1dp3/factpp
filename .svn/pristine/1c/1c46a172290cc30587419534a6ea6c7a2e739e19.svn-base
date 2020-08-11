/* 
 * DIM Include file for changing the number of open connections
 * Date: 06-12-2007
 * Author: C. Gaspar
 */

#ifdef WIN32
#define FD_SETSIZE      16384
#else
#ifdef linux
#ifndef NOMORECONNS
/* CG: Copied here bits/typesizes.h */
#ifndef _BITS_TYPESIZES_H
#define _BITS_TYPESIZES_H   1

/* See <bits/types.h> for the meaning of these macros.  This file exists so
that <bits/types.h> need not vary across different GNU platforms.  */

#define __DEV_T_TYPE        __UQUAD_TYPE
#define __UID_T_TYPE        __U32_TYPE
#define __GID_T_TYPE        __U32_TYPE    
#define __INO_T_TYPE        __ULONGWORD_TYPE    
#define __INO64_T_TYPE      __UQUAD_TYPE
#define __MODE_T_TYPE       __U32_TYPE
#define __NLINK_T_TYPE      __UWORD_TYPE
#define __OFF_T_TYPE        __SLONGWORD_TYPE
#define __OFF64_T_TYPE      __SQUAD_TYPE
#define __PID_T_TYPE        __S32_TYPE
#define __RLIM_T_TYPE       __ULONGWORD_TYPE
#define __RLIM64_T_TYPE     __UQUAD_TYPE
#define __BLKCNT_T_TYPE     __SLONGWORD_TYPE
#define __BLKCNT64_T_TYPE   __SQUAD_TYPE
#define __FSBLKCNT_T_TYPE   __ULONGWORD_TYPE
#define __FSBLKCNT64_T_TYPE __UQUAD_TYPE
#define __FSFILCNT_T_TYPE   __ULONGWORD_TYPE
#define __FSFILCNT64_T_TYPE __UQUAD_TYPE
#define __FSWORD_T_TYPE     __SWORD_TYPE
#define __ID_T_TYPE     __U32_TYPE
#define __CLOCK_T_TYPE      __SLONGWORD_TYPE
#define __TIME_T_TYPE       __SLONGWORD_TYPE
#define __USECONDS_T_TYPE   __U32_TYPE
#define __SUSECONDS_T_TYPE  __SLONGWORD_TYPE
#define __DADDR_T_TYPE      __S32_TYPE
#define __SWBLK_T_TYPE		__SLONGWORD_TYPE
#define __KEY_T_TYPE        __S32_TYPE
#define __CLOCKID_T_TYPE    __S32_TYPE
#define __TIMER_T_TYPE      void *
#define __BLKSIZE_T_TYPE    __SLONGWORD_TYPE
#define __FSID_T_TYPE       struct { int __val[2]; }
#define __SSIZE_T_TYPE      __SWORD_TYPE
#define __SYSCALL_SLONG_TYPE    __SLONGWORD_TYPE
#define __SYSCALL_ULONG_TYPE    __ULONGWORD_TYPE

#ifdef __LP64__
/* Tell the libc code that off_t and off64_t are actually the same type
for all ABI purposes, even if possibly expressed as different base types    
for C type-checking purposes.  */
# define __OFF_T_MATCHES_OFF64_T    1

/* Same for ino_t and ino64_t.  */
# define __INO_T_MATCHES_INO64_T    1
#endif

/* Number of descriptors that can fit in an `fd_set'.  */
#define __FD_SETSIZE        16384


#endif /* bits/typesizes.h */ 

/* CG: Copied here linux/posix_types.h */
#ifndef _LINUX_POSIX_TYPES_H
#define _LINUX_POSIX_TYPES_H

#include <linux/stddef.h>

#undef __NFDBITS
#define __NFDBITS	(8 * sizeof(unsigned long))

#undef __FD_SETSIZE
#define __FD_SETSIZE	16384

#undef __FDSET_LONGS
#define __FDSET_LONGS	(__FD_SETSIZE/__NFDBITS)

#undef __FDELT
#define	__FDELT(d)	((d) / __NFDBITS)

#undef __FDMASK
#define	__FDMASK(d)	(1UL << ((d) % __NFDBITS))

typedef struct {
	unsigned long fds_bits [__FDSET_LONGS];
} __kernel_fd_set;

/* Type of a signal handler.  */
typedef void (*__kernel_sighandler_t)(int);

/* Type of a SYSV IPC key.  */
typedef int __kernel_key_t;
typedef int __kernel_mqd_t;

#include <asm/posix_types.h>

#endif /* _LINUX_POSIX_TYPES_H */

#endif /* NOMORECONNS */
#endif /* linux */

#endif
