/*

    EnergyMech, IRC bot software
    Parts Copyright (c) 1997-2004 proton

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
#define IRC_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

void make_ireq(int t, char *from, char *nick)
{
	IReq	*ir;
	char	*pt;

	set_mallocdoer(make_ireq);
	ir = (IReq*)Calloc(sizeof(IReq) + strlen(from) + ((nick) ? strlen(nick) : 0));

	ir->t = t;
	ir->when = now;

	pt = Strcat(ir->from,from) + 1;
	if (nick)
	{
		ir->nick = pt;
		Strcpy(ir->nick,nick);
	}

	ir->next = current->parselist;
	current->parselist = ir;
}

void send_pa(int type, const char *nick, const char *format, ...)
{
	char	text[MAXLEN];
	va_list vargs;
	IReq	*ir,**pp;
	int	pr,end;

	pr   = 0;
	end  = type & PA_END;
	type = type & PA_TYPE;

	pp = &current->parselist;
	while((ir = *pp))
	{
		if (ir->t != type)
			goto next;
		if (nick && nickcmp(nick,ir->nick))
			goto next;

		if (format)
		{
			if (!pr)
			{
				++pr;
				va_start(vargs,format);
				vsprintf(text,format,vargs);
				va_end(vargs);
			}
			to_user(ir->from,"%s",text);
		}
		if (end)
		{
			*pp = ir->next;
			Free((char**)&ir);
			continue;
		}
	next:
		pp = &ir->next;
	}
}

/*
 *
 *  commands that parse irc replies
 *
 */

void do_irclusers(COMMAND_ARGS)
{
	to_server("LUSERS\n");
	make_ireq(PA_LUSERS,from,NULL);
}

void do_ircstats(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: DCC + CARGS
	 */
	char	*line,*serv;

	line = chop(&rest);
	serv = chop(&rest);

	to_server((serv) ? "STATS %s %s\n" : "STATS %s\n",line,serv);
	make_ireq(PA_STATS,from,NULL);
}

void do_ircwhois(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	char	*nick;

	nick = chop(&rest);
	to_server("WHOIS %s\n",nick);
	make_ireq(PA_WHOIS,from,nick);
}

void do_ircuserhost(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	char	*nick;

	nick = chop(&rest);
	to_server("WHOIS %s\n",nick);
	make_ireq(PA_USERHOST,from,nick);
}
