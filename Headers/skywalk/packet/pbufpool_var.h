/*
 * Copyright (c) 2016-2022 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef _SKYWALK_PACKET_PBUFPOOLVAR_H_
#define _SKYWALK_PACKET_PBUFPOOLVAR_H_

#include <Availability.h>

#ifndef __MAC_OS_X_VERSION_MIN_REQUIRED
#error "Missing macOS target version"
#endif

#ifdef BSD_KERNEL_PRIVATE
#include <skywalk/core/skywalk_var.h>

struct __kern_quantum;
struct __kern_packet;

/*
 * User packet pool hash bucket.  Packets allocated by user space are
 * kept in the hash table.  This allows the kernel to validate whether
 * or not a given packet object is valid or is already-freed, and thus
 * take the appropriate measure during internalize.
 */
struct kern_pbufpool_u_bkt {
	SLIST_HEAD(, __kern_quantum) upp_head;
};

struct kern_pbufpool_u_bft_bkt {
	SLIST_HEAD(, __kern_buflet_ext) upp_head;
};

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_13_0
#define PBUFPOOL_MAX_BUF_REGIONS    2
#define PBUFPOOL_BUF_IDX_DEF        0
#define PBUFPOOL_BUF_IDX_LARGE      1
#endif

struct kern_pbufpool {
	decl_lck_mtx_data(, pp_lock);
	uint32_t                pp_refcnt;
	uint32_t                pp_flags;
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_13_0
	uint32_t                pp_buf_obj_size[PBUFPOOL_MAX_BUF_REGIONS];
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_14_0
	uint32_t                pp_buf_size[PBUFPOOL_MAX_BUF_REGIONS];
#else
	uint16_t                pp_buf_size[PBUFPOOL_MAX_BUF_REGIONS];
#endif
#else
	uint16_t                pp_buflet_size;
#endif
	uint16_t                pp_max_frags;

	/*
	 * Caches
	 */
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_13_0
	struct skmem_cache      *pp_buf_cache[PBUFPOOL_MAX_BUF_REGIONS];
#else
	struct skmem_cache      *pp_buf_cache;
#endif
	struct skmem_cache      *pp_kmd_cache;
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_13_0
	struct skmem_cache      *pp_kbft_cache[PBUFPOOL_MAX_BUF_REGIONS];
#if __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_14_0
	struct skmem_cache      *pp_raw_kbft_cache;
#endif
#else
	struct skmem_cache      *pp_kbft_cache;
#endif

	/*
	 * Regions
	 */
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_13_0
	struct skmem_region     *pp_buf_region[PBUFPOOL_MAX_BUF_REGIONS];
#endif
	struct skmem_region     *pp_buf_region;
#else
	struct skmem_region     *pp_kmd_region;
	struct skmem_region     *pp_umd_region;
	struct skmem_region     *pp_ubft_region;
	struct skmem_region     *pp_kbft_region;

	/*
	 * User packet pool: packet metadata hash table
	 */
	struct kern_pbufpool_u_bkt *pp_u_hash_table;
	uint64_t                pp_u_bufinuse;

	/*
	 * User packet pool: buflet hash table
	 */
	struct kern_pbufpool_u_bft_bkt *pp_u_bft_hash_table;
	uint64_t                pp_u_bftinuse;

	void                    *pp_ctx;
	pbuf_ctx_retain_fn_t    pp_ctx_retain;
	pbuf_ctx_release_fn_t   pp_ctx_release;
	nexus_meta_type_t       pp_md_type;
	nexus_meta_subtype_t    pp_md_subtype;
	uint32_t                pp_midx_start;
	uint32_t                pp_bidx_start;
	pbufpool_name_t         pp_name;
	pbuf_seg_ctor_fn_t      pp_pbuf_seg_ctor;
	pbuf_seg_dtor_fn_t      pp_pbuf_seg_dtor;
};

