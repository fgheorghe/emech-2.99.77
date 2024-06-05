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
#define CHANNEL_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

time_t get_idletime(char *nick)
{
	ChanUser *cu,*cu2;
	Chan	*chan;

	cu2 = NULL;
	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if ((cu = find_chanuser(chan,nick)))
		{
			if (!cu2 || (cu->idletime > cu2->idletime))
				cu2 = cu;
		}
	}
	return((cu2) ? cu2->idletime : -1);
}

void check_idlekick(void)
{
	ChanUser *cu;
	Chan	*chan;
	time_t	timeout;
	int	limit;

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if (!chan->bot_is_op)
			continue;
		if ((limit = chan->setting[INT_IKT].int_var) == 0)
			continue;
		timeout = (now - (60 * limit));
		for(cu=chan->users;cu;cu=cu->next)
		{
			if (timeout < cu->idletime)
				continue;
			if (cu->flags & CU_CHANOP)
				continue;
			if (is_user(get_nuh(cu),chan->name))
				continue;
			send_kick(chan,cu->nick,"Idle for %i minutes",limit);
		}
	}
}

Chan *find_channel_ac(char *name)
{
	Chan	*chan;
	uchar	ni;

	ni = tolowertab[(uchar)(name[1])];

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if (!chan->active)
			continue;
		if (ni != tolowertab[(uchar)(chan->name[1])])
			continue;
		if (!Strcasecmp(name,chan->name))
			return(chan);
	}
	return(NULL);
}

Chan *find_channel_ny(char *name)
{
	Chan	*chan;
	uchar	ni;

	ni = tolowertab[(uchar)(name[1])];

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if (ni != tolowertab[(uchar)(chan->name[1])])
			continue;
		if (!Strcasecmp(name,chan->name))
			return(chan);
	}
	return(NULL);
}

void remove_chan(Chan *chan)
{
	Chan	**pp;

	pp = &current->chanlist;
	while(*pp)
	{
		if (*pp == chan)
		{
			*pp = chan->next;
			purge_banlist(chan);
			purge_chanusers(chan);
			delete_vars(chan->setting,CHANSET_SIZE);
			Free(&chan->name);
			Free(&chan->key);
			Free(&chan->topic);
			Free(&chan->kickedby);
			Free((char **)&chan);
			return;
		}
		pp = &(*pp)->next;
	}
}

void join_channel(char *name, char *key)
{
	Chan	*chan;

	if (!ischannel(name))
		return;

	if ((chan = find_channel_ny(name)) == NULL)
	{
		set_mallocdoer(join_channel);
		chan = (Chan*)Calloc(sizeof(Chan));
		set_mallocdoer(join_channel);
		chan->name = Strdup(name);
		if (key)
		{
			set_mallocdoer(join_channel);
			chan->key = Strdup(key);
		}
		copy_vars(chan->setting,current->setting);
		chan->next = current->chanlist;
		chan->rejoin = TRUE;
		chan->active = FALSE;
		current->chanlist = chan;
		current->rejoin = TRUE;
		if (current->sock == -1)
		{
			current->activechan = chan;
			chan->sync = TRUE;
		}
		else
		{
			to_server("JOIN %s %s\n",name,(key && *key) ? key : "");
			chan->sync = FALSE;
		}
		return;
	}
	if (key && (key != chan->key))
	{
		Free(&chan->key);
		set_mallocdoer(join_channel);
		chan->key = Strdup(key);
	}
	if (chan->active)
	{
		current->activechan = chan;
		return;
	}
	/*
	 *  If its not CH_ACTIVE, its CH_OLD; there are only those 2 states.
	 */
	if (current->sock >= 0 && chan->sync)
	{
		to_server("JOIN %s %s\n",name,(key) ? key : "");
		chan->sync = FALSE;
	}
	chan->rejoin = TRUE;
	current->rejoin = TRUE;
}

