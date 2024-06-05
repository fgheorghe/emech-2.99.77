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
#define COM_ONS_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

/*
 *  :nick!user@host KICK #channel kicknick :message
 */
void on_kick(char *from, char *rest)
{
	Chan	*chan;
	ChanUser *doer,*victim;
	char	*channel,*nick;

	channel = chop(&rest);
	if ((CurrentChan = chan = find_channel_ac(channel)) == NULL)
		return;

	nick = chop(&rest);
	if (rest && *rest == ':')
		rest++;

	nickcpy(CurrentNick,from);

	if (current->spy.channel)
		send_spy(chan->name,"*** %s was kicked by %s (%s)",nick,CurrentNick,rest);

	if (!nickcmp(current->nick,nick))
	{
		Free(&chan->kickedby);
		set_mallocdoer(on_kick);
		chan->kickedby = Strdup(from);
		chan->active = FALSE;
		chan->sync = TRUE;
		chan->bot_is_op = FALSE;
		join_channel(chan->name,chan->key);
		/*
		 *  if we're kicked from the active channel, we need to find a new
		 *  channel to set as the active one.
		 */
		if (chan == current->activechan)
		{
			for(chan=current->chanlist;chan;chan=chan->next)
			{
				if (chan->active)
					break;
			}
			current->activechan = chan;
			/*
			 *  Might be set to NULL now, but its supposed to be checked whenever used.
			 *  If not, we get a SEGV; and fix it.
			 */
		}
#ifdef STATS
		if (chan->stats)
			chan->stats->flags |= CSTAT_PARTIAL;
#endif /* STATS */
		/*
		 *  this is the only return we can do: its the bot itself!
		 *  channel userlist will be reconstructed on rejoin
		 */
		return;
	}

#ifdef STATS
	if (chan->setting[STR_STATS].str_var)
		stats_plusminususer(chan,-1);
	if (chan->stats)
		chan->stats->kicks++;
#endif /* STATS */

	/*
	 *  making life easy for ourselves
	 */
	victim = find_chanuser(chan,nick);
	doer = NULL;

	if (chan->bot_is_op)
	{
		/*
		 *  are we supposed to check for channel mass kicks?
		 */
		if (chan->setting[INT_MPL].int_var)
		{
			doer = find_chanuser(chan,from);
			if (check_mass(chan,doer,CHK_KICK))
				mass_action(chan,doer);
		}
		/*
		 *  are we supposed to protect users?
		 */
		if (chan->setting[INT_PROT].int_var)
		{
			if (victim->user && victim->user->prot)
			{
				/*
				 *  doer might be NULL, prot_action() handles it
				 */
				prot_action(chan,from,doer,NULL,victim);
				to_server("INVITE %s %s\n",nick,channel);
			}
		}
	}

	/*
	 *  cant delete users who arent there
	 */
	if (victim)
	{
#ifdef SEEN
		make_seen(nick,victim->userhost,from,rest,now,SEEN_KICKED);
#endif /* SEEN */

		/*
		 *  Dont delete the poor sod before all has been processed
		 */
		remove_chanuser(chan,nick);
	}
}

void on_join(Chan *chan, char *from)
{
	Ban	*ban;
	ChanUser *cu;
	char	*channel;
	int	vpri;

	/*
	 *  Satisfy spies before we continue...
	 */
	if (current->spy.channel)
		send_spy(chan->name,"*** Joins: %s (%s)",CurrentNick,getuh(from));
	/*
	 *
	 */
#ifdef GREET
	if (!CurrentShit && CurrentUser && CurrentUser->greet)
		greet();
#endif /* GREET */
	/*
	 *  No further actions to be taken if the bot isnt opped
	 */
	if (!chan->bot_is_op)
		return;

	channel = chan->name;
	cu      = chan->users;

	/*
	 *  Some stuff only applies to non-users
	 */
	if (!CurrentUser)
	{
		/*
		 *  Kick banned (desynched) users if ABK is set
		 */
		if (chan->setting[TOG_ABK].int_var)
		{
			for(ban=chan->banlist;ban;ban=ban->next)
			{
				if (!matches(ban->banstring,from))
					break;
			}
			if (ban)
			{
				send_kick(chan,CurrentNick,KICK_BANNED);
				return;
			}
		}
		/*
		 *  Kickban users with control chars in their ident
		 *  (which doesnt violate RFC1413 but is bloody annoying)
		 */
		if (chan->setting[TOG_CTL].int_var)
		{
			if (STRCHR(from,'\031') || STRCHR(from,'\002') || STRCHR(from,'\022') || STRCHR(from,'\026'))
			{
				deop_siteban(chan,cu);
				send_kick(chan,CurrentNick,KICK_BAD_IDENT);
				return;
			}
		}
	}
	/*
	 *  If they're shitted, they're not allowed to be opped or voiced
	 */
	if (CurrentShit)
	{
		shit_action(chan,cu);
		return;
	}
	/*
	 *  Check for +ao users if AOP is toggled on
	 */
	if (chan->setting[TOG_AOP].int_var)
	{
		if (cu->user && cu->user->aop)
		{
			send_mode(chan,140,QM_CHANUSER,'+','o',(void*)cu);
			return;
		}
	}
	/*
	 *  If AVOICE eq 0 we have nothing more to do
	 */
	vpri = 200;
	switch(chan->setting[INT_AVOICE].int_var)
	{
	case 1:
		vpri = 150;
		if (cu->user && cu->user->avoice)
			break;
		/* fall through */
	case 0:
		return;
	}
	send_mode(chan,vpri,QM_CHANUSER,'+','v',(void*)cu);
}