/* valid values for pp_flags */
#define PPF_EXTERNAL            0x1     /* externally configured */
#define PPF_CLOSED              0x2     /* closed; awaiting final destruction */
#define PPF_MONOLITHIC          0x4     /* non slab-based buffer region */
/* buflet is truncated and may not contain the full payload */
#define PPF_TRUNCATED_BUF       0x8
#define PPF_KERNEL              0x10    /* kernel only, no user region(s) */
#define PPF_BUFFER_ON_DEMAND    0x20    /* attach buffers to packet on demand */
#define PPF_BATCH               0x40    /* capable of batch alloc/free */
#define PPF_DYNAMIC             0x80    /* capable of magazine resizing */
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_13_0
#define PPF_LARGE_BUF           0x100   /* configured with large buffers */
#if __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_14_0
#define PPF_RAW_BUFLT           0x200   /* configured with raw buflet */
#endif
#endif

#define PP_KERNEL_ONLY(_pp)             \
	(((_pp)->pp_flags & PPF_KERNEL) != 0)

#define PP_HAS_TRUNCATED_BUF(_pp)               \
	(((_pp)->pp_flags & PPF_TRUNCATED_BUF) != 0)

#define PP_HAS_BUFFER_ON_DEMAND(_pp)            \
	(((_pp)->pp_flags & PPF_BUFFER_ON_DEMAND) != 0)

#define PP_BATCH_CAPABLE(_pp)           \
	(((_pp)->pp_flags & PPF_BATCH) != 0)

#define PP_DYNAMIC(_pp)                 \
	(((_pp)->pp_flags & PPF_DYNAMIC) != 0)

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_13_0
#define PP_HAS_LARGE_BUF(_pp)                 \
	(((_pp)->pp_flags & PPF_LARGE_BUF) != 0)

#if __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_14_0
#define PP_HAS_RAW_BFLT(_pp)                 \
	(((_pp)->pp_flags & PPF_RAW_BUFLT) != 0)
#endif
#endif

#define PP_LOCK(_pp)                    \
	lck_mtx_lock(&_pp->pp_lock)
#define PP_LOCK_ASSERT_HELD(_pp)        \
	LCK_MTX_ASSERT(&_pp->pp_lock, LCK_MTX_ASSERT_OWNED)
#define PP_LOCK_ASSERT_NOTHELD(_pp)     \
	LCK_MTX_ASSERT(&_pp->pp_lock, LCK_MTX_ASSERT_NOTOWNED)
#define PP_UNLOCK(_pp)                  \
	lck_mtx_unlock(&_pp->pp_lock)

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_13_0
#define PP_BUF_SIZE_DEF(_pp)      ((_pp)->pp_buf_size[PBUFPOOL_BUF_IDX_DEF])
#define PP_BUF_SIZE_LARGE(_pp)    ((_pp)->pp_buf_size[PBUFPOOL_BUF_IDX_LARGE])

#define PP_BUF_OBJ_SIZE_DEF(_pp)    \
	((_pp)->pp_buf_obj_size[PBUFPOOL_BUF_IDX_DEF])
#define PP_BUF_OBJ_SIZE_LARGE(_pp)    \
	((_pp)->pp_buf_obj_size[PBUFPOOL_BUF_IDX_LARGE])

#define PP_BUF_REGION_DEF(_pp)    ((_pp)->pp_buf_region[PBUFPOOL_BUF_IDX_DEF])
#define PP_BUF_REGION_LARGE(_pp)  ((_pp)->pp_buf_region[PBUFPOOL_BUF_IDX_LARGE])

#define PP_BUF_CACHE_DEF(_pp)    ((_pp)->pp_buf_cache[PBUFPOOL_BUF_IDX_DEF])
#define PP_BUF_CACHE_LARGE(_pp)  ((_pp)->pp_buf_cache[PBUFPOOL_BUF_IDX_LARGE])

#define PP_KBFT_CACHE_DEF(_pp)    ((_pp)->pp_kbft_cache[PBUFPOOL_BUF_IDX_DEF])
#define PP_KBFT_CACHE_LARGE(_pp)  ((_pp)->pp_kbft_cache[PBUFPOOL_BUF_IDX_LARGE])
#endif

__BEGIN_DECLS
extern int pp_init(void);
extern void pp_fini(void);
extern void pp_close(struct kern_pbufpool *);

