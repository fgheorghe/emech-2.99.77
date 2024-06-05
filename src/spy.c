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
#define SPY_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#ifdef DEBUG

LS const char SPY_DEFS[7][12] =
{
	"SPY_FILE",
	"SPY_CHANNEL",
	"SPY_DCC",
	"SPY_STATUS",
	"SPY_MESSAGE",
	"SPY_RAWIRC",
	"SPY_BOTNET"
};

#endif /* DEBUG */

void send_spy(const char *src, char *format, ...)
{
	Chan	*chan;
	Spy	*spy;
	va_list	msg;
	const char *tempsrc;
	char	buf[MAXLEN];
	int	fd,ischan;
	int	printed = FALSE;

	ischan = (*src == '#' || *src == '*') ? TRUE : FALSE;
	tempsrc = (src == SPYSTR_STATUS) ? time2medium(now) : src;

	for(spy=current->spylist;spy;spy=spy->next)
	{
		if (ischan && spy->t_src == SPY_CHANNEL)
		{
			if ((*src != '*') && Strcasecmp(spy->src,src))
				continue;
			if ((chan = find_channel_ac(spy->src)) == NULL)
				continue;
			if (find_chanuser(chan,CurrentNick) == NULL)
				continue;
			tempsrc = spy->src;
		}
		else
		/*
		 *  by using string constants we can compare addresses
		 */
		if (spy->src != src)
			continue;

		if (!printed)
		{
			printed = TRUE;
			va_start(msg,format);
			vsprintf(buf,format,msg);
			va_end(msg);
		}

		switch(spy->t_dest)
		{
		case SPY_DCC:
			to_file(spy->dcc->sock,"[%s] %s\n",tempsrc,buf);
			break;
		case SPY_CHANNEL:
			sendprivmsg(spy->dest,"[%s] %s",tempsrc,buf);
			break;
		case SPY_FILE:
			if ((fd = open(spy->dest,O_WRONLY|O_CREAT|O_APPEND,NEWFILEMODE)) >= 0)
			{
				to_file(fd,"[%s] %s\n",logtime(now),buf);
				close(fd);
			}
			break;
		}
	}
}

void send_global(char *format, ...)
{
	va_list msg;
	Mech	*bot,*backup;
	char	buf[MAXLEN];
	int	printed = FALSE;

	backup = current;
	for(bot=botlist;bot;bot=bot->next)
	{
		if (bot->spy.status)
		{
			current = bot;

			if (!printed)
			{
				printed = TRUE;
				va_start(msg,format);
				vsprintf(buf,format,msg);
				va_end(msg);
			}

			send_spy(SPYSTR_STATUS,"%s",buf);
		}
	}
	current = backup;
}

void spy_typecount(void)
{
	Spy	*spy;

	current->spy.rawirc = current->spy.message =
	current->spy.status = current->spy.channel = 0;

	for(spy=current->spylist;spy;spy=spy->next)
	{
		switch(spy->t_src)
		{
		case SPY_RAWIRC:
			current->spy.rawirc = 1;
			break;
		case SPY_MESSAGE:
			current->spy.message = 1;
			break;
		case SPY_STATUS:
			current->spy.status = 1;
			break;
		case SPY_CHANNEL:
			current->spy.channel = 1;
			break;
		}
	}
}

/*
 *
 *  commands related to spy-pipes
 *
 */

