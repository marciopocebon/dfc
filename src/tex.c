/*
 * Copyright (c) 2012, Robin Hahling
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1 Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  2 Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  4 Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * tex.c
 *
 * TeX display functions
 */
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tex.h"
#include "extern.h"

/*
 * Given a string S, returns the same string where the '_' character is replaced
 * by "\_". The returned string must be freed by the caller.
 */
static char *
sanitize_string(const char *s)
{
	unsigned nchars = 1; /* Trailing \0 */
	unsigned i;
	for (i = 0; s[i] != '\0'; i++) {
		if (s[i] == '_')
			nchars += 2;
		else
			nchars++;
	}

	if (i == nchars) /* No '_' was found. */
		return strdup(s);


	char *new = malloc(nchars);
	if (new == NULL)
		exit(EXIT_FAILURE);

	unsigned j = 0;
	for (i = 0; s[i] != '\0'; i++) {
		if (s[i] == '_') {
			new[j] = '\\';
			new[j+1] = '_';
			j += 2;
		} else {
			new[j] = s[i];
			j++;
		}
	}

	new[nchars-1] = '\0';
	return new;
}

void
init_disp_tex(struct Display *disp)
{
	disp->init         = tex_disp_init;
	disp->deinit       = tex_disp_deinit;
	disp->print_header = tex_disp_header;
	disp->print_sum    = tex_disp_sum;
	disp->print_bar    = tex_disp_bar;
	disp->print_at     = tex_disp_at;
	disp->print_fs     = tex_disp_fs;
	disp->print_type   = tex_disp_type;
	disp->print_inodes = tex_disp_inodes;
	disp->print_mount  = tex_disp_mount;
	disp->print_mopt   = tex_disp_mopt;
	disp->print_perct  = tex_disp_perct;
}

void
tex_disp_init(void)
{
	(void)puts("\\documentclass[a4]{report}");
	(void)puts("\\usepackage[landscape]{geometry}");
	(void)puts("\\begin{document}");

	unsigned ncolumns = 5;
	if (Tflag)
		ncolumns++;
	if (!bflag)
		ncolumns++;
	if (dflag)
		ncolumns++;
	if (iflag)
		ncolumns += 2;
	if (oflag)
		ncolumns++;
	(void)printf("\\begin{tabular}{");
	unsigned i;
	for (i = 0; i < ncolumns; i++)
		(void)printf("|l");
	printf("|}\n");

}

void
tex_disp_deinit(void)
{
	(void)puts("\\\\");
	(void)puts("\\hline");
	(void)puts("\\end{tabular}");
	(void)puts("\\end{document}");
}

void
tex_disp_header(struct list *lst)
{
	(void)puts("\\hline");
	(void)printf("%s", _("FILESYSTEM"));
	if (Tflag)
		(void)printf(" & %s", _("TYPE"));
	if (!bflag)
		(void)printf(" & %s", _("USAGE"));
	(void)printf(" & %s", _("\\%USED"));
	if (dflag)
		(void)printf(" & %s", _("USED"));
	(void)printf(" & %s ", _("AVAILABLE"));
	(void)printf(" & %s ", _("TOTAL"));
	if (iflag) {
		(void)printf(" & %s ", _("\\#INODES"));
		(void)printf(" & %s ", _("AV.INODES,"));
	} 
	(void)printf(" & %s ", _("MOUNTED ON"));
	if (oflag)
		(void)printf(" & %s ", _("MOUNT OPTIONS"));

	(void)puts("\\\\");
	(void)puts("\\hline");
}

void
tex_disp_sum(struct list *lst, double stot, double atot, double utot,
             double ifitot, double ifatot)
{
	(void)lst;

	double ptot = stot == 0 ? 100.0 : 100.0 * (utot / stot);
	(void)printf("\\\\ SUM");

	if (Tflag)
		(void)printf(" & N/A");

	if (!bflag)
		tex_disp_bar(ptot);

	tex_disp_perct(ptot);

	if (uflag) {
		stot = cvrt(stot);
		atot = cvrt(atot);
		if (dflag)
			utot = cvrt(utot);
	}

	if (dflag)
		tex_disp_at(utot, ptot);

	tex_disp_at(atot, ptot);
	tex_disp_at(stot, ptot);

	if (iflag) {
		(void)printf(" & %.fk", ifitot/1000.0);
		(void)printf(" & %.fk", ifatot/1000.0);
	}

	/* keep same amount of columns in table */
	(void)printf(" & NA");
	if (oflag)
		(void)printf(" & NA ");
}

void
tex_disp_bar(double perct)
{
	(void) perct;
	/* TODO */
}

void
tex_disp_at(double n, double perct)
{
	int i;

	(void)perct;

	/* XXX: This is a huge copy/paste of src/text.c. This should probably be
	* factorized in src/util.c */
	/* available  and total */
	switch (unitflag) {
	case 'h':
		i = humanize(&n);
		(void)printf(i == 0 ? " & %.f" : " & %.1f", n);
		switch (i) {
		case 0:	/* bytes */
			(void)printf("B");
			break;
		case 1: /* Kio  or Ko */
			(void)printf("K");
			break;
		case 2: /* Mio or Mo */
			(void)printf("M");
			break;
		case 3: /* Gio or Go*/
			(void)printf("G");
			break;
		case 4: /* Tio or To*/
			(void)printf("T");
			break;
		case 5: /* Pio or Po*/
			(void)printf("P");
			break;
		case 6: /* Eio or Eo*/
			(void)printf("E");
			break;
		case 7: /* Zio or Zo*/
			(void)printf("Z");
			break;
		case 8: /* Yio or Yo*/
			(void)printf("Y");
			break;
		}
		return;
	case 'b':
		(void)printf(" & %.fB", n);
		return;
	case 'k':
		(void)printf(" & %.fK", n);
		return;
	}

	(void)printf(" & %.1f", n);

	switch (unitflag) {
	case 'm':
		(void)printf("M");
		break;
	case 'g':
		(void)printf("G");
		break;
	case 't':
		(void)printf("T");
		break;
	case 'p':
		(void)printf("P");
		break;
	case 'e':
		(void)printf("E");
		break;
	case 'z':
		(void)printf("Z");
		break;
	case 'y':
		(void)printf("Y");
		break;
	}
}

void
tex_disp_fs(struct list *lst, char *fsname)
{
	(void) lst;

	static int must_close = 0;
	if (must_close == 1)
		(void) puts("\\\\");

	char *cleaned_fsname = sanitize_string(fsname);
	(void)printf("%s", cleaned_fsname);
	free(cleaned_fsname);

	must_close = 1;
}

void
tex_disp_type(struct list *lst, char *type)
{
	(void) lst;

	char *cleaned_type = sanitize_string(type);
	(void) printf(" & %s", cleaned_type);
	free(cleaned_type);
}

void
tex_disp_inodes(unsigned long files, unsigned long favail)
{
	(void) printf(" & %ldk & %ldk ", files, favail);
}

void
tex_disp_mount(char *dir)
{
	char *cleaned_dir = sanitize_string(dir);
	(void) printf(" & %s", cleaned_dir);
	free(cleaned_dir);
}

void
tex_disp_mopt(struct list *lst, char *dir, char *opts)
{
	(void) lst;
	(void) dir;

	char *cleaned_opts = sanitize_string(opts);
	(void) printf(" & %s", cleaned_opts);
	free(cleaned_opts);
}

void
tex_disp_perct(double perct)
{
	(void) printf(" & %.f\%", perct);
}
