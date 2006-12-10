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

/*
 * implements sysexxxx built-in functions
 * available through the interpreter
 *
 * each function is described in the manual.html file
 */

#include "dbg.h"
#include "default.h"
#include "node.h"
#include "exec.h"
#include "data.h"
#include "cons.h"

#include "song.h"
#include "user.h"
#include "saveload.h"
#include "textio.h"



unsigned
user_func_sysexlist(struct exec *o, struct data **r) {
	struct data *d, *n;
	struct songsx *i;

	d = data_newlist(NULL);
	for (i = user_song->sxlist; i != NULL; i = (struct songsx *)i->name.next) {
		n = data_newref(i->name.str);
		data_listadd(d, n);
	}
	*r = d;
	return 1;
}

unsigned
user_func_sysexnew(struct exec *o, struct data **r) {
	char *name;
	struct songsx *i;
	
	if (!exec_lookupname(o, "sysexname", &name)) {
		return 0;
	}
	i = song_sxlookup(user_song, name);
	if (i != NULL) {
		cons_err("sysexnew: sysex already exists");
		return 0;
	}
	i = songsx_new(name);
	song_sxadd(user_song, i);
	return 1;
}

unsigned
user_func_sysexdelete(struct exec *o, struct data **r) {
	struct songsx *c;
	if (!exec_lookupsx(o, "sysexname", &c)) {
		return 0;
	}
	if (!song_sxrm(user_song, c)) {
		return 0;
	}
	songsx_delete(c);
	return 1;
}

unsigned
user_func_sysexrename(struct exec *o, struct data **r) {
	struct songsx *c;
	char *name;
	
	if (!exec_lookupsx(o, "sysexname", &c) ||
	    !exec_lookupname(o, "newname", &name)) {
		return 0;
	}
	if (song_sxlookup(user_song, name)) {
		cons_err("name already used by another sysex");
		return 0;
	}
	str_delete(c->name.str);
	c->name.str = str_new(name);
	return 1;
}

unsigned
user_func_sysexexists(struct exec *o, struct data **r) {
	char *name;
	struct songsx *i;
	if (!exec_lookupname(o, "sysexname", &name)) {
		return 0;
	}
	i = song_sxlookup(user_song, name);
	*r = data_newlong(i != NULL ? 1 : 0);
	return 1;
}

unsigned
user_func_sysexinfo(struct exec *o, struct data **r) {
	struct songsx *c;
	struct sysex *e;
	unsigned i;
	
	if (!exec_lookupsx(o, "sysexname", &c)) {
		return 0;
	}

	textout_putstr(tout, "{\n");
	textout_shiftright(tout);

	for (e = c->sx.first; e != NULL; e = e->next) {
		textout_indent(tout);
		textout_putlong(tout, e->unit);
		textout_putstr(tout, " { ");
		if (e->first) {
			for (i = 0; i < e->first->used; i++) {
				if (i > 10) {
					textout_putstr(tout, "... ");
					break;
				}
				textout_putbyte(tout, e->first->data[i]);
				textout_putstr(tout, " ");
			}
		}
		textout_putstr(tout, "}\n");
	}
	textout_shiftleft(tout);
	textout_indent(tout);
	textout_putstr(tout, "}\n");
	return 1;
}


unsigned
user_func_sysexclear(struct exec *o, struct data **r) {
	struct songsx *c;
	struct sysex *x, **px;
	struct data *d;
	unsigned match;
	
	if (!exec_lookupsx(o, "sysexname", &c) ||
	    !exec_lookuplist(o, "data", &d)) {
		return 0;
	}
	px = &c->sx.first;
	for (;;) {
		if (!*px) {
			break;
		}
		if (!data_matchsysex(d, *px, &match)) {
			return 0;
		}
		if (match) {
			x = *px;
			*px = x->next;
			if (*px == NULL) {
				c->sx.lastptr = px;
			}
			sysex_del(x);
		} else {
			px = &(*px)->next;
		}
	}
	return 1;
}

unsigned
user_func_sysexsetunit(struct exec *o, struct data **r) {
	struct songsx *c;
	struct sysex *x;
	struct data *d;
	unsigned match;
	long unit;
	
	if (!exec_lookupsx(o, "sysexname", &c) ||
	    !exec_lookuplong(o, "unit", &unit) ||
	    !exec_lookuplist(o, "data", &d)) {
		return 0;
	}
	if (unit < 0 || unit >= DEFAULT_MAXNDEVS) {
		cons_err("sysexsetunit: unit out of range");
		return 0;
	}
	for (x = c->sx.first; x != NULL; x = x->next) {
		if (!x) {
			break;
		}
		if (!data_matchsysex(d, x, &match)) {
			return 0;
		}
		if (match) {
			x->unit = unit;
		}
	}
	return 1;
}

unsigned
user_func_sysexadd(struct exec *o, struct data **r) {
	struct songsx *c;
	struct sysex *x;
	struct data *byte;
	struct var *arg;
	long unit;
	
	if (!exec_lookupsx(o, "sysexname", &c) || 
	    !exec_lookuplong(o, "unit", &unit)) {
		return 0;
	}
	if (unit < 0 || unit >= DEFAULT_MAXNDEVS) {
		cons_err("sysexadd: unit out of range");
		return 0;
	}
	arg = exec_varlookup(o, "data");
	if (!arg) {
		dbg_puts("exec_lookupev: no such var\n");
		dbg_panic();
	}
	if (arg->data->type != DATA_LIST) {
		cons_err("sysexadd: data must be a list of numbers");
		return 0;
	}
	x = sysex_new(unit);
	for (byte = arg->data->val.list; byte != 0; byte = byte->next) {
		if (byte->type != DATA_LONG) {
			cons_err("sysexadd: only bytes allowed as data");
			sysex_del(x);
			return 0;
		}
		if (byte->val.num < 0 || byte->val.num > 0xff) {
			cons_err("sysexadd: data out of range");
			sysex_del(x);
			return 0;
		}
		sysex_add(x, byte->val.num);
	}
	if (!sysex_check(x)) {
		cons_err("sysexadd: bad sysex format");
		sysex_del(x);
		return 0;
	}
	if (x->first) {
		sysexlist_put(&c->sx, x);
	} else {
		sysex_del(x);
	}	
	return 1;
}

