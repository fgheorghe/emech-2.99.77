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
#define COMMAND_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"
#include "settings.h"

void ec_access(ESAY_ARGS)
{
	sprintf(*ec_dest,"%i",get_userlevel(from,to));
	*ec_dest = STREND(*ec_dest);
}

void ec_cc(ESAY_ARGS)
{
	*ec_dest = Strcpy(*ec_dest,(current->activechan) ? current->activechan->name : TEXT_NONE);
}

void ec_channels(ESAY_ARGS)
{
	Chan	*chan;
	int	n;

	if ((chan = current->chanlist) == NULL)
	{
		*ec_dest = Strcpy(*ec_dest,ERR_NOCHANNELS);
		return;
	}
	for(n=0;chan;chan=chan->next)
	{
		if (chan->active)
		{
			if (n)
				*((*ec_dest)++) = ' ';
			if (chan->bot_is_op)
				*((*ec_dest)++) = '@';
			*ec_dest = Strcpy(*ec_dest,chan->name);
		}
	}
}

void ec_time(ESAY_ARGS)
{
	*ec_dest = Strcpy(*ec_dest,time2away(now));
}

void ec_set(ESAY_ARGS)
{
	Chan	*chan;
	UniVar	*varval;
	char	*src;
	int	which;

	src = *ec_src;
	while(*src && *src != ')')
		src++;
	if (*src != ')')
		return;
	*(src++) = 0;

	if ((which = find_setting(*ec_src)) >= 0)
	{
		if (which >= CHANSET_SIZE)
			varval = &current->setting[which];
		else
		if ((chan = find_channel_ny(to)))
			varval = &chan->setting[which];
		else
		{
			*ec_dest = Strcpy(*ec_dest,"(unknown channel)");
			return;
		}

		if (IsProc(which))
			varval = varval->proc_var;

		if (IsChar(which))
		{
			*((*ec_dest)++) = varval->char_var;
		}
		else
		if (IsInt(which))
		{
			sprintf(*ec_dest,"%i",varval->int_var);
			*ec_dest = STREND(*ec_dest);
		}
		else
		if (IsTog(which))
		{
			*ec_dest = Strcpy(*ec_dest,(varval->int_var) ? "on" : "off");
		}
		else
		if (IsStr(which))
		{
			*ec_dest = Strcpy(*ec_dest,nullstr(varval->str_var));
		}
	}
	else
	{
		*ec_dest = Strcpy(*ec_dest,"(unknown setting)");
	}
	*ec_src = src;
}

void ec_on(ESAY_ARGS)
{
	*ec_dest = Strcpy(*ec_dest,idle2str(now - current->ontime,FALSE));
}

void ec_server(ESAY_ARGS)
{
	Server	*sv;
	char	*s;

	if ((sv = find_server(current->server)))
		s = (*sv->realname) ? sv->realname : sv->name;
	else
		s = TEXT_NOTINSERVLIST;
	*ec_dest = Strcpy(*ec_dest,s);
}

void ec_up(ESAY_ARGS)
{
	*ec_dest = Strcpy(*ec_dest,idle2str(now - current->uptime,FALSE));
}

void ec_ver(ESAY_ARGS)
{
	*ec_dest = Strcpy(*ec_dest,BOTCLASS);
	*((*ec_dest)++) = ' ';
	*ec_dest = Strcpy(*ec_dest,VERSION);
}

LS const struct
{
	char	len;
	void	(*func)(ESAY_TYPEARGS);
	char	name[12];

} ecmd[] =
{
	{ 7,	ec_access,	"$access"	},
	{ 3,	ec_cc,		"$cc"		},
	{ 9,	ec_channels,	"$channels"	},
	{ 5,	ec_time,	"$time"		},
	{ 5,	ec_set,		"$var("		},
	{ 3,	ec_on,		"$on"		},
	{ 7,	ec_server,	"$server"	},
	{ 3,	ec_up,		"$up"		},
	{ 4,	ec_ver,		"$ver"		},
	{ 0,	NULL,		""		},
};

