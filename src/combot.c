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
#define COMBOT_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"
#include "settings.h"

#ifdef IDWRAP

void unlink_identfile(void)
{
	if (current->identfile)
	{
#ifdef DEBUG
		debug("(unlink_identfile) unlink(%s)\n",current->identfile);
#endif /* DEBUG */
		unlink(current->identfile);
		Free((char**)&current->identfile);
	}
}

#endif /* IDWRAP */

int conf_callback(char *line)
{

	if (line && *line == '#')
		return(FALSE);

	fix_config_line(line);

	on_msg((char*)CoreUser.name,current->nick,line);
	return(FALSE);
}

void readcfgfile(void)
{
	Mech	*bot;
	int	oc,in;

	current = NULL;

	oc = TRUE;
	in = -1;

#ifdef SESSION
	if (!Strcmp(CFGFILE,configfile))
	{
		if ((in = open(SESSIONFILE,O_RDONLY)) >= 0)
		{
			to_file(1,"init: Restoring previously saved session...\n");
			oc = FALSE;
		}
	}
#endif /* SESSION */

	if (oc)
	{
		if ((in = open(configfile,O_RDONLY)) < 0)
		{
			to_file(1,"init: Couldn't open the file %s\n",configfile);
			mechexit(1,exit);
		}
	}

	current = add_bot(0,"(conf)");

	*CurrentNick = 0;
	CurrentShit = NULL;
	CurrentChan = NULL;
	CurrentDCC  = (Client*)&CoreClient;
	CurrentUser = (User*)&CoreUser;

	readline(in,&conf_callback);			/* readline closes in */

	CurrentDCC  = NULL;

	if (!current->guid)
	{
		to_file(1,"init: Error: No bots in the configfile\n");
		mechexit(1,exit);
	}

	if ((current) && (current->chanlist == NULL))
		to_file(1,"%s %s will not join any channels\n",ERR_INIT,current->nick);

	oc = 0;
	to_file(1,"init: Mech(s) added [ ");
	for(bot=botlist;bot;bot=bot->next)
	{
		if (oc > 30)
		{
			to_file(1,", ...");
			break;
		}
		to_file(1,"%s%s",(oc > 0) ? ", " : "",bot->nick);
		oc += strlen(bot->nick);
	}
	to_file(1," ]\n");
}

#ifdef SESSION

int write_session(void)
{
	Server	*sp;
	Chan	*chan;
	Mech	*bot;
	UniVar	*varval;
	int	j,sf;

	if ((sf = open(SESSIONFILE,O_WRONLY|O_CREAT|O_TRUNC,NEWFILEMODE)) < 0)
		return(FALSE);

	to_file(sf,"\n");

	for(bot=botlist;bot;bot=bot->next)
	{
		to_file(sf,"nick %i %s\n",bot->guid,bot->wantnick);
		/*
		 *  current->setting contains channel defaults and global vars
		 */
		for(j=0;VarName[j].name;j++)
		{
			varval = &bot->setting[j];
			if (IsProc(j))
				varval = varval->proc_var;
			if (IsChar(j))
			{
				if ((int)VarName[j].setto != varval->char_var)
					to_file(sf,"set %s %c\n",VarName[j].name,varval->char_var);
			}
			else
			if (IsNum(j))
			{
				if ((int)VarName[j].setto != varval->int_var)
					to_file(sf,"set %s %i\n",VarName[j].name,varval->int_var);
			}
			else
			if (IsStr(j))
			{
				/*
				 *  There are no default string settings
				 */
				if (varval->str_var)
					to_file(sf,"set %s %s\n",VarName[j].name,varval->str_var);
			}
		}
		for(chan=bot->chanlist;chan;chan=chan->next)
		{
			if (!chan->active && !chan->rejoin)
				continue;
			to_file(sf,"join %s %s\n",chan->name,(chan->key) ? chan->key : "");
			/*
			 *  using CHANSET_SIZE: only the first settings contain stuff
			 */
			for(j=0;j<CHANSET_SIZE;j++)
			{
				varval = &chan->setting[j];
				if (IsNum(j))
				{
					if ((int)VarName[j].setto != varval->int_var)
						to_file(sf,"set %s %i\n",VarName[j].name,varval->int_var);
				}
				else
				if (IsStr(j))
				{
					/*
					 *  There are no default string settings
					 */
					if (varval->str_var)
						to_file(sf,"set %s %s\n",VarName[j].name,varval->str_var);
				}
			}
		}
	}

	to_file(sf,"\n");

	for(sp=serverlist;sp;sp=sp->next)
	{
		to_file(sf,"server %s %i %s\n",sp->name,(sp->port) ? sp->port : 6667,
			(sp->pass[0]) ? sp->pass : "");
	}

#ifdef DYNCMD

	to_file(sf,"\n");

	for(j=0;mcmd[j].name;j++)
	{
		if (acmd[j] != mcmd[j].defaultaccess)
			to_file(sf,"chaccess %s %i\n",mcmd[j].name,(int)acmd[j]);
	}

#endif /* DYNCMD */

	close(sf);
	return(TRUE);
}

