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
#define PROTECTION_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

/*
 *
 *  kicking and screaming
 *
 */

void send_kick(Chan *chan, const char *nick, const char *format, ...)
{
	qKick	*new,**pp;
	va_list	vargs;

	/*
	 *  sock_buf safe to use since we're a `tail' function
	 */
	va_start(vargs,format);
	vsprintf(sock_buf,format,vargs);
	va_end(vargs);

	pp = &chan->kicklist;
	while(*pp)
		pp = &(*pp)->next;

	set_mallocdoer(send_kick);
	*pp = new = (qKick*)Malloc(sizeof(qKick) + strlen(nick) + strlen(sock_buf));
	new->next = NULL;

	new->reason = Strcpy(new->nick,nick) + 1;
	Strcpy(new->reason,sock_buf);
}

void push_kicks(Chan *chan)
{
	qKick	*kick;
	int	n;

	n = (current->sendq_time - now);
	while(n < 6)
	{
		if ((kick = chan->kicklist) == NULL)
			return;

		chan->kicklist = kick->next;
		to_server("KICK %s %s :%s\n",chan->name,kick->nick,kick->reason);
		Free((char**)&kick);
		n += 2;
	}
}

void unmode_chanuser(Chan *chan, ChanUser *cu)
{
	qMode	*mode,**pp;

	pp = &chan->modelist;
	while(*pp)
	{
		mode = *pp;
		if ((mode->type == QM_CHANUSER) && (mode->data == (void*)cu))
		{
			*pp = mode->next;
			Free((char**)&mode);
			/*
			 *  there might be more modes associated with this chanuser
			 */
			continue;
		}
		pp = &mode->next;
	}
}

void send_mode(Chan *chan, int pri, int type, char plusminus, char modeflag, void *data)
{
	qMode	*mode,**pp;

#ifdef NO_DEBUG
	debug("(send_mode) chan = %s; pri = %i; type = %i; plusminus = %c; modeflag = %c; data = "mx_pfmt"\n",
		chan->name,pri,type,plusminus,modeflag,(mx_ptr)data);
#endif /* DEBUG */

	/*
	 *  make minusmodes always one priority lower than plusmodes
	 */
	if (plusminus == '-')
		pri |= 1;

	pp = &chan->modelist;
	while(*pp)
	{
		mode = *pp;
		if (mode->pri == pri)
		{
			/*
			 *  check for duplicate
			 */
			if ((mode->type == type) && (mode->plusminus == plusminus) &&
				(mode->modeflag == modeflag) && (mode->data == data))
				return;
		}
		if (mode->pri > pri)
			break;
		pp = &mode->next;
	}

	set_mallocdoer(send_mode);
	mode = (qMode*)Malloc(sizeof(qMode));
	mode->next = *pp;
	*pp = mode;
	mode->pri = pri;
	mode->type = type;
	mode->plusminus = plusminus;
	mode->modeflag = modeflag;

	switch(type)
	{
	case QM_CHANUSER:
		mode->data = data;
		break;
	default:
		if (data)
		{
			set_mallocdoer(send_mode);
			mode->data = (void*)Strdup((char*)data);
		}
		else
		{
			mode->data = NULL;
		}
	}
}

int mode_effect(Chan *chan, qMode *mode)
{
	ChanUser *cu;
	int	f;

	if (mode->type == QM_CHANUSER)
	{
		cu = (ChanUser*)mode->data;
		f = 0;
		switch(mode->modeflag)
		{
		case 'o':
			f = (cu->flags & CU_CHANOP) ? '+' : '-';
			break;
		case 'v':
			f = (cu->flags & CU_VOICE) ? '+' : '-';
			break;
		}
		if (f && f == mode->plusminus)
			return(FALSE);
	}
	return(TRUE);
}

void push_modes(Chan *chan, int lowpri)
{
	qMode	*mode;
	char	flaglist[32],parmlist[MSGLEN];
	char	*dstflag,*dstparm,*srcparm,lastmode;
	int	n,maxmodes;

	n = (current->sendq_time - now);

loop:
	maxmodes = current->setting[INT_OPMODES].int_var;
	lastmode = 0;
	dstflag = flaglist;
	dstparm = parmlist;
	while((mode = chan->modelist))
	{
		/*
		 *  if the line has already been partially filled,
		 *  then its ok to fill up "empty slots" with non-priority modes.
		 */
		if (lowpri && !lastmode && (mode->pri >= QM_PRI_LOW))
			return;
		chan->modelist = mode->next;
		if (mode_effect(chan,mode))
		{
			switch(mode->type)
			{
			case QM_CHANUSER:
				srcparm = ((ChanUser*)mode->data)->nick;
				break;
			default:
				srcparm = (char*)mode->data;
			}
			if (mode->plusminus != lastmode)
				*(dstflag++) = lastmode = mode->plusminus;
			*(dstflag++) = mode->modeflag;
			if (srcparm)
			{
				*(dstparm++) = ' ';
				dstparm = Strcpy(dstparm,srcparm);
			}
			maxmodes--;
		}
#ifdef NO_DEBUG
		else
		{
			debug("(push_modes) ineffectual mode: %c%c %s\n",mode->plusminus,mode->modeflag,
				(mode->type == QM_CHANUSER) ? ((ChanUser*)mode->data)->nick : (char*)mode->data);
		}
#endif /* DEBUG */
		if (mode->type != QM_CHANUSER)
			Free((char**)&mode->data);
		Free((char**)&mode);
		if (!maxmodes)
			break;
	}
	if (!lastmode)
		return;
	*dstflag = 0;
	*dstparm = 0;
	/*
	 *  the missing space is not a bug.
	 */
	to_server("MODE %s %s%s\n",chan->name,flaglist,parmlist);
	n += 2;

	if (lowpri && n < lowpri)
		goto loop;
}

