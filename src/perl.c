/*

    EnergyMech, IRC bot software
    Copyright (c) 2001 proton
    Copyright (c) 2001 MadCamel

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
#define PERL_C
#include "config.h"

#ifdef PERL

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#if 0

PerlInterpreter *energymech_perl = NULL;

/*
 *  parse_jump() translates from C to perl
 */
int perl_parse_jump(char *from, char *rest, Hook *hook)
{
	dSP;		/* Declare a local stack pointer */
	char *args[3];

#ifdef DEBUG
	debug("(perl_parse_jump) %s %s %s\n",
		nullstr(hook->self),nullstr(from),nullstr(rest));
#endif /* DEBUG */

	args[0] = (*from && from) ? from : ""; /* Nulls are a no-no */
	args[1] = rest;
	args[2] = NULL;

	/* Call_argv returns the # of args returned from perl */
	if (call_argv(Hook->self, G_EVAL|G_SCALAR, args) < 1)
		return(0);

	SPAGAIN;	/* Rehash stack, it's probably been clobbered */
	return(POPi);	/* Pop an int */
	
}

/*
 *  accept 2 arguments
 *  char *name       = name of the IRC input to hook (PRIVMSG, NOTICE, PING, JOIN, PART, etc..)
 *  char *subroutine = name of the function that should be called in the script for each of the
 *                     hooked input lines (coming from parseline())
 *
 *  this function should be made directly callable from the script as "parse_hook" or similar
 */

/* Don't ask! You don't want to know! */
XS(XS_perl_parse_hook)
{
	Hook	*hook;
	char *name, *sub;
	int c;
	dXSARGS; items = 0;

	/*
	 *  translate *name and *sub from perl variables to C strings
	 *  SvPV(ST(0)) returns a string(char) pointer to the first arg. 
	 *  but I don't know if it's safe to point directly in to perl
	 *  space like that.
	 */
	if ((name = strdup(SvPV(ST(0), i)))) == NULL)
		XSRETURN_EMPTY;

	if ((sub = strdup(SvPV(ST(0), i)))) == NULL)
	{
		free(name);
		XSRETURN_EMPTY;
	}

	/*
	 *  make a Hook struct and link it into the parse hook list
	 */
	set_mallocdoer(perl_parse_hook);
	hook = (Hook*)Malloc(sizeof(Hook) + strlen(name) + strlen(sub));
	hook->func = perl_parse_jump;
	hook->next = hooklist;
	hooklist = hook;

	hook->command = Strcpy(hook->self,sub) + 1;
	Strcpy(hook->command,name);

	free(name);
	free(sub);

	/*
	 *  return successful status to script
	 *  I don't know how to return a number so I return the sub name.
	 */
	XST_mPV(0, sub);
	XSRETURN(1);
}

void init_perl(void)
{
	energymech_perl = perl_alloc();
	perl_construct(energymech_perl);

	/*
	 *  make parse_hook() callable from scripts
	 */
	newXS("mech::parse_hook", XS_perl_parse_hook, "mech");
}

void do_perl(COMMAND_ARGS)
{
	/*
	 *  call init_perl() if the perl interpreter isnt initialized yet
	 */

	/*
	 *  call the perl interpreter with the content of *rest
	 */

	/*
	 *  be verbose about success or fail to the user
	 */
}

void do_perlscript(COMMAND_ARGS)
{
	char *args[2];
	dSP;

	/*
	 *  call init_perl() if the perl interpreter isnt initialized yet
	 */

	/*
	 *  chop(&rest) for name of script filename and load it into the perl interpreter
	 */
	 args[0] = "";
	 args[1] = chop(&rest);

	/* FIXME: Trap parse errors */
	perl_parse(energymech_perl, NULL, 1, argv, (char **)NULL);

	/* Call sub named Init, what should contain
	 * mech::perl_parse_hook("PRIVMSG", "yoink_privmsg");
	 * Note to self: Wouldn't it be better to pass subs by
	 * Reference(perl ver of pointer) instead of name?
	 * How the fsck do i do that?!
	 */
	call_pv("Init", G_EVAL|G_NOARGS);
	if(SvTRUE(ERRSV))
	{
		STRLEN n_a;
		to_user(from, "perl script %s failed to init: %s",
			argv[1], SvPV(ERRSV, n_a));
	}
	/*
	 *  be verbose about success or fail to the user
	 */
}

#endif /* 0 */

#endif /* PERL */