#endif /* SESSION */

/*
 *  Bot nicking, adding and killing
 */

void setbotnick(Mech *bot, char *nick)
{
	/*
	 *  if its exactly the same we dont need to change it
	 */
	if (!Strcmp(bot->nick,nick))
		return;

	Free((char**)&bot->nick);
	set_mallocdoer(setbotnick);
	bot->nick = Strdup(nick);
#ifdef BOTNET
	botnet_refreshbotinfo();
#endif /* BOTNET */
}

Mech *add_bot(int guid, char *nick)
{
	Mech	*bot;

	set_mallocdoer(add_bot);
	bot = (Mech*)Calloc(sizeof(Mech));
	bot->connect = CN_NOSOCK;
	bot->sock = -1;
	bot->guid = guid;
	set_mallocdoer(add_bot);
	bot->nick = Strdup(nick);
	set_mallocdoer(add_bot);
	bot->wantnick = Strdup(nick);
	set_binarydefault(bot->setting);
	bot->uptime = now;
	bot->next = botlist;
	botlist = bot;

#ifndef I_HAVE_A_LEGITIMATE_NEED_FOR_MORE_THAN_4_BOTS
	spawning_lamer++;
#endif /* I_HAVE_A_LEGITIMATE_NEED_FOR_MORE_THAN_4_BOTS */

	return(bot);
}

void signoff(char *from, char *reason)
{
	Mech	**pp;
	char	*fname;
	int	i;

	if (from)
	{
		to_user(from,"Killing mech: %s",current->nick);
		to_user(from,"Saving the lists...");
	}
	fname = current->setting[STR_USERFILE].str_var;
	if (!write_userlist(fname) && from)
	{
		to_user(from,(fname) ? ERR_NOSAVE : ERR_NOUSERFILENAME,fname);
	}
#ifdef SEEN
	if (seenfile && !write_seenlist() && from)
	{
		to_user(from,TEXT_SEENNOSAVE,seenfile);
	}
#endif /* SEEN */
#ifdef NOTIFY
	if (current->notifylist)
		write_notifylog();
#endif /* NOTIFY */
	if (from)
	{
		to_user(from,"ShutDown Complete");
	}

	while(current->chanlist)
		remove_chan(current->chanlist);

	while(current->clientlist)
		delete_client(current->clientlist);

	while(current->authlist)
		remove_auth(current->authlist);

	purge_shitlist();
	purge_kicklist();

#ifdef NOTIFY
	purge_notify();
#endif /* NOTIFY */

	if (current->sock != -1)
	{
#ifdef IDWRAP
		unlink_identfile();
#endif /* IDWRAP */
		if (!reason)
			reason = randstring(SIGNOFFSFILE);
		to_server("QUIT :%s\n",(reason) ? reason : "");
		killsock(current->sock);
		current->sock = -1;
	}

	/*
	 *  release string var memory
	 */
	delete_vars(current->setting,(SIZE_VARS-1));

#ifdef DEBUG
	debug("(signoff) Removing userlist...\n");
#endif /* DEBUG */
	while(current->userlist)
		remove_user(current->userlist);

#ifdef DEBUG
	debug("(signoff) Removing lastcmd list...\n");
#endif /* DEBUG */
	for(i=0;i<LASTCMDSIZE;i++)
	{
		Free(&current->lastcmds[i]);
	}

	/*
	 *  These 2 are used by do_die() to pass reason and doer.
	 */
	Free((char**)&current->signoff);
	Free((char**)&current->from);

#ifdef DEBUG
	debug("(signoff) Unlinking bot record from linked list...\n");
#endif /* DEBUG */

	pp = &botlist;
	while(*pp)
	{
		if (*pp == current)
		{
			*pp = current->next;
			break;
		}
	}
	Free((char**)&current);

	/*
	 *  get a new current bot, or exit
	 */
	if ((current = botlist) == NULL)
	{
#if defined(BOUNCE) && defined(IDWRAP)
		bounce_cleanup();
#endif /* BOUNCE && IDWRAP */

#ifdef TRIVIA
		write_triviascore();
#endif /* TRIVIA */

#ifdef UPTIME
		uptime_death(UPTIME_GENERICDEATH);
#endif /* UPTIME */

		while(killsock(-2))
			/* killsock() sleeps 1 second in select() */
			;

		mechexit(0,exit);
	}

#ifdef DEBUG
	debug("(signoff) All done.\n");
#endif /* DEBUG */
}