void on_nick(char *from, char *newnick)
{
	ChanUser *cu;
	Chan	*chan;
	char	newnuh[NUHLEN];
	int	maxcount;
	int	isbot;

	nickcpy(CurrentNick,from);

#ifdef FASTNICK
	/*
	 *  grab the nick *RIGHT NOW*
	 */
	if (!nickcmp(CurrentNick,current->wantnick))
		to_server("NICK %s\n",current->wantnick);
#endif /* FASTNICK */

	/*
	 *  make the new From string
	 */
	sprintf(newnuh,"%s!%s",newnick,getuh(from));

#ifdef SEEN
	make_seen(CurrentNick,from,newnick,NULL,now,SEEN_NEWNICK);
#endif /* SEEN */

	/*
	 *  snooping buggers
	 */
	if (current->spy.channel)
		send_spy("*","*** %s is now known as %s",CurrentNick,newnick);

	change_authnick(from,newnuh);

	if ((isbot = !nickcmp(current->nick,CurrentNick)))
	{
		setbotnick(current,newnick);
	}

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if ((cu = find_chanuser(chan,from)) == NULL)
			continue;

		/*
		 *  only need to realloc the buffer if its too small
		 */
		if (strlen(cu->nick) >= strlen(newnick))
		{
			Strcpy(cu->nick,newnick);
		}
		else
		{
			Free((char**)&cu->nick);
			set_mallocdoer(on_nick);
			cu->nick = Strdup(newnick);
		}

		/*
		 *  if the bot isnt opped, there's nothing more to do
		 */
		if (!chan->bot_is_op)
			continue;

		/*
		 *  if its the current bot, we dont do diddly squat
		 */
		if (isbot)
			continue;

		shit_action(chan,cu);

		/*
		 *  check for nick-change-flood
		 */
		if ((maxcount = chan->setting[INT_NCL].int_var) < 2)
			continue;

		if ((now - cu->nicktime) > NICKFLOODTIME)
		{
			cu->nicktime = now + (NICKFLOODTIME / (maxcount - 1));
			cu->nicknum = 1;
		}
		else
		{
			cu->nicktime += (NICKFLOODTIME / (maxcount - 1));
			cu->nicknum++;
			if (++cu->nicknum >= maxcount)
			{
				deop_ban(chan,cu,NULL);
				send_kick(chan,newnick,KICK_NICKFLOOD);
			}
		}
	}
}