void update_modes(Chan *chan)
{
	ChanUser *cu;

	for(cu=chan->users;cu;cu=cu->next)
	{
		if ((cu->flags & (CU_CHANOP|CU_NEEDOP)) == CU_NEEDOP)
		{
#ifdef NO_DEBUG
			debug("(update_modes) pushing: MODE %s +o %s!%s\n",
				chan->name,cu->nick,cu->userhost);
#endif /* DEBUG */
			send_mode(chan,50,QM_CHANUSER,'+','o',(void*)cu);
		}
	}
}

/*
 *
 *
 *
 */

/*
 *  check_mass() takes no action, it only touches the abuse counters
 *  and timers, TRUE is returned if the limit is reached
 */
int check_mass(Chan *chan, ChanUser *doer, int type)
{
	time_t	*last;
	int	*num,limit;

	/*
	 *  must handle servers ...
	 */
	if (!doer)
		return(FALSE);

	if (doer->user && doer->user->access >= ASSTLEVEL)
		return(FALSE);

	if ((type <= CHK_PUBLIC) && (doer->flags & CU_CHANOP))
		return(FALSE);

	switch(type)
	{
	/*
	 *  two things we dont want channel users to do
	 */
	case CHK_CAPS:
		limit = chan->setting[INT_CKL].int_var;
		last = &doer->capstime;
		num = &doer->capsnum;
		break;
	case CHK_PUB:
		limit = chan->setting[INT_FL].int_var;
		last = &doer->floodtime;
		num = &doer->floodnum;
		break;
	/*
	 *  three things we dont want channel ops to do
	 */
	case CHK_DEOP:
		limit = chan->setting[INT_MDL].int_var;
		last = &doer->deoptime;
		num = &doer->deopnum;
		break;
	case CHK_BAN:
		limit = chan->setting[INT_MBL].int_var;
		last = &doer->bantime;
		num = &doer->bannum;
		break;
	default:
/*	case CHK_KICK: */
		limit = chan->setting[INT_MKL].int_var;
		last = &doer->kicktime;
		num = &doer->kicknum;
		break;
	}

	if ((now - *last) > 10)
	{
		*last = now;
		*num = 0;
	}
	++*num;
	if (*num >= limit && limit)
		return(TRUE);
	return(FALSE);
}

void mass_action(Chan *chan, ChanUser *doer)
{
	int	mpl;

	if ((mpl = chan->setting[INT_MPL].int_var) == 0)
		return;

	if (mpl >= 2)
	{
		if (0 == (doer->flags & CU_DEOPPED) || 0 == (doer->flags & CU_BANNED))
		{
			deop_ban(chan,doer,NULL);
			doer->flags |= CU_DEOPPED|CU_BANNED;
		}
	}

	if (0 == (doer->flags & CU_KICKED))
	{
		send_kick(chan,CurrentNick,KICK_MASSMODES);
		doer->flags |= CU_KICKED;
	}
}

void prot_action(Chan *chan, char *from, ChanUser *doer, char *target, ChanUser *victim)
{
	int	maxprot,uprot;

	/*
	 *  cant do anything to a user that isnt on the channel
	 *  doer is normally supplied, but not always
	 */
	if (!doer)
	{
		if ((doer = find_chanuser(chan,from)) == NULL)
			return;
	}

	/*
	 *  No protective measures for doers with high access
	 */
	if (doer->user && doer->user->access >= ASSTLEVEL)
		return;

	maxprot = chan->setting[INT_PROT].int_var;

	if (victim)
		uprot = (victim->user) ? victim->user->prot : 0;
	else
	{
		uprot = get_protuseraccess(chan,target);
	}

	if ((uprot >= 4) && (!(doer->flags & CU_BANNED)))
	{
		doer->flags |= CU_BANNED|CU_DEOPPED;
		deop_ban(chan,doer,NULL);
	}

	if ((uprot >= 3) && (!(doer->flags & CU_KICKED)))
	{
		doer->flags |= CU_KICKED;
		send_kick(chan,doer->nick,"\002%s is Protected\002",(target) ? target : get_nuh(victim));
	}
	/*
	 *  with (uprot == 2) our ONLY action is to deop the guilty party
	 */
	if ((uprot == 2) && (!(doer->flags & CU_DEOPPED)))
	{
		doer->flags |= CU_DEOPPED;
		send_mode(chan,50,QM_CHANUSER,'-','o',(void*)doer);
	}
}