void kill_all_bots(char *reason)
{
	while(TRUE)
	{
		current = botlist;
		signoff(NULL,reason);
	}
}

/*
 *  Server lists, connects, etc...
 */

Server *add_server(char *host, int port, char *pass)
{
	Server	*sp,**pp;

	pp = &serverlist;
	while(*pp)
	{
		sp = *pp;
		if ((sp->port == port) && ((!Strcasecmp(host,sp->name)) || (!Strcasecmp(host,sp->realname))))
			return(sp);
		pp = &sp->next;
	}
	set_mallocdoer(add_server);
	*pp = sp = (Server*)Calloc(sizeof(Server));
	sp->ident = serverident++;
	Strncpy(sp->name,host,NAMELEN);
	if (pass && *pass)
		Strncpy(sp->pass,pass,PASSLEN);
	sp->port = (port) ? port : 6667;
	return(sp);
}

Server *find_server(int id)
{
	Server	*sp;

	for(sp=serverlist;sp;sp=sp->next)
		if (sp->ident == id)
			return(sp);
	return(NULL);
}

int try_server(Server *sp, char *hostname)
{
#ifdef RAWDNS
	char	temphost[NAMEBUF];
	char	*host;
	ulong	ip;
#endif /* RAWDNS */

	if (!hostname)
		hostname = sp->name;
#ifdef RAWDNS
	if ((host = poll_rawdns(hostname)))
	{
#ifdef DEBUG
		debug("(try_server) rawdns: %s ==> %s\n",sp->name,host);
#endif /* DEBUG */
		Strcpy(temphost,host);
		hostname = temphost;
	}
	else
	if ((ip = inet_addr(hostname)) == -1)
	{
		current->server = sp->ident;
		current->connect = CN_DNSLOOKUP;
		current->conntry = now;
		rawdns(hostname);
		return(0);
	}
#endif /* RAWDNS */
	sp->lastattempt = now;
	sp->usenum++;
	current->server = sp->ident;
	if ((current->sock = SockConnect(hostname,sp->port,TRUE)) < 0)
	{
		sp->err = SP_ERRCONN;
		return(-1);
	}
	current->away = FALSE;
	current->connect = CN_TRYING;
	current->activity = current->conntry = now;
	*current->modes = 0;
	return(current->sock);
}

void connect_to_server(void)
{
	Server	*sp,*sptry;
	Chan	*chan;

	/*
	 *  This should prevent the bot from chewing up too
	 *  much CPU when it fails to connect to ANYWHERE
	 */
	current->conntry = now;

	/*
	 *  Is this the proper action if there is no serverlist?
	 */
	if (!serverlist)
		return;

	if (current->chanlist)
	{
#ifdef DEBUG
		debug("[CtS] Setting rejoin- and synced-status for all channels\n");
#endif /* DEBUG */
		current->rejoin = TRUE;
		for(chan=current->chanlist;chan;chan=chan->next)
		{
			if (chan->active)
			{
				chan->active = FALSE;
				chan->rejoin = TRUE;
			}
			chan->sync = TRUE;
			chan->bot_is_op = FALSE;
#ifdef STATS
			if (chan->stats)
				chan->stats->flags |= CSTAT_PARTIAL;
#endif /* STATS */
		}
	}

	if (current->nextserver)
	{
		sp = find_server(current->nextserver);
		current->nextserver = 0;
		if (sp && (try_server(sp,NULL) >= 0))
			return;
	}

	/*
	 *  The purpose of this kludge is to find the least used server
	 */
	sptry = NULL;
	for(sp=serverlist;sp;sp=sp->next)
	{
		if (sp->lastattempt != now)
		{
			if ((!sptry) || (sp->usenum < sptry->usenum))
				sptry = sp;
		}
	}
	/*
	 *  Connect...
	 */
	if (sptry)
		try_server(sptry,NULL);
}