void reverse_topic(Chan *chan, char *from, char *topic)
{
	if ((chan->setting[TOG_TOP].int_var) &&
		(get_userlevel(from,chan->name) < ASSTLEVEL))
	{
		if (chan->topic && Strcasecmp(chan->topic,topic))
			to_server("TOPIC %s :%s\n",chan->name,chan->topic);
		return;
	}

	Free((char**)&chan->topic);
	set_mallocdoer(reverse_topic);
	chan->topic = Strdup(topic);
}

void cycle_channel(Chan *chan)
{
	if (!chan->sync)
		return;
	chan->rejoin = TRUE;
	to_server("PART %s\nJOIN %s %s\n",chan->name,
		chan->name,(chan->key) ? chan->key : "");
}

int reverse_mode(char *from, Chan *chan, int m, int s)
{
	char	buffer[100];
	char	*ptr,*ptr2;
	char	mode,sign;

	if (!chan->bot_is_op || !chan->setting[TOG_ENFM].int_var ||
	    ((ptr = chan->setting[STR_CHANMODES].str_var) == NULL))
		return(FALSE);

	mode = (char)m;
	sign = (char)s;

	if (STRCHR(ptr,mode) && (sign == '+'))
		return(FALSE);
	if (!STRCHR(ptr,mode) && (sign == '-'))
		return(FALSE);
	if (get_userlevel(from,chan->name) >= ASSTLEVEL)
	{
		ptr2 = buffer;
		if (sign == '-')
		{
			while(*ptr)
			{
				if (*ptr != mode)
					*ptr2++ = *ptr;
				ptr++;
			}
			*ptr2 = 0;
		}
		else
		{
			buffer[0] = mode;
			buffer[1] = 0;
			Strcat(buffer,ptr);
		}
		set_str_varc(chan,STR_CHANMODES,buffer);
		return(FALSE);
	}
	return(TRUE);
}

void chan_modestr(Chan *chan, char *dest)
{
	*(dest++) = '+';
	if (chan->private)
		*(dest++) = 'p';
	if (chan->secret)
		*(dest++) = 's';
	if (chan->moderated)
		*(dest++) = 'm';
	if (chan->topprot)
		*(dest++) = 't';
	if (chan->invite)
		*(dest++) = 'i';
	if (chan->nomsg)
		*(dest++) = 'n';
	if (chan->limitmode && chan->limit)
		*(dest++) = 'l';
	if (chan->keymode)
		*(dest++) = 'k';
	*dest = 0;

	if ((chan->limitmode) && (chan->limit))
	{
		sprintf(dest," %i",chan->limit);
	}
	if (chan->keymode)
	{
		Strcat(dest," ");
		Strcat(dest,(chan->key) ? chan->key : "???");
	}
}

char *find_nuh(char *nick)
{
	Chan	*chan;
	ChanUser *cu;

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if ((cu = find_chanuser(chan,nick)))
			return(get_nuh(cu));
	}
	return(NULL);
}

void make_ban(Ban **banlist, char *from, char *banmask, time_t when)
{
	Ban	*new;
	int	sz;

	for(new=*banlist;new;new=new->next)
	{
		if (!Strcasecmp(new->banstring,banmask))
			return;
	}

	sz = sizeof(Ban) + strlen(from) + strlen(banmask);

	set_mallocdoer(make_ban);
	new = (Ban*)Malloc(sz);

	new->bannedby = Strcpy(new->banstring,banmask) + 1;
			Strcpy(new->bannedby,from);

	new->time = when;
	new->next = *banlist;
	*banlist  = new;
}

void delete_ban(Chan *chan, char *banmask)
{
	Ban	*ban,**pp;

	pp = &chan->banlist;
	while(*pp)
	{
		ban = *pp;
		if (!Strcasecmp(ban->banstring,banmask))
		{
			*pp = ban->next;
			Free((char**)&ban);
			return;
		}
		pp = &(*pp)->next;
	}
}

void purge_banlist(Chan *chan)
{
	Ban	*ban,*next;

	ban = chan->banlist;
	while(ban)
	{
		next = ban->next;
		Free((char**)&ban);
		ban = next;
	}
	chan->banlist = NULL;
}

