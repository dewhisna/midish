/*
 * Copyright (c) 2003-2007 Alexandre Ratchov <alex@caoua.org>
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
 * implements songxxx built-in functions
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

#include "frame.h"
#include "song.h"
#include "user.h"
#include "smf.h"
#include "saveload.h"
#include "textio.h"



unsigned
user_func_songsetcurchan(struct exec *o, struct data **r) {
	struct songchan *t;
	struct var *arg;
	
	arg = exec_varlookup(o, "channame");
	if (!arg) {
		dbg_puts("user_func_songsetcurchan: 'channame': no such param\n");
		return 0;
	}
	if (arg->data->type == DATA_NIL) {
		song_setcurchan(user_song, NULL);
		return 1;
	} 
	if (!exec_lookupchan_getref(o, "channame", &t)) {
		return 0;
	}
	song_setcurchan(user_song, t);
	return 1;
}

unsigned
user_func_songgetcurchan(struct exec *o, struct data **r) {
	struct songchan *cur;
	
	song_getcurchan(user_song, &cur);
	if (cur) {
		*r = data_newref(cur->name.str);
	} else {
		*r = data_newnil();
	}
	return 1;
}

unsigned
user_func_songsetcursysex(struct exec *o, struct data **r) {
	struct songsx *t;
	struct var *arg;
	
	arg = exec_varlookup(o, "sysexname");
	if (!arg) {
		dbg_puts("user_func_songsetcursysex: 'sysexname': no such param\n");
		return 0;
	}
	if (arg->data->type == DATA_NIL) {
		song_setcursx(user_song, NULL);
		return 1;
	} 
	if (!exec_lookupsx(o, "sysexname", &t)) {
		return 0;
	}
	song_setcursx(user_song, t);
	return 1;
}

unsigned
user_func_songgetcursysex(struct exec *o, struct data **r) {
	struct songsx *cur;
	
	song_getcursx(user_song, &cur);
	if (cur) {
		*r = data_newref(cur->name.str);
	} else {
		*r = data_newnil();
	}
	return 1;
}

unsigned
user_func_songsetunit(struct exec *o, struct data **r) {	/* tics per unit note */
	long tpu;
	if (!exec_lookuplong(o, "tics_per_unit", &tpu)) {
		return 0;
	}
	if ((tpu % DEFAULT_TPU) != 0 || tpu < DEFAULT_TPU) {
		cons_err("songsetunit: unit must be multiple of 96 tics");
		return 0;
	}
	/* XXX: should check that all tracks are empty (tempo track included) */
	if (user_song->trklist) {
		cons_err("WARNING: unit must be changed before any tracks are created");
	}
	user_song->tics_per_unit = tpu;
	return 1;
}

unsigned
user_func_songgetunit(struct exec *o, struct data **r) {	/* tics per unit note */
	*r = data_newlong(user_song->tics_per_unit);
	return 1;
}

unsigned
user_func_songsetcurpos(struct exec *o, struct data **r) {
	long measure;
	
	if (!exec_lookuplong(o, "measure", &measure)) {
		return 0;
	}
	if (measure < 0) {
		cons_err("measure cant be negative");
		return 0;
	}
	user_song->curpos = measure;
	return 1;
}

unsigned
user_func_songgetcurpos(struct exec *o, struct data **r) {
	*r = data_newlong(user_song->curpos);
	return 1;
}

unsigned
user_func_songsetcurlen(struct exec *o, struct data **r) {
	long len;
	
	if (!exec_lookuplong(o, "length", &len)) {
		return 0;
	}
	if (len < 0) {
		cons_err("'measures' parameter cant be negative");
		return 0;
	}
	user_song->curlen = len;
	return 1;
}

unsigned
user_func_songgetcurlen(struct exec *o, struct data **r) {
	*r = data_newlong(user_song->curlen);
	return 1;
}