/*
 *  === according to rfc1459 ===
 *     Command: USER
 *  Parameters: <username> <hostname> <servername> <realname>
 */
void register_with_server(void)
{
	Server	*sp;
	char	*ident,*ircname;
	int	sendpass;

#ifdef IRCD_EXTENSIONS
	current->ircx_flags = 0;
#endif /* IRCD_EXTENSIONS */
	sp = find_server(current->server);
	ident = current->setting[STR_IDENT].str_var;
	ircname = current->setting[STR_IRCNAME].str_var;
	sendpass = (sp && *sp->pass);
	to_server(
		(sendpass) ? "PASS :%s\nNICK %s\nUSER %s v2.99.energymech.net 0 :%s\n" :
			"%sNICK %s\nUSER %s v2.99.energymech.net 0 :%s\n",
		(sendpass) ? sp->pass : "",
		current->wantnick,
		(ident) ? ident : BOTLOGIN,
		(ircname) ? ircname : VERSION);
	current->connect = CN_CONNECTED;
	current->conntry = now;
}

/*
 *  scripting stuff
 */

#ifdef SCRIPTING

int sub_compile_timer(int limit, ulong *flags1, ulong *flags2, char *args)
{
	char	*s,*dash;
	ulong	f;
	int	n,hi,lo;

	*flags1 = 0;
	if (flags2) *flags2 = 0;

	if (!args || !*args)
		return -1;

	if (args[0] == '*' && args[1] == 0)
	{
		*flags1 = -1; /*  0-29 = 1 */
		if (flags2) *flags2 = -1; /* 30-59 = 1 */
		return 0;
	}

	/* "n,n,n-m,n-m" */
	for(s=args;*s;s++)
		if (*s == ',')
			*s = ' ';

	/* "n n n-m n-m" */
	do
	{
		s = chop(&args);
		if (s && *s)
		{
			if ((dash = STRCHR(s,'-')))
			{
				*(dash++) = 0;
				if (!*dash)
					return -1;

				lo = a2i(s);
				hi = a2i(dash);

				if (lo < 0 || lo > limit || hi < 0 || hi > limit)
					return -1;
				for(n=lo;n<=hi;n++)
				{
					if (n >= 30)
					{
						f = (1 << (n - 30));
						if (!flags2)
							return -1;
						*flags2 |= f;
					}
					else
					{
						f = (1 << n);
						*flags1 |= f;
					}
				}
			}
			else
			{
				n = a2i(s);
				if (n < 0 || n > limit)
					return -1;
				if (n >= 30)
				{
					f = (1 << (n - 30));
					if (!flags2)
						return -1;
					*flags2 |= f;
				}
				else
				{
					f = (1 << n);
					*flags1 |= f;
				}
			}
		}
	}
	while(s);
	return 0;
}

#if 0

 turn this:
"* * * *"
"0-59 0-59 0-23 0-6"
0,1,2,3,...,58,59 = all seconds
0,1,2,3,...,58,59 = all minutes
0,1,2,3,...,22,23 = all hours
0,1,2,3,4,5,6 = all weekdays

 into this:

typedef struct
{
        time_t  last;
        time_t  next;
        ulong   second1:30;
        ulong   second2:30;
        ulong   minute1:30;
        ulong   minute2:30;
        ulong   hour:24;
        ulong   weekday:7;

} HookTimer;

#endif /* 0 */
/*
 *  return -1 on failure
 */
int compile_timer(HookTimer *timer, char *rest)
{
	char	backup[strlen(rest)+1];
	char	*sec,*min,*hour,*day;

	Strcpy(backup,rest);
	rest = backup;
#ifdef DEBUG
	debug("(compile_timer) rest = %s\n",nullstr(rest));
#endif /* DEBUG */

	sec  = chop(&rest);
	min  = chop(&rest);
	hour = chop(&rest);
	day  = chop(&rest);

	if (sub_compile_timer(59,&timer->second1,&timer->second2,sec) < 0)
		return -1;
	if (sub_compile_timer(59,&timer->minute1,&timer->minute2,min) < 0)
		return -1;
	if (sub_compile_timer(23,&timer->hour,NULL,hour) < 0)
		return -1;
	if (sub_compile_timer(6,&timer->weekday,NULL,day) < 0)
		return -1;
	return 0;
}