void channel_massmode(Chan *chan, char *pattern, int filtmode, char mode, char typechar)
{
	ChanUser *cu;
	char	*pat,*uh,burst[MSGLEN],deopstring[MSGLEN];
	int	i,maxmode,mal,willdo,lvl,ispat;

	if ((pat = chop(&pattern)) == NULL)
		return;

	ispat   = (STRCHR(pat,'*')) ? TRUE : FALSE;
	maxmode = current->setting[INT_OPMODES].int_var;
	mal     = chan->setting[INT_MAL].int_var;
	*burst  = 0;

	for(cu=chan->users;cu;)
	{
		sprintf(deopstring,"MODE %s %c",chan->name,mode);
		uh = STREND(deopstring);
		i = maxmode;
		do {
			*(uh++) = typechar;
		}
		while(--i);
		*uh = 0;

		/* i == 0 from the while loop */
		while(cu && (i < maxmode))
		{
			willdo = FALSE;
			if ((mode == '+') && ((cu->flags & filtmode) == 0))
				willdo = TRUE;
			if ((mode == '-') && ((cu->flags & filtmode) != 0))
				willdo = TRUE;
#ifdef DEBUG
			lvl = 0;
#endif /* DEBUG */
			if (willdo)
			{
				willdo = FALSE;
				uh  = get_nuh(cu);
				lvl = get_userlevel(uh,chan->name);
				if (ispat)
				{
					if (!matches(pat,uh))
					{
						if (typechar == 'v')
							willdo = TRUE;
						if ((mode == '+') && (lvl >= mal))
							willdo = TRUE;
						if ((mode == '-') && (lvl < mal))
							willdo = TRUE;
					}
				}
				else
				if (!nickcmp(pat,cu->nick))
				{
					if (mode == '-')
					{
						/*
						 *  never deop yourself, stupid bot
						 */
						if (nickcmp(pat,current->nick))
							willdo = TRUE;
					}
					else
						willdo = TRUE;
				}
			}
#ifdef DEBUG
			else
			{
				uh  = get_nuh(cu);
			}
			debug("(massmode(2)) willdo = %s (%s[%i]) (pat=%s)\n",
				(willdo) ? "TRUE" : "FALSE",uh,lvl,pat);
#endif /* DEBUG */
			if (willdo && ((cu->flags & CU_MASSTMP) == 0))
			{
				Strcat(deopstring," ");
				Strcat(deopstring,cu->nick);
				cu->flags |= CU_MASSTMP;
				i++;
			}

			cu = cu->next;
			if (!cu && (pat = chop(&pattern)))
			{
				ispat = (STRCHR(pat,'*')) ? TRUE : FALSE;
				cu = chan->users;
			}
		}

		if (i)
		{
			if ((strlen(deopstring) + strlen(burst)) >= MSGLEN-2)
			{
				write(current->sock,burst,strlen(burst));
#ifdef DEBUG
				debug("(channel_massmode)\n%s\n",burst);
#endif /* DEBUG */
				*burst = 0;
			}
			Strcat(burst,deopstring);
			Strcat(burst,"\n");
		}
	}

	if (strlen(burst))
	{
		write(current->sock,burst,strlen(burst));
#ifdef DEBUG
		debug("(...)\n%s\n",burst);
#endif /* DEBUG */
	}

	for(cu=chan->users;cu;cu=cu->next)
		cu->flags &= ~CU_MASSTMP;
}

void channel_massunban(Chan *chan, char *pattern, time_t seconds)
{
	Ban	*ban;
	int	pri;

	pri = (seconds) ? 180 : 90;

	for(ban=chan->banlist;ban;ban=ban->next)
	{
		if (!matches(pattern,ban->banstring) || !matches(ban->banstring,pattern))
		{
			if (!seconds || ((now - ban->time) > seconds))
			{
				send_mode(chan,pri,QM_RAWMODE,'-','b',(void*)ban->banstring);
			}
		}
	}
}

/*
 *  Channel userlist stuff
 */

/*
 *  this is one of the big cpu hogs in the energymech.
 *  its been debugged a whole lot and probably cant be
 *  made much better without big changes.
 *
 *  cache hit ratio (%): 2 - 4    (smaller channels)
 *                       4 - 10   (larger channels)
 *
 *  nickcmp is called for an average of 1/19th of all chanusers,
 *  the rest is avoided with the first-char comparison.
 *
 *  for each nickcmp call, 10-15% cpu is saved by skipping one char
 *  into both nicks (first-char comparison has already been made).
 */