void do_esay(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS + CARGS
	 */
	char	output[MAXLEN];
	uchar	c,*chp;
	char	*ec_src;
	char	*ec_dest;
	int	i,n;

	if (find_channel_ac(to) == NULL)
	{
		to_user(from,ERR_CHAN,to);
		return;
	}

	ec_src = rest;
	rest = STREND(rest);
	ec_dest = output;
	c = 0;
	chp = NULL;

	while(*ec_src)
	{
		if (*ec_src != '$')
		{
			*(ec_dest++) = *(ec_src++);
			continue;
		}

		for(i=0;ecmd[i].len;i++)
		{
			if ((rest - ec_src) >= ecmd[i].len)
			{
				chp = ec_src + ecmd[i].len;
				c = *chp;
				*chp = 0;
			}
			n = Strcasecmp(ecmd[i].name,ec_src);
			if (c)
			{
				*chp = c;
				c = 0;
			}
			if (!n)
			{
				ec_src += ecmd[i].len;
				ecmd[i].func(&ec_dest,&ec_src,from,to);
#ifdef DEBUG
				debug("(do_esay) %s\n",output);
#endif /* DEBUG */
				break;
			}
		}

		if (!ecmd[i].len)
		{
			*(ec_dest++) = *(ec_src++);
		}
	}
	*ec_dest = 0;
	sendprivmsg(to,"%s",output);
}

void do_access(COMMAND_ARGS)
{
	char	*chan,*nuh;
	int	level;

	chan = get_channel(to,&rest);
	if (rest && ((nuh = chop(&rest))))
	{
		if (*nuh == current->setting[CHR_CMDCHAR].char_var)
		{
			nuh++; 
			level = access_needed(nuh);
			if (level < 0)
				return;
			else
			if (level > 200)
				to_user(from,"The command \"%s\" has been disabled",nuh);
			else
				to_user(from,"The access level needed for \"%s\" is: %i",nuh,level);
			return;
		}
		if ((nuh = nick2uh(from,nuh)) == NULL)
			return;
	}
	else
	{
		nuh = from;
	}
	to_user(from,"Immortality Level for %s",nuh);
	to_user(from,"Channel: %s  Access: %i",chan,get_userlevel(nuh,chan));
}

void do_time(COMMAND_ARGS)
{
	char	*ts;

	ts = time2away(now);
	if (ischannel(to))
	{
		sendprivmsg(to,"Current time is: %s",ts);
	}
	else
	{
		to_user(from,"Current time is: %s",ts);
	}
}

void do_uptime_ontime(COMMAND_ARGS)
{
	int	yes;

	yes = (CurrentCmd->name == C_UPTIME);

	to_user(from,(yes) ? "Uptime: %s" : "Ontime: %s",
		idle2str(now - ((yes) ? current->uptime : current->ontime),FALSE));
}

void do_cchan(COMMAND_ARGS)
{
	Chan	*chan;
	char	*channel;

	if (rest)
	{
		channel = chop(&rest);
		if ((chan = find_channel_ac(channel)) != NULL)
		{
			current->activechan = chan;
			to_user(from,"Current channel set to %s",chan->name);
		}
		else
			to_user(from,ERR_CHAN,channel);
		return;
	}
	to_user(from,"Current channel: %s",
		(current->activechan) ? current->activechan->name : "(none)");
}

void do_opme_deopme(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS
	 */
	Chan	*chan;
	ChanUser *cu;

	if ((chan = find_channel_ac(to)) && chan->bot_is_op)
	{
		if ((cu = find_chanuser(chan,from)))
		{
			send_mode(chan,80,QM_CHANUSER,
				(CurrentCmd->name == C_DOWN) ? '-' : '+','o',cu);
		}
	}
}