#endif /* SCRIPTING */

/*
 *  Periodic updates
 */

void update(SequenceTime *this)
{
	Server	*sp;
	Chan	*chan;
	char	*temp,*chantemp;
	int	tt,th;
	int	x,n;

	tt = now / 600;		/* current 10-minute period */
	th =  tt / 6;		/* current hour */

#ifdef DEBUG
	x = 0;
	if (tt != this->tenminute)
	{
		debug("(update) running: ten minute updates [%i]",tt);
		x++;
	}
	if (th != this->hour)
	{
		debug("%shour updates [%i]",(x) ? ", " : "(update) running: ",th);
		x++;
	}
	if (x)
		debug("\n");
#endif /* DEBUG */

	short_tv &= ~TV_REJOIN;
	for(current=botlist;current;current=current->next)
	{
		if (current->reset || current->connect != CN_ONLINE)
			continue;

		if (current->rejoin)
		{
			if ((now - current->lastrejoin) > REJOIN_DELAY)
			{
				current->rejoin = FALSE;
				current->lastrejoin = now;
			}
			short_tv |= TV_REJOIN;
		}

#ifdef NOTIFY
		send_ison();
#endif /* NOTIFY */

		for(chan=current->chanlist;chan;chan=chan->next)
		{
			if (!chan->active)
			{
				if (!current->rejoin && chan->rejoin)
				{
					/*
					 *  join_channel() will set current->rejoin to TRUE
					 */
					join_channel(chan->name,chan->key);
				}
			}

			if (tt != this->tenminute)
			{
				chan->last10 = chan->this10;
				chan->this10 = 0;
			}
			if (th != this->hour)
			{
				chan->last60 = chan->this60;
				chan->this60 = 0;
#ifdef STATS
				if ((temp = chan->setting[STR_STATS].str_var))
					stats_loghour(chan,temp,this->hour);
#endif /* STATS */
			}
#ifdef DYNAMODE
			if (chan->bot_is_op && chan->setting[STR_DYNLIMIT].str_var)
				check_dynamode(chan);
#endif /* DYNAMODE */
		}
		if ((now - current->lastreset) > RESETINTERVAL)
		{
			current->lastreset = now;
			if (Strcmp(current->nick,current->wantnick))
				to_server("NICK %s\n",current->wantnick);
			check_idlekick();
			if ((x = current->setting[INT_AAWAY].int_var) && current->away == FALSE)
			{
				if ((now - current->activity) > (x * 60))
				{
					temp = randstring(AWAYFILE);
					to_server(AWAYFORM,(temp && *temp) ? temp : "auto-away",time2away(now));
					current->away = TRUE;
				}
			}
		}
		/*
		 *  10 minute update.
		 */
		if (tt != this->tenminute)
		{
			/*
			 *  use `x' to count channels for the status line
			 */
			x = 0;
			for(chan=current->chanlist;chan;chan=chan->next)
			{
				if ((n = chan->setting[INT_AUB].int_var))
				{
					channel_massunban(chan,MATCH_ALL,60*n);
				}
				x++;
			}
			chantemp = (current->activechan) ? current->activechan->name : "(none)";
			if ((sp = find_server(current->server)) != NULL)
			{
				send_spy(SPYSTR_STATUS,"C:%s AC:%i CS:%s:%i",
					chantemp,x,(sp->realname[0]) ? sp->realname : sp->name,sp->port);
			}
			else
			{
				send_spy(SPYSTR_STATUS,"C:%s AC:%i CS:(not in serverlist)",chantemp,x);
			}
		}
		/*
		 *  Hourly update.
		 */
		if (th != this->hour)
		{
			if (current->userlist && current->ul_save)
			{
				temp = current->setting[STR_USERFILE].str_var;
				if (!write_userlist(temp))
					send_spy(SPYSTR_STATUS,(temp) ? ERR_NOSAVE : ERR_NOUSERFILENAME,temp);
				else
					send_spy(SPYSTR_STATUS,TEXT_LISTSAVED,temp);
			}
#ifdef SEEN
			if (seenfile && !write_seenlist())
			{
				send_spy(SPYSTR_STATUS,TEXT_SEENNOSAVE,seenfile);
			}
#endif /* SEEN */
#ifdef NOTIFY
			if (current->notifylist)
				write_notifylog();
#endif /* NOTIFY */
		}
	}

#ifdef SESSION
	if (th != this->hour)
	{
		if (!write_session())
			send_global("Session could not be saved to file %s",SESSIONFILE);
	}
#endif /* SESSION */

	this->tenminute = tt;
	this->hour = th;
}