unsigned
user_func_songsetcurquant(struct exec *o, struct data **r) {
	long quantum;
	
	if (!exec_lookuplong(o, "quantum", &quantum)) {
		return 0;
	}
	if (quantum < 0 || (unsigned)quantum > user_song->tics_per_unit) {
		cons_err("quantum must be between 0 and tics_per_unit");
		return 0;
	}
	user_song->curquant = quantum;
	return 1;
}

unsigned
user_func_songgetcurquant(struct exec *o, struct data **r) {
	*r = data_newlong(user_song->curquant);
	return 1;
}

unsigned
user_func_songsetcurtrack(struct exec *o, struct data **r) {
	struct songtrk *t;
	struct var *arg;
	
	arg = exec_varlookup(o, "trackname");
	if (!arg) {
		dbg_puts("user_func_songsetcurtrack: 'trackname': no such param\n");
		return 0;
	}
	if (arg->data->type == DATA_NIL) {
		song_setcurtrk(user_song, NULL);
		return 1;
	} 
	if (!exec_lookuptrack(o, "trackname", &t)) {
		return 0;
	}
	song_setcurtrk(user_song, t);
	return 1;
}

unsigned
user_func_songgetcurtrack(struct exec *o, struct data **r) {
	struct songtrk *cur;
	
	song_getcurtrk(user_song, &cur);
	if (cur) {
		*r = data_newref(cur->name.str);
	} else {
		*r = data_newnil();
	}
	return 1;
}

unsigned
user_func_songsetcurfilt(struct exec *o, struct data **r) {
	struct songfilt *f;
	struct var *arg;
	
	arg = exec_varlookup(o, "filtname");
	if (!arg) {
		dbg_puts("user_func_songsetcurfilt: 'filtname': no such param\n");
		return 0;
	}
	if (arg->data->type == DATA_NIL) {
		song_setcurfilt(user_song, NULL);
		return 1;
	} else if (arg->data->type == DATA_REF) {
		f = song_filtlookup(user_song, arg->data->val.ref);
		if (!f) {
			cons_err("no such filt");
			return 0;
		}
		song_setcurfilt(user_song, f);
		return 1;
	}
	return 0;
}

unsigned
user_func_songgetcurfilt(struct exec *o, struct data **r) {
	struct songfilt *cur;
	
	song_getcurfilt(user_song, &cur);
	if (cur) {
		*r = data_newref(cur->name.str);
	} else {
		*r = data_newnil();
	}	
	return 1;
}

