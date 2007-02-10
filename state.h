/*
 * Copyright (c) 2003-2006 Alexandre Ratchov <alex@caoua.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 *
 * 	- Redistributions of source code must retain the above
 * 	  copyright notice, this list of conditions and the
 * 	  following disclaimer.
 *
 * 	- Redistributions in binary form must reproduce the above
 * 	  copyright notice, this list of conditions and the
 * 	  following disclaimer in the documentation and/or other
 * 	  materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MIDISH_STATE_H
#define MIDISH_STATE_H

#undef STATE_DEBUG
#undef STATE_PROF

#include "ev.h"

struct seqev;

struct state  {
	struct ev ev;			/* last event */
	struct evctx ctx;		/* context for PCs and DATAENTs */
	struct state *next, **prev;	/* for statelist */
	unsigned char hi, lo;		/* 7bit nibbles of 14bit CTLs */
#define STATE_NEW	1		/* just created, never updated */
#define STATE_CHANGED	2		/* updated within the current tick */
#define STATE_BOGUS	4		/* frame detected as bogus */
#define STATE_NESTED	8		/* nested frame */
	unsigned flags;
	unsigned phase;			/* current phase (of the 'ev' field) */

	unsigned tic;			/* absolute tic of the FIRST event */
	struct seqev *pos;		/* pointer to the FIRST event */
	unsigned tag;			/* frame is selected */
	unsigned nevents;		/* number of events before timeout */
};

/*
 * return true if the state depends on any context (ex: PCs may depend
 * on bank changes, lo/hi nibbles of 14bit controllers depend on each
 * other).
 */
#define STATE_HASCTX(s) \
	((s)->ctx.ctl_hi != EV_CTL_UNKNOWN || \
	 (s)->ctx.ctl_lo != EV_CTL_UNKNOWN)

struct statelist {
	/* 
	 * instead of a simple list, we should use a hash table here,
	 * but statistics on real-life cases seem to show that lookups
	 * are very fast thanks to the state ordering (avg lookup
	 * time is around 1-2 iterations)
	 */
	struct state *first;	/* head of the state list */
	unsigned changed;	/* if changed within this tick */
#ifdef STATE_PROF
	unsigned lookup_n;	/* number of lookups */
	unsigned lookup_max;	/* max lookup time */
	unsigned lookup_time;	/* total lookup time */
#endif
};

#define STATE_REVMAX 4		/* num events state_cancel() & co return */

void	      state_pool_init(unsigned size);
void	      state_pool_done(void);
struct state *state_new(void);
void	      state_del(struct state *s);
void	      state_copyev(struct state *s, struct ev *ev, struct state *ctx);
unsigned      state_match(struct state *s, struct ev *ev, struct state *st);
unsigned      state_eq(struct state *s, struct ev *ev);
unsigned      state_cancel(struct state *st, struct ev *rev);
unsigned      state_restore(struct state *st, struct ev *rev);

void	      statelist_init(struct statelist *o);
void	      statelist_done(struct statelist *o);
void	      statelist_dump(struct statelist *o);
void	      statelist_dup(struct statelist *o, struct statelist *src);
void	      statelist_add(struct statelist *o, struct state *st);
void	      statelist_rm(struct statelist *o, struct state *st);
void	      statelist_empty(struct statelist *o);
struct state *statelist_lookup(struct statelist *o, struct ev *ev);
struct state *statelist_update(struct statelist *statelist, struct ev *ev);
void	      statelist_outdate(struct statelist *o);

#endif /* MIDISH_STATE_H */