void do_op_voice(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS
	 */
	Chan	*chan;

	if ((chan = find_channel_ac(to)) && chan->bot_is_op)
	{
		if (!rest)
			rest = CurrentNick;

		if (CurrentCmd->name == C_VOICE)
			channel_massmode(chan,rest,MODE_FORCE,'+','v');
		else
			channel_massmode(chan,rest,CU_CHANOP,'+','o');
	}
}

void do_deop_unvoice(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CARGS + CAXS
	 */
	Chan	*chan;

	if ((chan = find_channel_ac(to)) && chan->bot_is_op)
	{
		if (CurrentCmd->name == C_UNVOICE)
			channel_massmode(chan,rest,MODE_FORCE,'-','v');
		else
			channel_massmode(chan,rest,CU_CHANOP,'-','o');
	}
}

void do_invite(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS
	 */
	char	*nick;

	if ((find_channel_ac(to)) == NULL)
	{
		to_user(from,ERR_CHAN,to);
		return;
	}

	nick = (rest) ? rest : CurrentNick;

	while(nick && *nick)
		to_server("INVITE %s %s\n",chop(&nick),to);

	to_user(from,"User(s) invited to %s",to);
}
			
void do_say(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CARGS + CAXS
	 */
	sendprivmsg(to,"%s",rest);
}

void do_me(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CARGS + CAXS
	 */
	to_server("PRIVMSG %s :\001ACTION %s\001\n",to,rest);
}

void do_msg(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Client	*client;
	char	*nick;

	nick = chop(&rest);
	if (ischannel(nick))
	{
		/*
		 *  need to check channel access
		 */
		if (get_authlevel(from,nick) < cmdaccess)
			return;
	}

	if (*nick == '=')
	{
		nick++;
		if ((client = find_client(nick)) == NULL)
		{
			to_user(from,"I have no DCC connection to %s",nick);
			return;
		}
		if (to_file(client->sock,"%s\n",rest) < 0)
			client->flags = DCC_DELETE;
	}
	else
	{
		sendprivmsg(nick,"%s",rest);
	}
	to_user(from,"Message sent to %s",nick);
}


void do_do(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS + GAXS
	 */
	to_server("%s\n",rest);
}

void do_nick(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS + CARGS
	 */
	Mech	*backup;
	char	*nick;
	int	guid;

	nick = chop(&rest);
	if (!nick || !*nick)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}
	guid = a2i(nick);
	backup = current;	
	if (!errno)
	{
		nick = chop(&rest);
		for(current=botlist;current;current=current->next)
		{
			if (current->guid == guid)
				break;
			if (current->guid == 0)
				break;
		}
	}
	if (!isnick(nick))
	{
		current = backup;
		to_user(from,ERR_NICK,nick);
		return;
	}
	if (!current)
	{
#ifdef DEBUG
		debug("(do_nick) Adding new bot: guid = %i; nick = %s\n",guid,nick);
#endif /* DEBUG */
		current = add_bot(guid,nick);
		if (!sigmaster)
			sigmaster = guid;
		if (from == CoreUser.name)
			return;
	}
	else
	{
		if (current->guid == 0)
		{
			Free((char**)&current->nick);
			set_mallocdoer(do_nick);
			current->nick = Strdup(nick);
			current->guid = guid;
		}
		Free((char**)&current->wantnick);
		set_mallocdoer(do_nick);
		current->wantnick = Strdup(nick);
		to_server("NICK %s\n",current->wantnick);
	}
	current = backup;
}