unsigned
user_func_songinfo(struct exec *o, struct data **r) {
	char map[DEFAULT_MAXNCHANS];
	struct songtrk *t;
	struct songchan *c;
	struct songfilt *f;
	struct songsx *s;
	struct sysex *x;
	unsigned i, count;
	unsigned dev, ch;
	
	/* print info about channels */	

	textout_putstr(tout, "chanlist {\n");
	textout_shiftright(tout);
	textout_indent(tout);
	textout_putstr(tout, "# chan_name,  {devicenum, midichan}, default_input\n");
	SONG_FOREACH_CHAN(user_song, c) {
		textout_indent(tout);
		textout_putstr(tout, c->name.str);
		textout_putstr(tout, "\t");
		textout_putstr(tout, "{");
		textout_putlong(tout, c->dev);
		textout_putstr(tout, " ");
		textout_putlong(tout, c->ch);
		textout_putstr(tout, "}");
		textout_putstr(tout, "\t");
		textout_putstr(tout, "{");
		textout_putlong(tout, c->curinput_dev);
		textout_putstr(tout, " ");
		textout_putlong(tout, c->curinput_ch);
		textout_putstr(tout, "}");
		textout_putstr(tout, "\n");
		
	}	
	textout_shiftleft(tout);
	textout_putstr(tout, "}\n");

	/* print info about filters */	

	textout_putstr(tout, "filtlist {\n");
	textout_shiftright(tout);
	textout_indent(tout);
	textout_putstr(tout, "# filter_name,  default_channel\n");
	SONG_FOREACH_FILT(user_song, f) {
		textout_indent(tout);
		textout_putstr(tout, f->name.str);
		textout_putstr(tout, "\t");
		if (f->curchan != NULL) {
			textout_putstr(tout, f->curchan->name.str);
		} else {
			textout_putstr(tout, "nil");
		}
		textout_putstr(tout, "\n");
		
	}
	textout_shiftleft(tout);
	textout_putstr(tout, "}\n");

	/* print info about tracks */

	textout_putstr(tout, "tracklist {\n");
	textout_shiftright(tout);
	textout_indent(tout);
	textout_putstr(tout, "# track_name,  default_filter,  used_channels,  flags\n");
	SONG_FOREACH_TRK(user_song, t) {
		textout_indent(tout);
		textout_putstr(tout, t->name.str);
		textout_putstr(tout, "\t");
		if (t->curfilt != NULL) {
			textout_putstr(tout, t->curfilt->name.str);
		} else {
			textout_putstr(tout, "nil");
		}
		textout_putstr(tout, "\t{");
		track_chanmap(&t->track, map);
		for (i = 0, count = 0; i < DEFAULT_MAXNCHANS; i++) {
			if (map[i]) {
				if (count) {
					textout_putstr(tout, " ");
				}
				c = song_chanlookup_bynum(user_song, i / 16, i % 16);
				if (c) {
					textout_putstr(tout, c->name.str);
				} else {
					textout_putstr(tout, "{");
					textout_putlong(tout, i / 16);
					textout_putstr(tout, " ");
					textout_putlong(tout, i % 16);
					textout_putstr(tout, "}");
				}
				count++;
			}
		}
		textout_putstr(tout, "}");
		if (t->mute) {
			textout_putstr(tout, " mute");
		}
		textout_putstr(tout, "\n");
		
	}	
	textout_shiftleft(tout);
	textout_putstr(tout, "}\n");

	/* print info about sysex banks */	

	textout_putstr(tout, "sysexlist {\n");
	textout_shiftright(tout);
	textout_indent(tout);
	textout_putstr(tout, "# sysex_name,  number_messages\n");
	SONG_FOREACH_SX(user_song, s) {
		textout_indent(tout);
		textout_putstr(tout, s->name.str);
		textout_putstr(tout, "\t");
		i = 0;
		for (x = s->sx.first; x != NULL; x = x->next) {
			i++;
		}
		textout_putlong(tout, i);
		textout_putstr(tout, "\n");
		
	}	
	textout_shiftleft(tout);
	textout_putstr(tout, "}\n");
	
	/* print current values */

	textout_putstr(tout, "curchan ");
	song_getcurchan(user_song, &c);
	if (c) {
		textout_putstr(tout, c->name.str);
	} else {
		textout_putstr(tout, "nil");
	}
	textout_putstr(tout, "\n");

	textout_putstr(tout, "curfilt ");
	song_getcurfilt(user_song, &f);
	if (f) {
		textout_putstr(tout, f->name.str);
	} else {
		textout_putstr(tout, "nil");
	}
	textout_putstr(tout, "\n");

	textout_putstr(tout, "curtrack ");
	song_getcurtrk(user_song, &t);
	if (t) {
		textout_putstr(tout, t->name.str);
	} else {
		textout_putstr(tout, "nil");
	}
	textout_putstr(tout, "\n");	

	textout_putstr(tout, "cursysex ");
	song_getcursx(user_song, &s);
	if (s) {
		textout_putstr(tout, s->name.str);
	} else {
		textout_putstr(tout, "nil");
	}
	textout_putstr(tout, "\n");	

	textout_putstr(tout, "curquant ");
	textout_putlong(tout, user_song->curquant);
	textout_putstr(tout, "\n");	
	textout_putstr(tout, "curpos ");
	textout_putlong(tout, user_song->curpos);
	textout_putstr(tout, "\n");	
	textout_putstr(tout, "curlen ");
	textout_putlong(tout, user_song->curlen);
	textout_putstr(tout, "\n");	

	textout_indent(tout);
	textout_putstr(tout, "curinput {");
	song_getcurinput(user_song, &dev, &ch);
	textout_putlong(tout, dev);
	textout_putstr(tout, " ");
	textout_putlong(tout, ch);
	textout_putstr(tout, "}\n");
	return 1;
}