/*
 *  Read data from server socket
 */
void parse_server_input(void)
{
#ifdef WINGATE
	Server	*sp;
#endif /* WINGATE */
	char	linebuf[MSGLEN];
	char	*res;

	if (FD_ISSET(current->sock,&write_fds))
	{
		setbotnick(current,current->wantnick);
#ifdef DEBUG
		debug("[PSI] {%i} connection established (%s) [ASYNC]\n",
			current->sock,current->wantnick);
#endif /* DEBUG */
#ifdef WINGATE
		if ((current->vhost_type & VH_WINGATE_BOTH) == VH_WINGATE)
		{
			sp = find_server(current->server);
			if (!sp)
			{
#ifdef IDWRAP
				unlink_identfile();
#endif /* IDWRAP */
				close(current->sock);
				current->sock = -1;
				return;
			}
#ifdef DEBUG
			debug("[PSI] Connecting via WinGate proxy...\n");
#endif /* DEBUG */
			to_server("%s %i\n",sp->name,sp->port);
			/*
			 *  If we fail here, we'd better drop the WinGate
			 *  and retry the SAME server again
			 */
			if (current->sock == -1)
			{
				current->nextserver = sp->ident;
				current->vhost_type |= VH_WINGATE_FAIL;
			}
			current->connect = CN_WINGATEWAIT;
			current->conntry = now;
			return;
		}
#endif /* WINGATE */
		/*
		 *  send NICK, USER and maybe PASS
		 */
		register_with_server();
#ifdef IDWRAP
		if (current->sock == -1)
			unlink_identfile();
#endif /* IDWRAP */
		return;
	}
	if (FD_ISSET(current->sock,&read_fds))
	{
get_line:
		res = sockread(current->sock,current->sd,linebuf);
		if (res)
		{
#ifdef WINGATE
			if (current->connect == CN_WINGATEWAIT)
			{
				if (!matches("Connecting to host *Connected",linebuf))
				{
#ifdef DEBUG
					debug("[PSI] WinGate proxy active\n");
#endif /* DEBUG */
					register_with_server();
					return;
				}
				else
				{
#ifdef IDWRAP
					unlink_identfile();
#endif /* IDWRAP */
					close(current->sock);
					current->sock = -1;
					return;
				}
			}
#endif /* WINGATE */
			parseline(linebuf);
			goto get_line;
		}
		switch(errno)
		{
		case EINTR:
		case EAGAIN:
			break;
		default:
#ifdef DEBUG
			debug("[PSI] {%i} errno = %i; closing server socket\n",current->sock,errno);
#endif /* DEBUG */
			*current->sd = 0;
#ifdef IDWRAP
			unlink_identfile();
#endif /* IDWRAP */
			close(current->sock);
			current->sock = -1;
			current->connect = CN_NOSOCK;
			break;
		}
	}
}

/*
 *
 *  commands associated with combot.c
 *
 */

void do_vers(COMMAND_ARGS)
{
	to_user(from,"%s %s",BOTCLASS,VERSION);
}