void on_msg(char *from, char *to, char *msg)
{
#ifdef ALIAS
	char	amem[MSGLEN];		/* big buffers at the top */
	Alias	*alias;
	int	arec;
#endif /* ALIAS */
	char	*command,*pt,*orig;
	int	has_cc,has_bang;
	int	uaccess;
	int	i,j;

	/*
	 *  No line sent to this routine should be longer than MSGLEN
	 *  Callers responsibility to check that from, to and msg is
	 *  non-NULL and non-zerolength
	 */

#ifdef NOTE
	if (notelist && catch_note(from,to,msg))
		return;
#endif /* NOTE */

	/*
	 * If the message is for a channel and we dont accept
	 * public commands, we can go directly to common_public()
	 */
	if (CurrentChan && !CurrentChan->setting[TOG_PUB].int_var)
	{
		common_public(CurrentChan,from,"<%s> %s",msg);
		return;
	}

	if (CurrentDCC)
	{
		uaccess = CurrentUser->access;
	}
	else
	if ((uaccess = get_authlevel(from,NULL)) > OWNERLEVEL)
	{
		/*
		 *  If its a bot we want nothing to do with it
		 */
		return;
	}

	/*
	 *  remember where the string started
	 */
	orig = msg;

	if (from == CoreUser.name)
	{
		has_cc = TRUE;
	}
	else
	{
		has_cc = (current->setting[TOG_CC].int_var) ? FALSE : TRUE;
	}

	if ((command = chop(&msg)) == NULL)
		return;

	if (!nickcmp(current->nick,command))
	{
		if ((command = chop(&msg)) == NULL)
			return;
		has_cc = TRUE;
	}

	has_bang = FALSE;
	if (*command == current->setting[CHR_CMDCHAR].char_var)
	{
		has_cc = TRUE;
		command++;
	}
	else
	if (!has_cc && *command == '!')
	{
		has_bang = TRUE;
		command++;
	}

#ifdef ALIAS
	arec = 0;
recheck_alias:
#endif /* ALIAS */

#ifdef ALIAS
	for(alias=aliaslist;alias;alias=alias->next)
	{
		if (!Strcasecmp(alias->alias,command))
		{
			afmt(amem,alias->format,msg);
#ifdef DEBUG
			debug("(on_msg) [ALIAS] %s %s --> %s\n",command,msg,amem);
#endif /* DEBUG */
			msg = amem;
			pt = chop(&msg);
			i = Strcasecmp(pt,command);
			command = pt;
			arec++;
			if ((arec < MAXALIASRECURSE) && (i != 0))
				goto recheck_alias;
		}
	}
#endif /* ALIAS */

	for(i=0;mcmd[i].name;i++)
	{
		if (!has_cc && mcmd[i].cc && !(has_bang && mcmd[i].cbang))
			continue;
		if (uaccess < acmd[i])
			continue;
		j = Strcasecmp(mcmd[i].name,command);
		if (j < 0)
			continue;
		if (j > 0)
			break;

		if (mcmd[i].nopub && CurrentChan)
		{
#ifdef DEBUG
			debug("(on_msg) Public command (%s) ignored\n",command);
#endif /* DEBUG */
			return;
		}

		if (mcmd[i].dcc && !CurrentDCC)
		{
			dcc_chat(from);
			return;
		}

		/*
		 *  convert the command to uppercase
		 */
		for(pt=command;*pt;pt++)
		{
			if (*pt >= 'a' && *pt <= 'z')
				*pt = *pt - 0x20;
		}

		/*
		 *  send statmsg with info on the command executed
		 */
		if (current->setting[TOG_SPY].int_var)
		{
			send_spy(SPYSTR_STATUS,":%s[%i]: Executing %s[%i]",
				CurrentNick,uaccess,command,(int)acmd[i]);
		}

		/*
		 *  list of last LASTCMDSIZE commands
		 */
		if (from != CoreUser.name)
		{
			Free(&current->lastcmds[LASTCMDSIZE-1]);
			for(j=LASTCMDSIZE-2;j>=0;j--)
				current->lastcmds[j+1] = current->lastcmds[j];
			if ((pt = STRCHR(from,'@')) == NULL)
				pt = from;
			set_mallocdoer(on_msg);
			current->lastcmds[0] = (char*)Malloc(strlen(pt) + 45);
			if (CurrentUser)
			{
				sprintf(current->lastcmds[0],"[%s] %-10s %11s[%-3i] (*%s)",
					time2medium(now),command,CurrentUser->name,
					(int)CurrentUser->access,pt);
			}
			else
			{
				sprintf(current->lastcmds[0],"[%s] %-10s %11s[---] (*%s)",
					time2medium(now),command,CurrentNick,pt);
			}
		}

		/*
		 *  CAXS check: first argument might be a channel
		 *              check user access on target channel
		 */
		if (mcmd[i].caxs)
		{
			to = get_channel(to,&msg);
			if (!ischannel(to))
			{
				/*
				 *  this is rare, should the user get a message?
				 */
				if (uaccess) usage_command(from,command);
				return;
			}
			uaccess = get_authlevel(from,to);
			if (uaccess < acmd[i])
				return;
		}
		else
		/*
		 *  GAXS check: user needs global access
		 */
		if (mcmd[i].gaxs)
		{
			uaccess = get_authlevel(from,MATCH_ALL);
			if (uaccess < acmd[i])
				return;
		}

		/*
		 *  dont send zero-length command args
		 */
		if (*msg == 0)
			msg = NULL;

		/*
		 *  CARGS check: at least one argument is required
		 */
		if (mcmd[i].args && !msg)
		{
			if (uaccess) usage_command(from,command);
			return;
		}

#ifdef REDIRECT
		/*
		 *  can this command be redirected?
		 */
		if (mcmd[i].redir)
		{
			if (begin_redirect(from,msg) < 0)
				return;
		}
#endif /* REDIRECT */
		CurrentCmd = &mcmd[i];

		/*
		 *  call the function for the command
		 */
		current_target = target_notice;
		mcmd[i].func(from,to,msg,acmd[i]);

#ifdef DEBUG
		CurrentCmd = NULL;
#endif /* DEBUG */
#ifdef REDIRECT
		end_redirect();
#endif /* REDIRECT */

		/*
		 *  be quick to exit afterwards, there are "dangerous" commands like DIE and DEL (user)
		 */
		return;
	}

	/*
	 *  un-chop() the message string
	 */
	unchop(orig,msg);

	if (CurrentChan)
	{
		common_public(CurrentChan,from,"<%s> %s",orig);
	}
	else
	if (has_cc && *command && uaccess)
	{
		to_user(from,ERR_UNKNOWN_COMMAND);
	}
	else
	if (CurrentDCC)
	{
		partyline_broadcast(CurrentDCC,"<%s> %s\n",orig);
#ifdef BOTNET
		botnet_relay(NULL,"PM* %s@%s %s\n",CurrentNick,current->nick,orig);
#endif /* BOTNET */
	}
	else
	{
		send_spy(SPYSTR_MESSAGE,"<%s> %s",CurrentNick,orig);
  	}
}

