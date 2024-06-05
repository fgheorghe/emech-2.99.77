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
#define KICKSAY_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

void check_kicksay(Chan *chan, char *text)
{
	KickSay *kick;

	for(kick=current->kicklist;kick;kick=kick->next)
	{
		if (*kick->chan == '*' || !Strcasecmp(chan->name,kick->chan))
		{
			if (!matches(kick->mask,text))
			{
				send_kick(chan,CurrentNick,"%s",kick->reason);
				return;
			}
		}
	}
}

KickSay *find_kicksay(char *text, char *channel)
{
	KickSay *kick,*save;
	int	num,best;

	save = NULL;
	best = 0;
	for(kick=current->kicklist;kick;kick=kick->next)
	{
		if (!channel || *kick->chan == '*' || !Strcasecmp(channel,kick->chan))
		{
			num = num_matches(kick->mask,text);
			if (num > best)
			{
				best = num;
				save = kick;
			}
		}
	}
	return(save);
}

void remove_kicksay(KickSay *kick)
{
	KickSay **pp;

	pp = &current->kicklist;
	while(*pp)
	{
		if (*pp == kick)
		{
			*pp = kick->next;
			Free((char**)&kick);
			return;
		}
		pp = &(*pp)->next;
	}
}

void purge_kicklist(void)
{
	while(current->kicklist)
		remove_kicksay(current->kicklist);
}

/*
 * 
 *  kicksay commands
 *
 */

void do_kicksay(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	KickSay *kick;
	char	*channel,*mask;
	int	sz;

	//do_kicksay(CoreUser.name,NULL,rest,0);

	channel = chop(&rest);	/* cant be NULL (CARGS) */
	if (!ischannel(channel) && *channel != '*')
		goto usage;

	if (get_userlevel(from,channel) < cmdaccess)
		return;

	/*
	 *  dig out the mask
	 */
	if (!rest || !*rest || *rest != '"')
		goto usage;
	rest++;

	mask = get_token(&rest,"\"");	/* rest cannot be NULL */
	if (!mask || !*mask)
		goto usage;

	/*
	 *  check for previously existing kicks
	 */
	if ((kick = find_kicksay(mask,channel)) != NULL)
	{
		to_user(from,"I'm already kicking on \"%s\"",kick->mask);
		return;
	}

	/*
	 *  dig out the reason (the rest)
	 */
	while(rest && *rest == ' ')
		rest++;
	if (!rest || !*rest)
		goto usage;

	/*
	 *  add it to the list
	 */
	sz = sizeof(KickSay) + strlen(channel) + strlen(mask) + strlen(rest);
	set_mallocdoer(do_kicksay);
	kick = (KickSay*)Calloc(sz);

	kick->next = current->kicklist;
	current->kicklist = kick;

	kick->chan    = Strcpy(kick->mask,mask) + 1;
	kick->reason  = Strcpy(kick->chan,channel) + 1;
			Strcpy(kick->reason,rest);

	to_user(from,"Now kicking on \"%s\" on %s",mask,channel);
	return;
usage:
	usage(from);	/* usage for CurrentCmd->name */
}

void do_rkicksay(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	KickSay *kick;
	char	*channel;

	channel = chop(&rest);	/* cant be NULL (CARGS) */
	if (!ischannel(channel) && *channel != '*')
		goto usage;

	if (!rest)
		goto usage;

	if (!(kick = find_kicksay(rest,channel)))
	{
		to_user(from,"I'm not kicking on that");
		return;
	}
	to_user(from,"No longer kicking on %s",kick->mask);
	remove_kicksay(kick);
	return;
usage:
	usage(from);	/* usage for CurrentCmd->name */
}

void do_kslist(COMMAND_ARGS)
{
	KickSay *kick;

	if (!current->kicklist)
	{
		to_user(from,"Kicksay list is empty");
		return;
	}

	to_user(from,"--- Channel ------- String ---------- Kick Reason ------");
	for(kick=current->kicklist;kick;kick=kick->next)
	{
		to_user(from,"%15s %s     %s",kick->chan,kick->mask,kick->reason);
	}
}