unsigned
user_func_songsave(struct exec *o, struct data **r) {
	char *filename;	
	if (!exec_lookupstring(o, "filename", &filename)) {
		return 0;
	}	
	song_save(user_song, filename);
	return 1;
}

unsigned
user_func_songload(struct exec *o, struct data **r) {
	char *filename;		
	if (!exec_lookupstring(o, "filename", &filename)) {
		return 0;
	}
	song_done(user_song);
	song_init(user_song);
	return song_load(user_song, filename);
}

unsigned
user_func_songreset(struct exec *o, struct data **r) {
	song_done(user_song);
	song_init(user_song);
	return 1;
}

unsigned
user_func_songexportsmf(struct exec *o, struct data **r) {
	char *filename;
	if (!exec_lookupstring(o, "filename", &filename)) {
		return 0;
	}	
	return song_exportsmf(user_song, filename);
}

unsigned
user_func_songimportsmf(struct exec *o, struct data **r) {
	char *filename;
	struct song *sng;
	if (!exec_lookupstring(o, "filename", &filename)) {
		return 0;
	}
	sng = song_importsmf(filename);
	if (sng == NULL) {
		return 0;
	}
	song_delete(user_song);
	user_song = sng;
	return 1;
}

unsigned
user_func_songidle(struct exec *o, struct data **r) {
	song_idle(user_song);
	return 1;
}
		
unsigned
user_func_songplay(struct exec *o, struct data **r) {
	song_play(user_song);
	return 1;
}

unsigned
user_func_songrecord(struct exec *o, struct data **r) {
	song_record(user_song);
	return 1;
}

unsigned
user_func_songsettempo(struct exec *o, struct data **r) {
	long tempo, measure;
	
	if (!exec_lookuplong(o, "measure", &measure) ||
	    !exec_lookuplong(o, "beats_per_minute", &tempo)) {
		return 0;
	}	
	if (tempo < 40 || tempo > 240) {
		cons_err("tempo must be between 40 and 240 beats per measure");
		return 0;
	}
	track_settempo(&user_song->meta, measure, tempo);
	return 1;
}

unsigned
user_func_songtimeins(struct exec *o, struct data **r) {
	long num, den, amount, from;
	
	if (!exec_lookuplong(o, "from", &from) ||
	    !exec_lookuplong(o, "amount", &amount) ||
	    !exec_lookuplong(o, "numerator", &num) || 
	    !exec_lookuplong(o, "denominator", &den)) {
		return 0;
	}
	if (den != 1 && den != 2 && den != 4 && den != 8) {
		cons_err("only 1, 2, 4 and 8 are supported as denominator");
		return 0;
	}
	if (amount == 0) {
		return 1;
	}
	track_timeins(&user_song->meta, from, amount, 
	    num, user_song->tics_per_unit / den);
	return 1;
}

unsigned
user_func_songtimerm(struct exec *o, struct data **r) {
	long amount, from;
	if (!exec_lookuplong(o, "from", &from) ||
	    !exec_lookuplong(o, "amount", &amount)) {
		return 0;
	}
	track_timerm(&user_song->meta, from, amount);
	return 1;
}

unsigned
user_func_songtimeinfo(struct exec *o, struct data **r) {
	track_output(&user_song->meta, tout);
	textout_putstr(tout, "\n");
	return 1;
}

unsigned
user_func_songsetcurinput(struct exec *o, struct data **r) {
	unsigned dev, ch;
	struct data *l;
	
	if (!exec_lookuplist(o, "inputchan", &l)) {
		return 0;
	}
	if (!data_num2chan(l, &dev, &ch)) {
		return 0;
	}
	song_setcurinput(user_song, dev, ch);
	return 1;
}