/* create flags for pp_create() */
#define PPCREATEF_EXTERNAL      0x1     /* externally requested */
#define PPCREATEF_KERNEL_ONLY   0x2     /* kernel-only */
#define PPCREATEF_TRUNCATED_BUF 0x4     /* compat-only (buf is short) */
#define PPCREATEF_ONDEMAND_BUF  0x8     /* buf alloc/free is decoupled */
#define PPCREATEF_DYNAMIC       0x10    /* dynamic per-CPU magazines */
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_13_0 && __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_14_0
#define PPCREATEF_RAW_BFLT      0x20    /* buflet can be alloced w/o buf */
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_13_0
extern struct kern_pbufpool *pp_create(const char *name,
    struct skmem_region_params *srp_array, pbuf_seg_ctor_fn_t buf_seg_ctor,
    pbuf_seg_dtor_fn_t buf_seg_dtor, const void *ctx,
    pbuf_ctx_retain_fn_t ctx_retain, pbuf_ctx_release_fn_t ctx_release,
    uint32_t ppcreatef);
#else
extern struct kern_pbufpool *pp_create(const char *name,
    struct skmem_region_params *buf_srp, struct skmem_region_params *kmd_srp,
    struct skmem_region_params *kbft_srp, struct skmem_region_params *ubft_srp,
    struct skmem_region_params *umd_srp, pbuf_seg_ctor_fn_t buf_seg_ctor,
    pbuf_seg_dtor_fn_t buf_seg_dtor, const void *ctx,
    pbuf_ctx_retain_fn_t ctx_retain, pbuf_ctx_release_fn_t ctx_release,
    uint32_t ppcreatef);
#endif
extern void pp_destroy(struct kern_pbufpool *);

extern int pp_init_upp(struct kern_pbufpool *, boolean_t);
extern void pp_insert_upp(struct kern_pbufpool *, struct __kern_quantum *,
    pid_t);
extern void pp_insert_upp_locked(struct kern_pbufpool *,
    struct __kern_quantum *, pid_t);
extern void pp_insert_upp_batch(struct kern_pbufpool *pp, pid_t pid,
    uint64_t *array, uint32_t num);
extern struct __kern_quantum *pp_remove_upp(struct kern_pbufpool *, obj_idx_t,
    int *);
extern struct __kern_quantum *pp_remove_upp_locked(struct kern_pbufpool *,
    obj_idx_t, int *);
extern struct __kern_quantum *pp_find_upp(struct kern_pbufpool *, obj_idx_t);
extern void pp_purge_upp(struct kern_pbufpool *, pid_t);
extern struct __kern_buflet *pp_remove_upp_bft(struct kern_pbufpool *,
    obj_idx_t, int *);
extern void pp_insert_upp_bft(struct kern_pbufpool *, struct __kern_buflet *,
    pid_t);
extern boolean_t pp_isempty_upp(struct kern_pbufpool *);

extern void pp_retain_locked(struct kern_pbufpool *);
extern void pp_retain(struct kern_pbufpool *);
extern boolean_t pp_release_locked(struct kern_pbufpool *);
extern boolean_t pp_release(struct kern_pbufpool *);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_13_0
/* flags for pp_regions_params_adjust() */
/* configure packet pool regions for RX only */
#define PP_REGION_CONFIG_BUF_IODIR_IN          0x00000001
/* configure packet pool regions for TX only */
#define PP_REGION_CONFIG_BUF_IODIR_OUT         0x00000002
/* configure packet pool regions for bidirectional operation */
#define PP_REGION_CONFIG_BUF_IODIR_BIDIR    \
    (PP_REGION_CONFIG_BUF_IODIR_IN | PP_REGION_CONFIG_BUF_IODIR_OUT)