void do_core(COMMAND_ARGS)
{
	char	tmp[MSGLEN];	/* big buffers at the top */
	Server	*sp;
	Chan	*chan;
	User	*user;
	char	*pt;
	int	i,u,su,bu;

	u = su = bu = 0;
	for(user=current->userlist;user;user=user->next)
	{
		u++;
		if (user->access == OWNERLEVEL)
			su++;
		if (user->access == BOTLEVEL)
			bu++;
	}

	i = Strcmp(current->nick,current->wantnick);
	to_user(from,(i) ? TEXT_CURRNICKWANT : TEXT_CURRNICKHAS,current->nick,current->wantnick);
	to_user(from,TEXT_CURRGUID,current->guid);
	to_user(from,TEXT_USERLISTSTATS,u,su,EXTRA_CHAR(su),bu,EXTRA_CHAR(bu));

	pt = tmp;
	*pt = i = 0;
	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if (*tmp && ((pt - tmp) + strlen(chan->name) > 57))
		{
			to_user(from,(i) ? TEXT_MOREACTIVECHANS : TEXT_ACTIVECHANS,tmp);
			pt = tmp;
			i++;
		}
		if (chan->bot_is_op)
			*(pt++) = '@';
		if (chan == current->activechan)
		{
			*(pt++) = '\037';
			pt = Strcpy(pt,chan->name);
			*(pt++) = '\037';
		}
		else
		{
			pt = Strcpy(pt,chan->name);
		}
		if (chan->next)
			*(pt++) = ' ';
		*pt = 0;
	}
	to_user(from,(i) ? TEXT_MOREACTIVECHANS : TEXT_ACTIVECHANS,(*tmp) ? tmp : TEXT_NONE);

	if (current->setting[STR_VIRTUAL].str_var)
	{
		if ((current->vhost_type & VH_IPALIAS_FAIL) == 0)
			pt = "";
		else
			pt = TEXT_VHINACTIVE;
		to_user(from,TEXT_VIRTHOST,current->setting[STR_VIRTUAL].str_var,pt);
	}
#ifdef WINGATE
	if (current->setting[STR_WINGATE].str_var && current->setting[INT_WINGPORT].int_var)
	{
		if ((current->vhost_type & VH_WINGATE_FAIL) == 0)
			pt = "";
		else
			pt = TEXT_VHINACTIVE;
		to_user(from,TEXT_VIRTHOSTWINGATE,current->setting[STR_WINGATE].str_var,
			current->setting[INT_WINGPORT].int_var,pt); 
	}
#endif /* WINGATE */
	sp = find_server(current->server);
	if (sp)
		to_user(from,TEXT_CURRSERVER,
			(sp->realname[0]) ? sp->realname : sp->name,sp->port);
	else
		to_user(from,TEXT_CURRSERVERNOT);
	to_user(from,TEXT_SERVERONTIME,idle2str(now - current->ontime,FALSE));
	to_user(from,TEXT_BOTMODES,(*current->modes) ? current->modes : TEXT_NONE);
	to_user(from,TEXT_CURRENTTIME,time2str(now));
	to_user(from,TEXT_BOTSTARTED,time2str(uptime));
	to_user(from,TEXT_BOTUPTIME,idle2str(now - uptime,FALSE));
	to_user(from,TEXT_BOTVERSION,VERSION,SRCDATE);
	to_user(from,TEXT_BOTFEATURES,__mx_opts);
}

void do_die(COMMAND_ARGS)
{
	/*
	 *  on_msg checks GAXS
	 */
	char	*reason;

#ifdef SESSION
	write_session();
#endif /* SESSION */

	if (!rest || !*rest)
	{
		if ((reason = randstring(SIGNOFFSFILE)) == NULL)
			reason = "I'll get you for this!!!";
	}
	else
	{
		reason = rest;
	}

	set_mallocdoer(do_die);
	current->signoff = Strdup(reason);
	set_mallocdoer(do_die);
	current->from = Strdup(from);

	current->connect = CN_BOTDIE;
}

void do_shutdown(COMMAND_ARGS)
{
	send_global(TEXT_SHUTDOWNBY,nickcpy(NULL,from));

#ifdef SESSION
	write_session();
#endif /* SESSION */

	kill_all_bots(NULL);
	/* NOT REACHED */
}

void do_addserver(COMMAND_ARGS)
{
	char	*host,*aport,*pass;
	int	port;

	if (strlen(rest) >= MAXHOSTLEN-1)
	{
		to_user(from,TEXT_NAMETOOLONG);
		return;
	}
	to_user(from,TEXT_ADDTOSERVLIST,rest);

	host = chop(&rest);
	if (!host && !*host)
		return;
	aport = chop(&rest);
	pass = chop(&rest);
	if (aport && *aport == '#')
	{
		aport = pass = NULL;
	}
	if (pass && *pass == '#')
	{
		pass = NULL;
	}
	port = a2i(aport);
	if (errno)
		port = 6667;
	add_server(host,port,pass);
}