void on_mode(char *from, char *channel, char *rest)
{
	Chan	*chan;
	ChanUser *doer;
	ChanUser *victim;
	Shit	*shit;
	char	templimit[20];
	char	*nick;
	char	*parm,*nickuh,*mode;
	int	i,sign,enfm,maxprot;

	if ((chan = find_channel_ac(channel)) == NULL)
		return;
	channel = chan->name;

	if (current->spy.channel)
		send_spy(channel,"*** %s sets mode: %s",CurrentNick,rest);

	maxprot = chan->setting[INT_PROT].int_var;
	sign = '+';

	mode = chop(&rest);

	/*
	 *  might be NULL but we have to handle that due to server modes
	 */
	doer = find_chanuser(chan,from);

modeloop:
	if (*mode == 'o' || *mode == 'v')
	{
		nick = chop(&rest);
		if ((victim = find_chanuser(chan,nick)) == NULL)
		{
			mode++;
			goto modeloop;
		}
	}

	switch(*mode)
	{
	case '+':
	case '-':
		sign = *mode;
		break;
	/*
	 *
	 *  MODE <channel> +/-o <nick>
	 *
	 */
	case 'o':
		i = (victim->user) ? victim->user->access : 0;
/* +o */	if (sign == '+')
		{
			victim->flags |= CU_CHANOP;
			victim->flags &= ~CU_DEOPPED;
			if (!i)
			{
				if (victim->shit || (chan->setting[TOG_SD].int_var && !doer) ||
					chan->setting[TOG_SO].int_var)
				{
					send_mode(chan,60,QM_CHANUSER,'-','o',victim);
				}
			}
			else
			if (!nickcmp(current->nick,nick))
			{
				/*
				 *  wooohoooo! they gave me ops!!!
				 */
				chan->bot_is_op = TRUE;
				if (chan->kickedby)
				{
					if (chan->setting[TOG_RK].int_var)
						send_kick(chan,nickcpy(NULL,chan->kickedby),KICK_REVENGE);
					Free(&chan->kickedby);
				}
				check_shit();
				update_modes(chan);
			}
#ifdef DEBUG
			debug("(on_mode) %s!%s --> %i\n",victim->nick,victim->userhost,i);
#endif /* DEBUG */
		}
/* -o */	else
		{
			victim->flags &= ~(CU_CHANOP|CU_DEOPPED);
			if (i == BOTLEVEL)
			{
				if (!nickcmp(current->nick,nick))
				{
					/*
					 *  they dont love me!!! :~(
					 */
					chan->bot_is_op = FALSE;
				}
			}
			/*
			 *  idiots deopping themselves
			 */
			if (!nickcmp(from,nick))
				break;
			/*
			 *  1. Use enfm var to temporarily store users access
			 *  2. get_userlevel also checks is_localbot()...
			 */
			enfm = (doer && doer->user) ? doer->user->access : 0;
			if (enfm == BOTLEVEL)
				break;
			if (check_mass(chan,doer,CHK_DEOP))
				mass_action(chan,doer);
			if (maxprot && (victim->user && victim->user->prot) && !victim->shit)
			{
				/*
				 *  FIXME: does it matter if the user is logged in or not?
				 */
				nickuh = get_nuh(victim);
				if (get_authlevel(nickuh,channel))
				{
					send_mode(chan,60,QM_CHANUSER,'+','o',victim);
					prot_action(chan,from,doer,NULL,victim);
				}
			}
		}
		break;
	/*
	 *
	 *  MODE <channel> +/-v <nick>
	 *
	 */
	case 'v':
		if (sign == '+')
			victim->flags |= CU_VOICE;
		else
			victim->flags &= ~CU_VOICE;
		break;
	/*
	 *
	 *  MODE <channel> +/-b <parm>
	 *
	 */
	case 'b':
		parm = chop(&rest);
/* +b */	if (sign == '+')
		{
			make_ban(&chan->banlist,from,parm,now);
			if (doer && doer->user && doer->user->access == BOTLEVEL)
				break;
			if (check_mass(chan,doer,CHK_BAN))
				mass_action(chan,doer);
			if (maxprot && get_protuseraccess(chan,parm))
			{
				shit = get_shituser(parm,channel);
				if (!shit || !shit->access)
				{
					/*
					 *  FIXME: do we have a CU for the `from' user? -- yes: doer
					 *  bot_is_op checked: no
					 */
					send_mode(chan,160,QM_RAWMODE,'-','b',parm);
					prot_action(chan,from,doer,parm,NULL);
				}
			}
		}
/* -b */	else
		{
			delete_ban(chan,parm);
			if (!chan->setting[TOG_SHIT].int_var)
				break;
			shit = get_shituser(parm,channel);
			i = (shit) ? shit->access : 0;
			if (i < 3)
			{
				shit = find_shit(parm,channel);
				i = (shit) ? shit->access : 0;
			}
			if (i > 2)
			{
				send_mode(chan,160,QM_RAWMODE,'+','b',shit->mask);
			}
		}
		break;
	case 'p':
	case 's':
	case 'm':
	case 't':
	case 'i':
	case 'n':
		if (reverse_mode(from,chan,*mode,sign))
		{
			send_mode(chan,160,QM_RAWMODE,(sign == '+') ? '-' : '+',*mode,NULL);
		}
		i = (sign == '+');
		switch(*mode)
		{
		case 'p':
			chan->private = i;
			break;
		case 's':
			chan->secret = i;
			break;
		case 'm':
			chan->moderated = i;
			break;
		case 't':
			chan->topprot = i;
			break;
		case 'i':
			chan->invite = i;
			break;
		case 'n':
			chan->nomsg = i;
			break;
		}
		break;
/* k */
	case 'k':
		parm = chop(&rest);
		enfm = reverse_mode(from,chan,'k',sign);
		if (sign == '+')
		{
			chan->keymode = TRUE;
			/*
			 *  Undernet clueless-coder-kludge
			 */
			chan->hiddenkey = (parm) ? FALSE : TRUE;
			if (enfm && parm)
			{
				send_mode(chan,160,QM_RAWMODE,'-','k',parm);
			}
			Free(&chan->key);
			set_mallocdoer(on_mode);
			chan->key = Strdup((parm) ? parm : "???");
		}
		else
		{
			if (enfm && parm)
			{
				send_mode(chan,160,QM_RAWMODE,'+','k',parm);
			}
			chan->keymode = FALSE;
		}
		break;
/* l */
	case 'l':
		if (sign == '+')
		{
			parm = chop(&rest);
			chan->limit = a2i(parm);
			if (errno)
				chan->limit = 0;
			chan->limitmode = TRUE;
		}
		else
		{
			chan->limitmode = FALSE;
		}
		if (reverse_mode(from,chan,'l',sign))
		{
			if (sign == '+')
			{
				send_mode(chan,160,QM_RAWMODE,'-','l',NULL);
			}
			else
			{

				sprintf(templimit,"%i",chan->limit);
				send_mode(chan,160,QM_RAWMODE,'+','l',templimit);
			}
		}
		break;
	case 0:
		return;
	}
	mode++;
	goto modeloop;
}