ChanUser *find_chanuser(Chan *chan, char *nick)
{
	ChanUser *cu;
	uchar	ni;

	/*
	 *  small quick'n'dirty cache
	 */
	if (chan->cacheuser && !nickcmp(nick,chan->cacheuser->nick))
		return(chan->cacheuser);

	/*
	 *  avoid calling nickcmp if first char doesnt match
	 */
	ni = nickcmptab[(uchar)(*nick)];
	nick++;

	/*
	 *  hog some cpu...
	 */
	for(cu=chan->users;cu;cu=cu->next)
	{
		if (ni == nickcmptab[(uchar)(*cu->nick)])
		{
			if (!nickcmp(nick,cu->nick+1))
				return(chan->cacheuser = cu);
		}
	}
	return(NULL);
}

void remove_chanuser(Chan *chan, char *nick)
{
	ChanUser *cu,**pp;
	uchar	ni;

	/*
	 *  avoid calling Strcasecmp if first char doesnt match
	 */
	ni = nickcmptab[(uchar)(*nick)];
	nick++;

	/*
	 *  Dont call find_chanuser() because it caches the found user
	 *  and we dont want to cache a user who quits/parts/is kicked...
	 */
	pp = &chan->users;
	while(*pp)
	{
		cu = *pp;
		if (ni == nickcmptab[(uchar)(*cu->nick)])
		{
			if (!nickcmp(nick,cu->nick+1))
			{
				if (cu == chan->cacheuser)
					chan->cacheuser = NULL;
				*pp = cu->next;
				/*
				 *  the mode-queue might contain a reference to this
				 *  chanuser, remove it.
				 */
				unmode_chanuser(chan,cu);
				/*
				 *  byebye chanuser
				 */
				Free((char**)&cu->nick);
				Free((char**)&cu);
				return;
			}
		}
		pp = &cu->next;
	}
}

/*
 *  Requires CurrentChan to be set properly
 */
void make_chanuser(char *nick, char *userhost)
{
	ChanUser *new;

	/*
	 *  malloc ChanUser record with buffer space for user and host in
	 *  a single chunk and calculate the offsets for the strings
	 */
	set_mallocdoer(make_chanuser);
	new = (ChanUser*)Malloc(sizeof(ChanUser) + strlen(userhost));

	new->flags = 0;
	new->idletime = now;

	new->user = NULL;
	new->shit = NULL;

	/*
	 *  set to zero, the number of times doesnt matter since the
	 *  "last time" was 30+ years ago.
	 */
	new->floodtime = new->bantime = new->deoptime = \
	new->kicktime = new->nicktime = new->capstime = 0;

	new->next = CurrentChan->users;
	CurrentChan->users = new;

	Strcpy(new->userhost,userhost);

	/*
	 *  nick can change without anything else changing with it
	 */
	set_mallocdoer(make_chanuser);
	new->nick = Strdup(nick);
}

void purge_chanusers(Chan *chan)
{
	while(chan->users)
		remove_chanuser(chan,chan->users->nick);
}

char *get_nuh(ChanUser *user)
{
	sprintf(nuh_buf,"%s!%s",user->nick,user->userhost);
	return(nuh_buf);
}

/*
 *
 *  commands associated with channels
 *
 */

void do_join(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	char	*channel,*key;

	channel = chop(&rest);
	if (!ischannel(channel))
	{
		to_user(from,"Invalid channel name");
		return;
	}
	if (get_authlevel(from,channel) < cmdaccess)
		return;
	to_user(from,"Attempting the join of %s",channel);
	key = chop(&rest);
	join_channel(channel,key);
}

void do_part(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CAXS
	 */
	Chan	*chan;

	if ((chan = find_channel_ac(to)) == NULL)
	{
		to_user(from,ERR_CHAN,to);
		return;
	}
	to_user(from,"Leaving %s",to);
	chan->rejoin = FALSE;
	chan->active = FALSE;
	to_server("PART %s\n",to);
	if (chan == current->activechan)
	{
		for(chan=current->chanlist;chan;chan=chan->next)
			if (chan->active)
				break;
		current->activechan = chan;
	}
}