void do_delserver(COMMAND_ARGS)
{
	Server	*sp,*dp,**spp;
	char	*name,*port;
	int	n,iport;

	if (!serverlist)
	{
		to_user(from,TEXT_EMPTYSERVLIST);
		return;
	}
	name = chop(&rest);
	port = chop(&rest);
	iport = a2i(port);
	if (errno)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}
	if (!port)
		iport = 0;
	n = 0;
	dp = NULL;
	for(sp=serverlist;sp;sp=sp->next)
	{
		if ((!Strcasecmp(name,sp->name)) || (!Strcasecmp(name,sp->realname)))
		{
			if (iport)
			{
				if (sp->port == iport)
				{
					dp = sp;
					n++;
				}
			}
			else
			{
				dp = sp;
				n++;
			}
		}
	}
	switch(n)
	{
	case 0:
		to_user(from,(iport) ? TEXT_NOSERVMATCHP : TEXT_NOSERVMATCH,name,iport);
		break;
	case 1:
		to_user(from,TEXT_SERVERDELETED,name,dp->port);
		for(spp=&serverlist;*spp;spp=&(*spp)->next)
		{
			if (*spp == dp)
			{
				*spp = dp->next;
				Free((void*)&dp);
				break;
			}
		}
		break;
	default:
		to_user(from,TEXT_MANYSERVMATCH,name);
	}
}

void do_server(COMMAND_ARGS)
{
	Server	*sp;
	char	*server,*aport;
	char	*last;
	int	iport;

	if (!rest || !*rest)
	{
		if (!CurrentDCC)
		{
			dcc_chat(from);
			return;
		}
		for(sp=serverlist;sp;sp=sp->next)
		{
			if (sp->lastconnect)
				last = idle2str(now - sp->lastconnect,FALSE);
			else
			{
				switch(sp->err)
				{
				case SP_NOAUTH:
					last = TEXT_SP_NOAUTH;
					break;
				case SP_KLINED:
					last = TEXT_SP_KLINED;
					break;
				case SP_FULLCLASS:
					last = TEXT_SP_FULLCLASS;
					break;
				case SP_TIMEOUT:
					last = TEXT_SP_TIMEOUT;
					break;
				case SP_ERRCONN:
					last = TEXT_SP_ERRCONN;
					break;
				case SP_DIFFPORT:
					last = TEXT_SP_DIFFPORT;
					break;
				default:
					last = TEXT_NEVER;
				}
			}
			to_user(from,"Server: %s:%i",
				(*sp->realname) ? sp->realname : sp->name,sp->port);
			to_user(from,"Connect: %s%s%s",last,
				(sp->lastconnect) ? TEXT_AGO : "",
				(sp->ident == current->server) ? TEXT_CURRENT : "");
		}
		return;
	}

	server = chop(&rest);
	aport = chop(&rest);

	if (!server || !*server || (strlen(server) >= MAXHOSTLEN))
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}
	iport = a2i(aport);
	if (errno || iport <= 0 || iport >= 65535)
		iport = 6667;
	if ((sp = add_server(server,iport,NULL)) == NULL)
	{
		to_user(from,"Problem switching servers");
		return;
	}
	if (from == CoreUser.name)
		return;
	current->nextserver = sp->ident;
	to_user(from,"Trying new server: %s on port %i",server,iport);
	switch(current->connect)
	{
	case CN_CONNECTED:
	case CN_ONLINE:
		to_server("QUIT :Trying new server, brb...\n");
		killsock(current->sock);
		break;
	default:
		if (current->sock != -1)
			close(current->sock);
	}
	current->sock = -1;
}

void do_cserv(COMMAND_ARGS)
{
	Server	*sp;

	if ((sp = find_server(current->server)))
		to_user(from,TEXT_CSERV,(*sp->realname) ? sp->realname : sp->name,sp->port);
	else
		to_user(from,TEXT_CSERVNOT);
}

void do_away(COMMAND_ARGS)
{
	if (!rest || !*rest)
	{
		to_server("AWAY\n");
		to_user(from,TEXT_NOLONGERAWAY);
		current->away = FALSE;
		current->activity = now;
		return;
	}
	to_server(AWAYFORM,rest,time2away(now));
	to_user(from,TEXT_NOWSETAWAY);
	current->away = TRUE;
}