void common_public(Chan *chan, char *from, char *spyformat, char *rest)
{
	ChanUser *doer;

	if (current->spy.channel)
		send_spy(chan->name,spyformat,CurrentNick,rest);

	if (!chan->bot_is_op)
		return;

	doer = find_chanuser(chan,from);

	if (capslevel(rest) >= 50)
	{
		if (check_mass(chan,doer,CHK_CAPS))
			send_kick(chan,CurrentNick,KICK_CAPS);
	}

	if (chan->setting[TOG_KS].int_var)
	{
		if (!CurrentUser || !CurrentUser->access)
			check_kicksay(chan,rest);
	}

	if (check_mass(chan,doer,CHK_PUB))
	{
		if (chan->setting[INT_FPL].int_var > 1)
			deop_ban(chan,doer,NULL);
		send_kick(chan,CurrentNick,KICK_TEXTFLOOD);
		send_spy(SPYSTR_STATUS,"%s kicked from %s for flooding",from,chan->name);
	}
}

void on_action(char *from, char *to, char *rest)
{
	if (CurrentChan)
	{
		common_public(CurrentChan,from,"* %s %s",rest);
		return;
	}
	if (CurrentDCC)
	{
		partyline_broadcast(CurrentDCC,"* %s %s\n",rest);
#ifdef BOTNET
		botnet_relay(NULL,"PM* %s@%s \001%s\n",CurrentNick,current->nick,rest);
#endif /* BOTNET */
		return;
	}
	if (current->spy.message)
		send_spy(SPYSTR_MESSAGE,"* %s %s",CurrentNick,rest);
}