void do_spy(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Spy	*spy;
	char	*src,*dest;
	int	t_src,t_dest;
	int	sz;

	src  = chop(&rest);
	dest = chop(&rest);

	if (!dest || !*dest)
		dest = NULL;

	t_dest = SPY_DCC;
	t_src = SPY_CHANNEL;

	if (!src)
	{
spy_usage:
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	if (!Strcasecmp(src,SPYSTR_STATUS))
	{
		t_src = SPY_STATUS;
		src = SPYSTR_STATUS;
	}
	else
	if (!Strcasecmp(src,SPYSTR_MESSAGE))
	{
		t_src = SPY_MESSAGE;
		src = SPYSTR_MESSAGE;
	}
	else
	if (!Strcasecmp(src,SPYSTR_RAWIRC))
	{
		t_src = SPY_RAWIRC;
		src = SPYSTR_RAWIRC;
	}
	else
	{
		if (!ischannel(src))
			goto spy_usage;
		if (get_userlevel(from,src) < cmdaccess)
			return;
	}

	if (dest && *dest)
	{
		/*
		 *  log to a file
		 */
		if (*dest == '>')
		{
			dest++;
			if (!*dest)
			{
				dest = chop(&rest);
				if (!dest || !*dest)
					goto spy_usage;
			}
			/*
			 *  We dont like filenames with slashes in them.
			 */
			if (STRCHR(dest,'/'))
				goto spy_usage;
			t_dest = SPY_FILE;
			goto spy_dest_ok;
		}

		if (!ischannel(dest))
			goto spy_usage;
		if (get_userlevel(from,dest) < cmdaccess)
		{
			to_user(from,"You don't have enough access on %s",dest);
			return;
		}
		t_dest = SPY_CHANNEL;
	}

spy_dest_ok:
#ifdef DEBUG
	debug("(do_spy) src = `%s'; t_src = %i (%s); dest = `%s'; t_dest = %i (%s)\n",
		src,t_src,SPY_DEFS[t_src-1],nullstr(dest),t_dest,SPY_DEFS[t_dest-1]);
#endif /* DEBUG */

	if (t_dest == SPY_DCC)
	{
		if (!CurrentDCC)
		{
			to_user(from,"Spying is only allowed in DCC chat");
			return;
		}
		dest = CurrentDCC->user->name;
	}

	for(spy=current->spylist;spy;spy=spy->next)
	{
		if ((spy->t_src == t_src) && (spy->t_dest == t_dest) &&
			!Strcasecmp(spy->src,src) && !Strcasecmp(spy->dest,dest))
		{
			to_user(from,"Requested spy channel is already active");
			return;
		}
	}

	set_mallocdoer(do_spy);

	sz = sizeof(Spy);

	if (t_dest != SPY_DCC)
		sz += strlen(dest);

	if (t_src == SPY_CHANNEL)
		sz += strlen(src);

	spy = Calloc(sz);

	if (t_dest != SPY_DCC)
	{
		spy->dest = spy->p;
		spy->src = Strcat(spy->p,dest) + 1;
	}
	else
	{
		spy->dest = CurrentDCC->user->name;
		spy->dcc = CurrentDCC;
		spy->src = spy->p;
	}

	if (t_src == SPY_CHANNEL)
	{
		Strcpy(spy->src,src);
	}
	else
	{
		spy->src = src;
	}

	spy->t_src = t_src;
	spy->t_dest = t_dest;
	spy->next = current->spylist;
	current->spylist = spy;

	switch(t_src)
	{
	case SPY_STATUS:
		send_spy(SPYSTR_STATUS,"(%s) Added to mech core",nickcpy(NULL,from));
		break;
	case SPY_MESSAGE:
		src = "messages";
	default:
		to_user(from,"Spy channel for %s has been activated",src);
	}
	spy_typecount();
}

void do_rspy(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Spy	*spy,**pspy;
	char	*src,*dest,*tmp;
	int	t_src,t_dest;

	src  = chop(&rest);
	dest = chop(&rest);

	t_dest = SPY_DCC;
	t_src = SPY_CHANNEL;

	if (!src)
	{
rspy_usage:
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	if (!Strcasecmp(src,SPYSTR_STATUS))
	{
		t_src = SPY_STATUS;
		src = SPYSTR_STATUS;
	}
	else
	if (!Strcasecmp(src,SPYSTR_MESSAGE))
	{
		t_src = SPY_MESSAGE;
		src = SPYSTR_MESSAGE;
	}
	else
	if (!Strcasecmp(src,SPYSTR_RAWIRC))
	{
		t_src = SPY_RAWIRC;
		src = SPYSTR_RAWIRC;
	}
	else
	{
		if (!ischannel(src))
			goto rspy_usage;
		if (get_userlevel(from,src) < cmdaccess)
			return;
	}

	if (dest && *dest)
	{
		if (*dest == '>')
		{
			dest++;
			if (!*dest)
			{
				dest = chop(&rest);
				if (!dest || !*dest)
					goto rspy_usage;
			}
			/*
			 *  We dont like filenames with slashes in them.
			 */
			if (STRCHR(dest,'/'))
				goto rspy_usage;
			t_dest = SPY_FILE;
			goto rspy_dest_ok;
		}

		if (ischannel(dest))
			t_dest = SPY_CHANNEL;
	}
	else
		dest = from;

rspy_dest_ok:
#ifdef DEBUG
	debug("(do_rspy) src = `%s'; t_src = %i (%s); dest = `%s'; t_dest = %i (%s)\n",
		src,t_src,SPY_DEFS[t_src-1],dest,t_dest,SPY_DEFS[t_dest-1]);
#endif /* DEBUG */

	for(spy=current->spylist;spy;spy=spy->next)
	{
		if ((spy->t_src == t_src) && (spy->t_dest == t_dest) && (!Strcasecmp(spy->src,src)))
		{
			if ((t_dest == SPY_DCC) && (!nickcmp(spy->dest,dest)))
				break;
			else
			if (!Strcasecmp(spy->dest,dest))
				break;
		}
	}
	if (!spy)
	{
		to_user(from,"No matching spy channel could be found");
		return;
	}

	switch(t_src)
	{
	case SPY_STATUS:
		tmp = (t_dest == SPY_DCC) ? nickcpy(NULL,spy->dest) : spy->dest;
		send_spy(SPYSTR_STATUS,"(%s) Removed from mech core",tmp);
		break;
	case SPY_MESSAGE:
		src = "messages";
	default:
		to_user(from,"Spy channel for %s has been removed",src);
		break;
	}

	pspy = &current->spylist;
	while(*pspy)
	{
		if (*pspy == spy)
		{
			*pspy = spy->next;
			Free((char**)&spy);
			return;
		}
		pspy = &(*pspy)->next;
	}

	spy_typecount();
}

void do_spylist(COMMAND_ARGS)
{
	Spy	*spy;
	char	*src,*extra;

	if (!current->spylist)
	{
		to_user(from,"No active spy channels");
		return;
	}

	to_user(from,"\037source\037          \037target\037");

	for(spy=current->spylist;spy;spy=spy->next)
	{
		switch(spy->t_src)
		{
		case SPY_MESSAGE:
			src = "messages";
			break;
		default:
			src = spy->src;
		}
		extra = (spy->t_dest == SPY_FILE) ? " (file)" : "";
		to_user(from,"%-15s %s%s",src,spy->dest,extra);
	}
}