void do_cycle(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS
	 */
	Chan	*chan;

	if ((chan = find_channel_ac(to)) == NULL)
	{
		to_user(from,ERR_CHAN,to);
		return;
	}
	to_user(from,"Cycling channel %s",to);
	cycle_channel(chan);
}

void do_forget(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Chan	*chan;
	char	*channel;

	channel = chop(&rest);	/* cant be NULL (CARGS) */
	if ((chan = find_channel_ny(channel)) == NULL)
	{
		to_user(from,"Channel %s is not in memory",channel);
		return;
	}
	if (chan->active)
	{
		to_user(from,"I'm currently active on %s",channel);
		return;
	}
	to_user(from,"Channel %s is now forgotten",channel);
	remove_chan(chan);
}

void do_channels(COMMAND_ARGS)
{
	ChanUser *cu;
	Chan	*chan;
	char	text[MSGLEN];
	char	*p;
	int	u,o,v;

	if (current->chanlist == NULL)
	{
		to_user(from,ERR_NOCHANNELS);
		return;
	}
	to_user(from,
		"\037channel\037                            "
		"\037@\037   \037users\037   \037ops\037   \037voiced\037   \037modes\037");
	for(chan=current->chanlist;chan;chan=chan->next)
	{
		*(p = text) = 0;
		p = Strcat(p,chan->name);
		if (chan == current->activechan)
			p = Strcat(p," (current)");
		while(p < text+35)
			*(p++) = ' ';
		if (chan->active)
		{
			u = o = v = 0;
			for(cu=chan->users;cu;cu=cu->next)
			{
				u++;
				if (cu->flags & CU_CHANOP)
					o++;
				else
				if (cu->flags & CU_VOICE)
					v++;
			}
			sprintf(p,"%c   %-8i%-6i%-9i",(chan->bot_is_op) ? '@' : ' ',u,o,v);
			p = STREND(p);
			chan_modestr(chan,p);
		}
		else
		{
			sprintf(p,"    --      --    --       %s",(chan->rejoin) ? "(Trying to rejoin...)" : "(Inactive)");
		}
		to_user(from,"%s",text);
	}
}

void do_wall(COMMAND_ARGS)
{
	ChanUser *cu;
	Chan	*chan;
	char	*channel;

	channel = get_channel(to,&rest);

	if ((chan = find_channel_ac(channel)) == NULL)
	{
		to_user(from,ERR_CHAN,channel);
		return;
	}

#ifdef IRCD_EXTENSIONS
	if(current->ircx_flags & IRCX_WALLCHOPS)
	{
		to_server("WALLCHOPS %s :[Wallop/%s] %s\n",channel,channel,rest);
	}
	else
#endif /* IRCD_EXTENSIONS */
	if (current->setting[TOG_ONOTICE].int_var)
	{
		to_server("NOTICE @%s :[Wallop/%s] %s\n",channel,channel,rest);
	}
	else
	{
		/*
		 *  FIXME: this will cause excess flood in channels with many ops
		 */
		for(cu=chan->users;cu;cu=cu->next)
		{
			if (cu->flags & CU_CHANOP)
			{
				to_server("NOTICE %s :[Wallop/%s] %s\n",
					cu->nick,channel,rest);
			}
		}
	}
	to_user(from,TEXT_SENTWALLOP,channel);
}

void do_mode(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	char	*target;
	int	uaccess;

#ifdef DEBUG
	debug("(do_mode) to = %s, rest = %s\n",nullstr(to),nullstr(rest));
#endif /* DEBUG */

	if ((uaccess = get_authlevel(from,to)) < cmdaccess)
		return;
	if (ischannel(to))
	{
		to_server("MODE %s %s\n",to,rest);
	}
	else
	{
		target = chop(&rest);

		if (!nickcmp(current->nick,target))
		{
			to_server("MODE %s %s\n",target,rest);
		}
	}
}