void do_ban_siteban(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Chan	*chan;
	ChanUser *victim;
	char	*channel,*nick,*nuh;
	int	ul;

	channel = get_channel(to,&rest);
	if ((ul = get_authlevel(from,channel)) < cmdaccess)
		return;

	if ((nick = chop(&rest)) == NULL)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	if ((chan = find_channel_ac(channel)) == NULL)
		return;

	/*
	 *  convert a mere nickname into a full nick!user@host
	 */
	if ((victim = find_chanuser(chan,nick)) == NULL)
		return;
	nuh = get_nuh(victim);

	if (CurrentCmd->name == C_BAN)
	{
		if (victim->user && victim->user->access > ul)
		{
			send_kick(chan,CurrentNick,"Ban attempt of %s",nuh);
			return;
		}
		deop_ban(chan,victim,NULL);
		to_user(from,"%s banned on %s",nick,channel);
	}
	else
	{
		if (victim->user && victim->user->access > ul)
		{
			send_kick(chan,CurrentNick,"Siteban attempt of %s",nuh);
			return;
		}
		deop_siteban(chan,victim);
		to_user(from,"%s sitebanned on %s",nick,channel);
	}
}

void do_kickban(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Chan	*chan;
	ChanUser *victim;
	char	*channel,*nick,*nuh;
	int	uaccess;

	channel = get_channel(to,&rest);
	if ((uaccess = get_authlevel(from,channel)) < cmdaccess)
		return;

	if ((nick = chop(&rest)) == NULL)
	{
		usage(from);
		return;
	}

	if ((chan = find_channel_ac(channel)) == NULL)
	{
		if (!CurrentChan) to_user(from,ERR_CHAN,channel);
		return;
	}

	/*
	 *  is the bot opped on that channel?
	 */
	if (!chan->bot_is_op)
	{
		if (!CurrentChan) to_user(from,ERR_NOTOPPED,channel);
		return;
	}

	if ((victim = find_chanuser(chan,nick)) == NULL)
	{
		if (!CurrentChan) to_user(from,ERR_NOTOPPED,channel);
		return;
	}

	if (*rest == 0)
	{
		if ((rest = randstring(RANDKICKSFILE)) == NULL)
			rest = "Requested Kick";
	}

	if (!victim->user || victim->user->access < uaccess)
		uaccess = 0;

	if (CurrentCmd->name == C_KB)
	{
		if (uaccess)
		{
			nuh = get_nuh(victim);
			send_kick(chan,CurrentNick,"Kickban attempt of %s",nuh);
			return;
		}
		deop_ban(chan,victim,NULL);
		send_kick(chan,nick,"%s",rest);
		to_user(from,"%s kickbanned on %s",nick,channel);
	}
	else
	if (CurrentCmd->name == C_SITEKB)
	{
		if (uaccess)
		{
			nuh = get_nuh(victim);
			send_kick(chan,CurrentNick,"Sitekickban attempt of %s",nuh);
			return;
		}
		deop_siteban(chan,victim);
		send_kick(chan,nick,"%s",rest);
		to_user(from,"%s sitekickbanned on %s",nick,channel);
	}
	else
	/* if (CurrentCmd->name == C_SCREW) */
	{
		if (uaccess)
		{
			nuh = get_nuh(victim);
			send_kick(chan,CurrentNick,"Screwban attempt of %s",nuh);
			return;
		}
		deop_screwban(chan,victim);
		send_kick(chan,nick,"%s",rest);
		to_user(from,"%s screwbanned on %s",nick,channel);
	}
}

void do_kick(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Chan	*chan;
	char	*channel,*nick,*nuh;
	int	uaccess;

	channel = get_channel(to,&rest);
	if ((uaccess = get_authlevel(from,channel)) < cmdaccess)
		return;

	if ((nick = chop(&rest)) == NULL)
	{
		usage(from);
		return;
	}

	if ((chan = find_channel_ac(channel)) == NULL)
		/* FIXME: be verbose */
		return;

	if ((nuh = nick2uh(from,nick)) == NULL)
		return;

	if (uaccess < get_userlevel(nuh,channel))
	{
		send_kick(chan,CurrentNick,"Kick attempt of %s",nuh);
		return;
	}
	send_kick(chan,nick,"%s",(rest) ? rest : randstring(RANDKICKSFILE));
	to_user(from,"%s kicked on %s",nick,channel);
}