#ifdef DYNCMD

void do_chaccess(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS + CARGS
	 */
	int	i,oldaccess,newaccess,uaccess,dis;
	char	*name,*axs;

	name = chop(&rest);
	axs = chop(&rest);

	if (!axs && !name)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	dis = FALSE;
	newaccess = a2i(axs);
	if (axs)
	{
		if (!Strcasecmp(axs,"disable"))
		{
			dis = TRUE;
			newaccess = 100;
		}
		else
		{
			if (errno || (newaccess < 0) || (newaccess > OWNERLEVEL))
			{
				to_user(from,"Command access level must be between 0 and %i",OWNERLEVEL);
				return;
			}
		}
	}

	uaccess = get_userlevel(from,NULL);
	if (newaccess > uaccess)
	{
		to_user(from,"Can't change access level to one higher than yours");
		return;
	}
	if (dis && uaccess < OWNERLEVEL)
	{
		to_user(from,"Insufficient access to disable commands");
		return;
	}

	for(i=0;mcmd[i].name;i++)
	{
		if (!Strcasecmp(mcmd[i].name,name))
		{
			oldaccess = acmd[i];
			if (dis || oldaccess > 200)
			{
				to_user(from,"The command \"%s\" has been permanently disabled",name);
				acmd[i] = 250; /* unsigned char, max 255 */
				return;
			}
			if (newaccess == -1)
			{
				to_user(from,"The access level needed for that command is %i",oldaccess);
				to_user(from,"To change it, specify new access level");
				return;
			}
			if (oldaccess > uaccess)
			{
				to_user(from,"Can't change an access level that is higher than yours");
				return;
			}
			if (oldaccess == newaccess)
				to_user(from,"The access level was not changed");
			else
				to_user(from,"Command access level changed from %i to %i",oldaccess,newaccess);
			acmd[i] = newaccess;
			return;
		}
	}
	to_user(from,"Unknown command: %s",name);
}

#endif /* DYNCMD */

int access_needed(char *name)
{
	int	i;

	for(i=0;mcmd[i].name;i++)
	{
		if (!Strcasecmp(mcmd[i].name,name))
		{
			return(acmd[i]);
		}
	}
	return(-1);
}

void do_last(COMMAND_ARGS)
{
	char	*pt,*thenum;
	int	i,num;

	if (!rest || !*rest)
		num = 5;
	else
	{
		thenum = chop(&rest);
		num = a2i(thenum);
	}
	if ((num < 1) || (num > LASTCMDSIZE))
		usage(from);	/* usage for CurrentCmd->name */
	else
	{
		pt = (char*)1;
		to_user(from,TEXT_LASTHDR,num);
		for(i=0;i<num;i++)
		{
			if (!pt)
				break;
			pt = current->lastcmds[i];
			to_user(from,"%s",(pt) ? pt : TEXT_NONE);
		}
	}
}