/* configure packet pool metadata regions as persistent (wired) */
#define PP_REGION_CONFIG_MD_PERSISTENT         0x00000004
/* configure packet pool buffer regions as persistent (wired) */
#define PP_REGION_CONFIG_BUF_PERSISTENT        0x00000008
/* Enable magazine layer (per-cpu caches) for packet pool metadata regions */
#define PP_REGION_CONFIG_MD_MAGAZINE_ENABLE    0x00000010
/* configure packet pool regions required for kernel-only operations */
#define PP_REGION_CONFIG_KERNEL_ONLY           0x00000020
/* configure packet pool buflet regions */
#define PP_REGION_CONFIG_BUFLET                0x00000040
/* configure packet pool buffer region as user read-only */
#define PP_REGION_CONFIG_BUF_UREADONLY         0x00000080
/* configure packet pool buffer region as kernel read-only */
#define PP_REGION_CONFIG_BUF_KREADONLY         0x00000100
/* configure packet pool buffer region as a single segment */
#define PP_REGION_CONFIG_BUF_MONOLITHIC        0x00000200
/* configure packet pool buffer region as physically contiguous segment */
#define PP_REGION_CONFIG_BUF_SEGPHYSCONTIG     0x00000400
/* configure packet pool buffer region as cache-inhibiting */
#define PP_REGION_CONFIG_BUF_NOCACHE           0x00000800
#if __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_14_0
/* configure buflet without buffer attached at construction */
#define PP_REGION_CONFIG_RAW_BUFLET            0x00001000
#endif
/* configure packet pool buffer region (backing IOMD) as thread safe */
#define PP_REGION_CONFIG_BUF_THREADSAFE        0x00002000

extern void pp_regions_params_adjust(struct skmem_region_params *,
    nexus_meta_type_t, nexus_meta_subtype_t, uint32_t, uint16_t,
    uint32_t, uint32_t);

#else

extern void pp_regions_params_adjust(struct skmem_region_params *,
	struct skmem_region_params *, struct skmem_region_params *,
	struct skmem_region_params *, struct skmem_region_params *,
    nexus_meta_type_t, nexus_meta_subtype_t, uint32_t, uint16_t,
    uint32_t, uint32_t);
#endif

extern uint64_t pp_alloc_packet(struct kern_pbufpool *, uint16_t, uint32_t);
extern uint64_t pp_alloc_packet_by_size(struct kern_pbufpool *, uint32_t,
    uint32_t);
extern int pp_alloc_packet_batch(struct kern_pbufpool *, uint16_t, uint64_t *,
    uint32_t *, boolean_t, alloc_cb_func_t, const void *, uint32_t);
extern int pp_alloc_pktq(struct kern_pbufpool *, uint16_t, struct pktq *,
    uint32_t, alloc_cb_func_t, const void *, uint32_t);
extern void pp_free_packet(struct kern_pbufpool *, uint64_t);
extern void pp_free_packet_batch(struct kern_pbufpool *, uint64_t *, uint32_t);
extern void pp_free_packet_single(struct __kern_packet *);
extern void pp_free_packet_chain(struct __kern_packet *, int *);
extern void pp_free_pktq(struct pktq *);
extern errno_t pp_alloc_buffer(const kern_pbufpool_t, mach_vm_address_t *,
    kern_segment_t *, kern_obj_idx_seg_t *, uint32_t);
extern void pp_free_buffer(const kern_pbufpool_t, mach_vm_address_t);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_14_0

extern errno_t pp_alloc_buflet(struct kern_pbufpool *pp, kern_buflet_t *kbft,
    uint32_t skmflag, bool large);
extern errno_t pp_alloc_buflet_batch(struct kern_pbufpool *pp, uint64_t *array,
    uint32_t *size, uint32_t skmflag, bool large);

#elif __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_13_0
/* flags for pp_alloc_buflet */
/* alloc a buflet with an attached large-sized buffer */
#define PP_ALLOC_BFT_LARGE                        0x01
/* alloc a buflet with an attached buffer */
#define PP_ALLOC_BFT_ATTACH_BUFFER                0x02

extern errno_t pp_alloc_buflet(struct kern_pbufpool *pp, kern_buflet_t *kbft,
    uint32_t skmflag, uint32_t flags);
extern errno_t pp_alloc_buflet_batch(struct kern_pbufpool *pp, uint64_t *array,
    uint32_t *size, uint32_t skmflag, uint32_t flags);
#else
extern errno_t pp_alloc_buflet(struct kern_pbufpool *pp, kern_buflet_t *kbft,
    uint32_t skmflag);
extern errno_t pp_alloc_buflet_batch(struct kern_pbufpool *pp, uint64_t *array,
    uint32_t *size, uint32_t skmflag);
#endif

extern void pp_free_buflet(const kern_pbufpool_t, kern_buflet_t);

extern void pp_reap_caches(boolean_t);
__END_DECLS
#endif /* BSD_KERNEL_PRIVATE */
#endif /* !_SKYWALK_PACKET_PBUFPOOLVAR_H_ */