void do_showusers(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CAXS
	 */
	Chan	*chan;
	ChanUser *cu;
	char	*pattern,*nuh;
	char	modechar,thechar;
	int	umode,ul,flags;

	flags = 0;
	pattern = nuh = NULL;
	if (rest)
	{
		pattern = chop(&rest);
		if (*pattern == '-')
		{
			nuh = pattern;
			pattern = chop(&rest);
		}
		else
		if (*rest == '-')
			nuh = chop(&rest);

		if (nuh)
		{
			if (!Strcasecmp(nuh,"-ops"))
				flags = 1;	
			else
			if (!Strcasecmp(nuh,"-nonops"))
				flags = 2;
			else
			{
				usage(from);	/* usage for CurrentCmd->name */
				return;
			}
		}
	}

	if ((chan = find_channel_ny(to)) == NULL)
	{
		to_user(from,"I have no information on %s",to);
		return;
	}
	to_user(from,"\037Users on %s%s\037",to,(chan->active) ? "" : " (from memory)");

	thechar = 0;
	if (chan->users)
	{
		for(cu=chan->users;cu;cu=cu->next)
		{
			umode = cu->flags;

			if ((flags == 1) && !(umode & CU_CHANOP))
				continue;
			if ((flags == 2) && (umode & CU_CHANOP))
				continue;

			nuh = get_nuh(cu);
			if (pattern && matches(pattern,nuh))
				continue;

			modechar = (umode & CU_CHANOP) ? '@' : ((umode & CU_VOICE) ? '+' : ' ');

			thechar = 'u';
			ul = get_userlevel(nuh,to);
			if (!ul)
			{
				if ((ul = get_shitlevel(nuh,to)) != 0)
					thechar = 's';
			}
			else
			if (ul == 200)
				thechar = 'b';

			to_user(from,"%4i%c   %c %9s %s",ul,thechar,modechar,cu->nick,cu->userhost);
		}
		if (!thechar)
			to_user(from,"No matching users found");
	}
}

void do_unban(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CAXS
	 */
	Ban	*ban;
	Chan	*chan;
	char	*nick,*nuh;

	nick = (rest) ? chop(&rest) : NULL;

	if (((chan = find_channel_ac(to)) == NULL) || !chan->bot_is_op)
		return;

	if (nick && STRCHR(nick,'*'))
	{
		channel_massunban(chan,nick,0);
		return;
	}

	if (!nick)
	{
		to_user(from,"You have been unbanned on %s",to);
		nuh = from;
	}
	else
	{
		if ((nuh = nick2uh(from,nick)) == NULL)
			return;
		to_user(from,"%s unbanned on %s",nuh,to);
	}

	for(ban=chan->banlist;ban;ban=ban->next)
	{
		if (!matches(ban->banstring,nuh))
		{
			send_mode(chan,90,QM_RAWMODE,'-','b',(void*)ban->banstring);
		}
	}
}

void do_topic(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CARGS + CAXS
	 */
	Chan	*chan;

	if ((chan = find_channel_ac(to)) == NULL)
	{
		to_user(from,ERR_CHAN,to);
		return;
	}
	if (chan->bot_is_op || !chan->topprot)
	{
		to_server("TOPIC %s :%s\n",to,rest);
		to_user(from,"Topic changed on %s",to);
		return;
	}
	to_user(from,ERR_NOTOPPED,to);
}

void do_nextserver(COMMAND_ARGS)
{
	to_user(from,"Switching servers...");
	switch(current->connect)
	{
	case CN_CONNECTED:
	case CN_ONLINE:
		to_server("QUIT :Switching servers...\n");
		killsock(current->sock);
		break;
	default:
		if (current->sock != -1)
			close(current->sock);
	}
	current->sock = -1;
}