unsigned
user_func_songgetcurinput(struct exec *o, struct data **r) {
	unsigned dev, ch;
	song_getcurinput(user_song, &dev, &ch);  
	*r = data_newlist(NULL);
	data_listadd(*r, data_newlong(dev));
	data_listadd(*r, data_newlong(ch));
	return 1;
}

unsigned
user_func_songsetfactor(struct exec *o, struct data **r) {
	long tpu;
	if (!exec_lookuplong(o, "tempo_factor", &tpu)) {
		return 0;
	}
	if (tpu < 50 || tpu > 200) {
		cons_err("songsetfactor: factor must be between 50 and 200");
		return 0;
	}
	user_song->tempo_factor = 0x100 * 100 / tpu;
	return 1;
}

unsigned
user_func_songgetfactor(struct exec *o, struct data **r) {
	*r = data_newlong(user_song->tempo_factor);
	return 1;
}

unsigned
user_func_ctlconf(struct exec *o, struct data **r) {
	char *name;
	unsigned num, old;
	long defval, bits;
	struct var *arg;
	
	if (!exec_lookupname(o, "name", &name) ||
	    !exec_lookupctl(o, "ctl", &num) ||
	    !exec_lookuplong(o, "bits", &bits)) {
		return 0;
	}

	if (bits != 7 && bits != 14) {
		cons_err("only 7bit and 14bit precision is allowed\n");
		return 0;
	}
	if (bits == 14 && num >= 32) {
		cons_err("only controllers 0..31 can be 14bit");
		return 0;
	}
	arg = exec_varlookup(o, "defval");
	if (!arg) {
		dbg_puts("user_func_ctlconf: 'defval': no such param\n");
		return 0;
	}
	if (arg->data->type == DATA_NIL) {
		defval = EV_UNDEF;
	} else if (arg->data->type == DATA_LONG) {
		defval = arg->data->val.num;
		if (defval < 0 || 
		    defval > (bits == 7 ? EV_MAXCOARSE : EV_MAXFINE)) {
			cons_err("defval out of bounds");
			return 0;
		}
	}
	if (evctl_lookup(name, &old)) {
		evctl_unconf(old);
	}
	evctl_unconf(num);
	evctl_conf(num, name, bits == 14 ? 1 : 0, defval);
	return 1;
}


unsigned
user_func_ctlunconf(struct exec *o, struct data **r) {
	char *name;
	unsigned num;
	
	if (!exec_lookupname(o, "name", &name)) {
		return 0;
	}
	if (!evctl_lookup(name, &num)) {
		cons_errs(name, "no such controller");
		return 0;
	}
	evctl_unconf(num);
	return 1;
}


unsigned
user_func_ctlinfo(struct exec *o, struct data **r) {
	unsigned i;
	struct evctl *ctl;
	
	textout_putstr(tout, "ctltab {\n");
	textout_shiftright(tout);
	textout_indent(tout);
	textout_putstr(tout, "#\n");
	textout_indent(tout);
	textout_putstr(tout, "# name\tnumber\tprec\tdefval\n");
	textout_indent(tout);
	textout_putstr(tout, "#\n");
	for (i = 0; i < 128; i++) {
		ctl = &evctl_tab[i];
		if (ctl->name) {
			textout_indent(tout);
			textout_putstr(tout, ctl->name);
			textout_putstr(tout, "\t");
			textout_putlong(tout, i);
			textout_putstr(tout, "\t");
			textout_putlong(tout, ctl->isfine ? 14 : 7);
			textout_putstr(tout, "\t");
			if (ctl->defval == EV_UNDEF) {
				textout_putstr(tout, "nil");
			} else {
				textout_putlong(tout, ctl->defval);

			}
    			textout_putstr(tout, "\n");
		}
	}
	textout_shiftleft(tout);
	textout_putstr(tout, "}\n");
	return 1;
}

