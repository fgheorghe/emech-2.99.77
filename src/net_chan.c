/*

    EnergyMech, IRC bot software
    Copyright (c) 1997-2004 proton

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
#define NET_C
#include "config.h"

#ifdef BOTNET

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

ChanUser *find_chanbot(Chan *chan, char *nick)
{
	ChanUser *cu;

	if (chan->cacheuser && !nickcmp(nick,chan->cacheuser->nick))
		return(chan->cacheuser);

	for(cu=chan->users;cu;cu=cu->next)
	{
		if (cu->user && cu->user->access == BOTLEVEL)
		{
			if (!nickcmp(nick,cu->nick))
				return(chan->cacheuser = cu);
		}
	}
	return(NULL);
}

void check_botjoin(Chan *chan, ChanUser *cu)
{
	BotNet	*bn;
	BotInfo *binfo;

#ifdef DEBUG
	debug("(check_botjoin) chan = %s; cu = %s!%s\n",chan->name,cu->nick,cu->userhost);
#endif /* DEBUG */

	for(bn=botnetlist;bn;bn=bn->next)
	{
		if (bn->status != BN_LINKED)
			continue;

		if (bn->rbot)
		{
			if (!nickcmp(cu->nick,bn->rbot->nuh) &&
				!Strcasecmp(cu->userhost,getuh(bn->rbot->nuh)))
				goto opme;
		}
		for(binfo=bn->subs;binfo;binfo=binfo->next)
		{
			if (!nickcmp(cu->nick,binfo->nuh) &&
				!Strcasecmp(cu->userhost,getuh(binfo->nuh)))
				goto opme;
		}
	}
	return;
opme:
	cu->flags |= CU_NEEDOP;
	send_mode(chan,50,QM_CHANUSER,'+','o',(void*)cu);
#ifdef DEBUG
	debug("(check_botjoin) CU_NEEDOP set, mode pushed\n");
#endif /* DEBUG */
}

void check_botinfo(BotInfo *binfo)
{
	Chan	*chan;
	ChanUser *cu;
	Mech	*backup;
	char	*userhost;

	userhost = getuh(binfo->nuh);

	backup = current;
	for(current=botlist;current;current=current->next)
	{
		for(chan=current->chanlist;chan;chan=chan->next)
		{
			if ((cu = find_chanbot(chan,binfo->nuh)) == NULL)
				continue;
			if (!Strcasecmp(cu->userhost,userhost))
			{
				cu->flags |= CU_NEEDOP;
				send_mode(chan,50,QM_CHANUSER,'+','o',(void*)cu);
			}
		}
	}
	current = backup;
}

/*
 *
 *  protocol routines
 *
 */

#endif /* BOTNET */