void do_banlist(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS
	 */
	Ban	*ban;
	Chan	*chan;

	if ((chan = find_channel_ac(to)) == NULL)
	{
		to_user(from,ERR_CHAN,to);
		return; 
	}

	if (chan->banlist)
	{
		to_user(from,"\037Banlist on %s\037",to);
		to_user(from,"\037Ban pattern\037%30s","\037Set by\037");
		for(ban=chan->banlist;ban;ban=ban->next)
			to_user(from,"%-30s   %s",ban->banstring,ban->bannedby);
	}
	else
		to_user(from,"There are no active bans on %s",to);
}

void do_showidle(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS
	 */
	ChanUser *cu;
	Chan	*chan;
	char	*secs;
	int	n;

	if ((chan = find_channel_ac(to)) == NULL)
	{
		to_user(from,ERR_CHAN,to);
		return;
	}

	n = 10;
	if (rest && (secs = chop(&rest)))
	{
		n = a2i(secs);
		if (errno)
		{
			usage(from);	/* usage for CurrentCmd->name */
			return;
		}
	}

	to_user(from,"Users on %s that are idle more than %i seconds:",chan->name,n);
	for(cu=chan->users;cu;cu=cu->next)
	{
		if (n >= (now - cu->idletime))
			continue;
		to_user(from,"%s: %s!%s",idle2str((now - cu->idletime),TRUE),cu->nick,cu->userhost);
        }
	to_user(from," ");
}

void do_idle(COMMAND_ARGS)
{
	time_t	when;

	when = get_idletime(rest);
	if (when == -1)
	{
		to_user(from,"That user is not on any of my channels");
		return;
	}
	to_user(from,"%s has been idle for %s",rest,idle2str(now - when,FALSE));
}

void do_save(COMMAND_ARGS)
{
	char	*fname;

	fname = current->setting[STR_USERFILE].str_var;
	current->ul_save = 1;
	if (!write_userlist(fname))
	{
		to_user(from,(fname) ? ERR_NOSAVE : ERR_NOUSERFILENAME,fname);
	}
	else
	{
		to_user(from,TEXT_LISTSAVED,fname);
	}
#ifdef SEEN
	if (seenfile && !write_seenlist())
	{
		to_user(from,TEXT_SEENNOSAVE,seenfile);
	}
#endif /* SEEN */
#ifdef NOTIFY
	if (current->notifylist)
		write_notifylog();
#endif /* NOTIFY */
#ifdef TRIVIA
	write_triviascore();
#endif /* TRIVIA */
}

void do_load(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS
	 */
	char	*fname;

	fname = current->setting[STR_USERFILE].str_var;
	if (!read_userlist(fname))
	{
		to_user(from,(fname) ? ERR_NOREAD : ERR_NOUSERFILENAME,fname);
	}
	else
	{
		to_user(from,TEXT_LISTREAD,fname);
	}
#ifdef SEEN
	if (seenfile && !read_seenlist())
	{
		to_user(TEXT_SEENNOLOAD,seenfile);
	}
#endif /* SEEN */
#ifdef TRIVIA
	read_triviascore();
#endif /* TRIVIA */
}

void do_names(COMMAND_ARGS)
{
	ChanUser *cu;
	Chan	*chan;
	char	names[MSGLEN];
	char	*p;

	p = get_channel(to,&rest);
	if ((chan = find_channel_ny(p)) == NULL)
	{
		to_user(from,ERR_CHAN,p);
		return;
	}

	to_user(from,"Names on %s%s:",p,(chan->active) ? "" : " (from memory)");
	for(cu=chan->users;cu;)
	{
		p = names;

		while(cu && ((p - names) < 60))
		{
			if (p > names)
				*(p++) = ' ';

			if ((cu->flags) & CU_CHANOP)
				*(p++) = '@';
			else
			if ((cu->flags) & CU_VOICE)
				*(p++) = '+';

			p = Strcpy(p,cu->nick);
			cu = cu->next;
		}

		if (p > names)
		{
			*p = 0;
			to_user(from,"%s",names);
		}
	}
}

