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

#ifdef MD5CRYPT
#define md5banneropt " MD5"
char *crypt(char *, char *);
#else
#define md5banneropt
#endif /* MD5CRYPT */

/*
 *  this is a partial copy of the BotNet struct
 */
LS struct
{
	struct	BotNet *next;

	int	sock;
	int	status;
	int	has_data;

} linksock = { NULL, -1, BN_LINKSOCK, 0 };

/*
 *  command lists
 */
typedef struct LinkCmd
{
	char	command;
	void	(*func)(BotNet *, char *);

} LinkCmd;

LS const LinkCmd basicProto[] =
{
{	'A',	basicAuth	},
{	'B',	basicBanner	},
{	'K',	basicAuthOK	},
{	'L',	basicLink	},
{	'Q',	basicQuit	},
{	0,	NULL		},
};

LS const LinkCmd chanProto[] = 
{
{	0,	NULL		},
};

LS const LinkCmd partyProto[] = 
{
{	'M',	partyMessage	},
{	0,	NULL		},
};

LS const LinkCmd ushareProto[] = 
{
{	'T',	ushareTick	},
{	'U',	ushareUser	},
{	0,	NULL		},
};

LS int deadlinks = FALSE;

/*
 *
 *  misc
 *
 */

Mech *get_netbot(void)
{
	Mech	*netbot,*bot;
	int	uid;

	netbot = NULL;
	uid = INT_MAX;
	for(bot=botlist;bot;bot=bot->next)
	{
		if (bot->guid < uid)
		{
			uid = bot->guid;
			netbot = bot;
		}
	}
	return(netbot);
}

void reset_linkable(int guid)
{
	NetCfg	*cfg;

	for(cfg=netcfglist;cfg;cfg=cfg->next)
	{
		if (cfg->guid == guid)
		{
#ifdef DEBUG
			debug("(reset_linkable) guid %i reset to linkable\n",guid);
#endif /* DEBUG */
			cfg->linked = FALSE;
			return;
		}
	}
}

void botnet_deaduplink(BotNet *bn)
{
	bn->status = BN_DEAD;
	deadlinks = TRUE;
	reset_linkable(bn->guid);
}

NetCfg *find_netcfg(int guid)
{
	NetCfg	*cfg;

	for(cfg=netcfglist;cfg;cfg=cfg->next)
	{
		if (cfg->guid == guid)
			return(cfg);
	}
	return(NULL);
}

BotInfo *make_botinfo(int guid, int hops, char *nuh, char *server, char *version)
{
	BotInfo	*new;

	set_mallocdoer(make_botinfo);
	new = (BotInfo*)Calloc(sizeof(BotInfo) + strlen(nuh) + strlen(server) + strlen(version));

	new->guid = guid;
	new->hops = hops;

	new->server = Strcat(new->nuh,nuh) + 1;
	new->version = Strcat(new->server,server) + 1;
	Strcpy(new->version,version);

	return(new);
}

void botnet_relay(BotNet *source, char *format, ...)
{
	BotNet	*bn;
	va_list msg;
	int	sz;

	sz = 0;

	for(bn=botnetlist;bn;bn=bn->next)
	{
		if ((bn == source) || (bn->status != BN_LINKED))
			continue;

		if (!sz)
		{
			va_start(msg,format);
			vsprintf(sock_buf,format,msg);
			va_end(msg);
			sz = strlen(sock_buf);
		}

		if (write(bn->sock,sock_buf,sz) < 0)
			botnet_deaduplink(bn);
	}
}

void botnet_refreshbotinfo(void)
{
	Server	*sv;

	sv = find_server(current->server);
	botnet_relay(NULL,"BL%i 0 %s!%s %s:%i %s %s\n",
		current->guid,current->nick,
		(current->userhost) ? current->userhost : "unknown@unknown",
		(sv) ? ((*sv->realname) ? sv->realname : sv->name) : "unknown",
		(sv) ? sv->port : 0,BOTCLASS,VERSION);
#ifdef DEBUG
	debug("(botnet_refreshbotinfo) sent refreshed information to botnet\n");
#endif /* DEBUG */
}

void botnet_BL(int sock, BotInfo *binfo)
{
	to_file(sock,"BL%i %i %s %s %s\n",
		binfo->guid,
		(binfo->hops + 1),
		(binfo->nuh) ? binfo->nuh : "unknown@unknown",
		(binfo->server) ? binfo->server : "unknown",
		(binfo->version) ? binfo->version : "-"
		);
}

void botnet_relayBL(BotNet *source, BotInfo *binfo)
{
	botnet_relay(source,"BL%i %i %s %s %s\n",
		binfo->guid,
		(binfo->hops + 1),
		(binfo->nuh) ? binfo->nuh : "unknown@unknown",
		(binfo->server) ? binfo->server : "unknown",
		(binfo->version) ? binfo->version : "-"
		);
}

void botnet_dumplinklist(BotNet *bn)
{
	BotInfo *binfo;
	BotNet	*bn2;
	Mech	*bot;
	Server	*sv;

	/*
	 *  send link lines
	 */
	for(bot=botlist;bot;bot=bot->next)
	{
		/*
		 *  tell the other side who the local bots are
		 */
		sv = find_server(bot->server);
		to_file(bn->sock,"BL%i %c %s!%s %s:%i %s %s\n",
			bot->guid,
			(bot == bn->controller) ? '0' : '1',
			bot->nick,
			(bot->userhost) ? bot->userhost : "unknown@unknown",
			(sv) ? ((*sv->realname) ? sv->realname : sv->name) : "unknown",
			(sv) ? sv->port : 0,
			BOTCLASS,VERSION);
	}
	for(bn2=botnetlist;bn2;bn2=bn2->next)
	{
		if ((bn2 == bn) || (bn2->status != BN_LINKED) || !(bn2->list_complete))
			continue;
		if (bn2->rbot)
			botnet_BL(bn->sock,bn2->rbot);
		for(binfo=bn2->subs;binfo;binfo=binfo->next)
			botnet_BL(bn->sock,binfo);
	}
	/*
	 *  tell remote end to sync
	 */
	to_file(bn->sock,"BL-\n");
}

int connect_to_bot(NetCfg *cfg)
{
	BotNet	*bn;
	int	s;

#ifdef DEBUG
	debug("(connect_to_bot) NetCfg* "mx_pfmt" = { \"%s\", guid = %i, port = %i, ... }\n",
		(mx_ptr)cfg,nullstr(cfg->host),cfg->guid,cfg->port);
#endif /* DEBUG */

	if ((s = SockConnect(cfg->host,cfg->port,FALSE)) < 0)
		return(-1);

	/*
	 *  set linked status
	 */
	cfg->linked = TRUE;

	set_mallocdoer(connect_to_bot);
	bn = (BotNet*)Calloc(sizeof(BotNet));

	bn->sock = s;
	bn->status = BN_CONNECT;
	bn->when = now;
	bn->guid = cfg->guid;

	bn->next = botnetlist;
	botnetlist = bn;

	return(0);
}

/*
 *
 *  protocol routines
 *
 */

void basicAuth(BotNet *bn, char *rest)
{
	NetCfg	*cfg;
	char	*pass;
	int	authtype = -1;

	if (bn->status != BN_WAITAUTH)
		return;

	if ((pass = chop(&rest)))
	{
		if (!Strcmp(pass,"PTA"))
			authtype = BNAUTH_PLAINTEXT;
#ifdef MD5CRYPT
		if (!Strcmp(pass,"MD5"))
			authtype = BNAUTH_MD5;
#endif /* MD5CRYPT */
	}

	/*
	 *  find the other bots' password so we can check if it matches
	 */
	pass = NULL;
	for(cfg=netcfglist;cfg;cfg=cfg->next)
	{
		if (cfg->guid == bn->guid)
		{
			pass = cfg->pass;
			break;
		}
	}
	if (!pass || !*pass)
		goto badpass;

	switch(authtype)
	{
	case BNAUTH_PLAINTEXT:
		if (Strcmp(pass,rest))
			goto badpass;
		break;
#ifdef MD5CRYPT
	case BNAUTH_MD5:
		if (netpass && *netpass)
		{
			char	*enc,temppass[24+strlen(pass)+strlen(netpass)];

			/* "mypass theirpass REMOTEsid LOCALsid" */
			sprintf(temppass,"%s %s %i %i",netpass,pass,bn->rsid,bn->lsid);
#ifdef DEBUG
			debug(">> md5 pass exchange: %s\n",temppass);
#endif /* DEBUG */
			enc = crypt(temppass,rest);
#ifdef DEBUG
			debug("(basicAuth) their = %s, mypass = %s :: md5 = %s\n",
				pass,netpass,enc);
#endif /* DEBUG */
			if (!Strcmp(enc,rest))
				break;
		}
#endif /* MD5CRYPT */
	default:
	badpass:
		/*
		 *  we can/should use deaduplink here since we set cfg->linked = TRUE in basicBanner()
		 */
		botnet_deaduplink(bn);
#ifdef DEBUG
		debug("(basicAuth) bad password [ guid = %i ]\n",bn->guid);
#endif /* DEBUG */
		return;
	}

	to_file(bn->sock,"BK\n");
	bn->status = BN_LINKED;
	botnet_dumplinklist(bn);
#ifdef DEBUG
	debug("(basicAuth) bn->tick = 0\n");
#endif /* DEBUG */
	bn->tick = 0;
	bn->tick_last = now - 580; /* 10 minutes (10*60) - 20 seconds */
}

void basicAuthOK(BotNet *bn, char *rest)
{
	if (bn->status != BN_WAITLINK)
		return;

	bn->status = BN_LINKED;
	botnet_dumplinklist(bn);
#ifdef DEBUG
	debug("(basicAuthOK) bn->tick = 0\n");
#endif /* DEBUG */
	bn->tick = 0;
	bn->tick_last = now - 580; /* 10 minutes (10*60) - 20 seconds */
}

void basicBanner(BotNet *bn, char *rest)
{
	Mech	*netbot;
	NetCfg	*cfg;
	char	*p;
	int	guid;
	int	authtype = -1;

	/*
	 *  we're only prepared to listen to banners in the first connection phase
	 */
	if (bn->status != BN_BANNERSENT && bn->status != BN_UNKNOWN)
		return;

	/*
	 *  find out who's calling
	 */
	p = chop(&rest);
	guid = a2i(p);
	/*
	 *  bad guid received
	 */
	if (errno)
	{
		if (bn->guid)
		{
			/*
			 *  we know who its supposed to be, ergo sum, we've set cfg->linked = TRUE, lets undo that
			 */
			reset_linkable(bn->guid);
		}
		bn->status = BN_DEAD;
		deadlinks = TRUE;
		return;
	}

	if (bn->guid && bn->guid != guid)
	{
		/*
		 *  its not who we expected!
		 */
#ifdef DEBUG
		debug("(basicBanner) {%i} calling guid %i but got guid %i!\n",
			bn->sock,bn->guid,guid);
#endif /* DEBUG */
		botnet_deaduplink(bn);
		return;
	}

	/*
	 *  either (bn->guid == 0) || (bn->guid == guid), we dont need to check
	 */
	bn->guid = guid;

	/*
	 *  prevent circular linking
	 */
	if (bn->status == BN_UNKNOWN)
	{
		/*
		 *  they are connecting to us
		 */
		if ((cfg = find_netcfg(guid)))
		{
			if (cfg->linked)
			{
				/*
				 *  we already think this remote bot is connected and it's still connecting to us!
				 */
				bn->status = BN_DEAD;
				deadlinks = TRUE;
#ifdef DEBUG
				debug("(basicBanner) {%i} guid %i (connecting to us) is already linked!\n",
					bn->sock,bn->guid);
#endif /* DEBUG */
				return;
			}
			/*
			 *  its not linked? well it is now!
			 */
			cfg->linked = TRUE;
		}
	}

	/*
	 *  get a session number
	 */
	p = chop(&rest);
	bn->rsid = a2i(p);
	if (errno)
	{
		botnet_deaduplink(bn);
		return;
	}

	/*
	 *  parse banner options
	 */
	while((p = chop(&rest)))
	{
		if (!Strcmp(p,"PTA"))
			bn->opt.pta = TRUE;
#ifdef MD5CRYPT
		if (!Strcmp(p,"MD5"))
			bn->opt.md5 = TRUE;
#endif /* MD5CRYPT */
	}

	/*
	 *  update timestamp
	 */
	bn->when = now;

	/*
	 *  if the remote bot initiated the connection we need a valid pass from them
	 *  before we send our own password to validate
	 */
	if (bn->status == BN_UNKNOWN)
	{
		bn->controller = netbot = get_netbot();
		to_file(bn->sock,"BB%i %i PTA" md5banneropt "\n",netbot->guid,bn->lsid);
		bn->status = BN_WAITAUTH;
		return;
	}

	/*
	 *  we're the one that initiated the connection, we now send our password
	 */
	if (!netpass || !*netpass)
	{
		botnet_deaduplink(bn);
		return;
	}

	/*
	 *  select authentication method
	 */
	if (bn->opt.pta && (BNAUTH_PLAINTEXT > authtype))
		authtype = BNAUTH_PLAINTEXT;
#ifdef MD5CRYPT
	if (bn->opt.md5 && (BNAUTH_MD5 > authtype))
		authtype = BNAUTH_MD5;
#endif /* MD5CRYPT */

	switch(authtype)
	{
	case BNAUTH_PLAINTEXT:
		to_file(bn->sock,"BAPTA %s\n",netpass);
		break;
#ifdef MD5CRYPT
	case BNAUTH_MD5:
		if ((cfg = find_netcfg(guid)))
		{
			if (cfg->pass && *cfg->pass && *netpass)
			{
				char	*enc,salt[8];
				char	temppass[24+strlen(cfg->pass)+strlen(netpass)];

				/* "theirpass mypass LOCALsid REMOTEsid" */
				sprintf(temppass,"%s %s %i %i",cfg->pass,netpass,bn->lsid,bn->rsid);
#ifdef DEBUG
				debug(">> md5 pass exchange: %s\n",temppass);
#endif /* DEBUG */
				sprintf(salt,"$1$%04x",(rand() >> 16));
				enc = crypt(temppass,salt);
				to_file(bn->sock,"BAMD5 %s\n",enc);
				break;
			}
		}
#endif /* MD5CRYPT */
	default:
		botnet_deaduplink(bn);
		return;
	}
	bn->status = BN_WAITLINK;
}

void basicLink(BotNet *bn, char *version)
{
	BotInfo	*binfo,*delete,**pp;
	NetCfg	*cfg;
	char	*nuh,*server;
	int	guid,hops;

	if (bn->status != BN_LINKED)
		return;

	/*
	 *  BL-
	 */
	if (*version == '-')
	{
		/*
		 *  check all links for conflicts
		 */
		for(binfo=bn->subs;binfo;binfo=binfo->next)
		{
			if ((cfg = find_netcfg(binfo->guid)) == NULL)
				continue;
			if (cfg->linked == TRUE)
				break;
		}
		if (binfo)
		{
#ifdef DEBUG
			debug("(basicLink) circular linking: guid == %i\n",binfo->guid);
#endif /* DEBUG */
			/*
			 *  drop everything we've gotten sofar...
			 */
			botnet_deaduplink(bn);
			return;
		}

		/*
		 *  we're done, we're fine, we're linked!
		 */
		if (bn->rbot)
		{
			botnet_relayBL(bn,bn->rbot);
			check_botinfo(bn->rbot);
		}
		for(binfo=bn->subs;binfo;binfo=binfo->next)
		{
			botnet_relayBL(bn,binfo);
			check_botinfo(binfo);
			if ((cfg = find_netcfg(binfo->guid)) == NULL)
				continue;
			cfg->linked = TRUE;
		}
		bn->list_complete = TRUE;
		return;
	}

	/*
	 *  BL<guid> <hops> <nick>!<userhost> <server> <version>
	 */
	nuh = chop(&version);
	guid = a2i(nuh);
	if (errno)
		return;

	nuh = chop(&version);
	hops = a2i(nuh);
	if (errno)
		return;

	nuh = chop(&version);
	server = chop(&version);

	if (!version || !*version)
		return;

	binfo = make_botinfo(guid,hops,nuh,server,version);

	if (guid == bn->guid)
	{
		if (bn->rbot)
			Free((char**)&bn->rbot);
		bn->rbot = binfo;
		send_global("botnet: connected to -- %s [guid %i]",nickcpy(NULL,nuh),bn->guid);
	}
	else
	{
		pp = &bn->subs;
		while(*pp)
		{
			delete = *pp;
			if (guid == delete->guid)
			{
				*pp = delete->next;
				Free((char**)&delete);
				break;
			}
			pp = &delete->next;
		}
		binfo->next = *pp;
		*pp = binfo;
	}

	if (bn->list_complete)
	{
		if ((cfg = find_netcfg(guid)))
			cfg->linked = TRUE;
		/*
		 *  broadcast the new link
		 */
		botnet_relayBL(bn,binfo);
		check_botinfo(binfo);
	}
}

void basicQuit(BotNet *bn, char *rest)
{
	BotInfo	*binfo,**pp_binfo;
	int	guid;

	if (bn->status != BN_LINKED)
		return;

	guid = a2i(rest);
	if (errno)
		return;

	pp_binfo = &bn->subs;
	while(*pp_binfo)
	{
		binfo = *pp_binfo;
		if (binfo->guid == guid)
		{
			send_global("botnet quit: Lost guid %s (from %i)",rest,bn->guid);
			*pp_binfo = binfo->next;
			reset_linkable(guid);
			Free((char**)&binfo);
			break;
		}
		pp_binfo = &binfo->next;
	}
	botnet_relay(bn,"BQ%s\n",rest);
}

/*
 *
 */

void partyMessage(BotNet *bn, char *rest)
{
	char	*src,*dst,*orig = rest;

	orig = rest;

	dst = chop(&rest);
	src = chop(&rest);

	if (!*rest)
		return;

	if (*dst == '*')
	{
		/*
		 *  partyline_broadcast() uses CurrentNick for the first %s in the format
		 */
		Strncpy(CurrentNick,src,NUHLEN);

		if (*rest == 1)
			partyline_broadcast(NULL,"* %s %s\n",(rest+1));
		else
			partyline_broadcast(NULL,"<%s> %s\n",rest);
	}

	unchop(orig,rest);
	botnet_relay(bn,"PM%s\n",orig);
}

/*
 *
 */

void ushareUser(BotNet *bn, char *rest)
{
	User	*user,tempuser;
	char	c,*handle,*channel,*pass;
	int	i,tick,modcount,uaccess;

	c = *(rest++);
	tick = a2i(chop(&rest));
	if (errno)
		return;
	if (c != '+' && bn->tick != tick)
		return;
	switch(c)
	{
	case '+':
		/* `UU+tick modcount access handle chan pass' */
		i = 0;
		modcount = a2i(chop(&rest));
		i += errno;
		uaccess = a2i(chop(&rest));
		i += errno;
		handle = chop(&rest);
		channel = chop(&rest);
		pass = chop(&rest);
		if (i == 0 && handle && *handle && channel && *channel && pass && *pass)
		{
			if (!Strcmp(pass,"none"))
				pass = NULL;
			i = 0;
			bn->addsession = (rand() | 1);
			for(current=botlist;current;current=current->next)
			{
				if (current->setting[TOG_NETUSERS].int_var == 0)
					continue;
				user = find_handle(handle);
				if (user && user->modcount < modcount)
				{
					/* user with higher modcount overwrites */
					remove_user(user);
					user = NULL;
				}
				if (!user)
				{
#ifdef DEBUG
					debug("(ushareUser) user %s ++ re-creating\n",user->name);
#endif /* DEBUG */
					user = add_user(handle,channel,pass,uaccess);
					user->modcount = modcount;
					user->guid = bn->guid;		/* we got new user from <guid> this session */
					user->tick = global_tick;
					user->addsession = bn->addsession;
					i++;
				}
			}
			if (i)
			{
#ifdef DEBUG
				debug("(ushareUser) bn->tick = %i\n",tick);
#endif /* DEBUG */
				bn->tick = tick;
				global_tick++;
			}
		}
		break;
	case '-':
#ifdef DEBUG
		debug("(ushareUser) ticking to next user %i++\n",bn->tick);
#endif /* DEBUG */
		for(current=botlist;current;current=current->next)
			for(user=current->userlist;user;user=user->next)
				if (user->guid == bn->guid && user->addsession)
					user->addsession = 0;
		bn->addsession = 0;
		bn->tick++;
		to_file(bn->sock,"UT%i\n",bn->tick);
		bn->tick_last = now;
		break;
	case '*':
		for(current=botlist;current;current=current->next)
		{
			if (current->setting[TOG_NETUSERS].int_var == 0)
				continue;
			for(user=current->userlist;user;user=user->next)
			{
				if (user->guid == bn->guid && user->addsession == bn->addsession)
				{
#ifdef DEBUG
					debug("(ushareUser) user %s ++ mask %s\n",user->name,rest);
#endif /* DEBUG */
					addmasktouser(user,rest);
				}
			}
		}
		break;
	case '!':
		user = cfgUser;
		cfgUser = &tempuser;
		cfg_opt(chop(&rest));
		cfgUser = user;
		for(current=botlist;current;current=current->next)
		{
			if (current->setting[TOG_NETUSERS].int_var == 0)
				continue;
			for(user=current->userlist;user;user=user->next)
			{
				if (user->guid == bn->guid && user->addsession == bn->addsession)
				{
#ifdef DEBUG
					debug("(ushareUser) user %s ++ touching flags\n",user->name);
#endif /* DEBUG */
					user->aop = tempuser.aop;
					user->echo = tempuser.echo;
					user->avoice = tempuser.avoice;
#ifdef BOUNCE
					user->bounce = tempuser.bounce;
#endif /* BOUNCE */
				}
			}
		}
		break;
	}
#ifdef DEBUG
	current = NULL;
#endif /* DEBUG */
}

void ushareTick(BotNet *bn, char *rest)
{
	Strp	*pp;
	Mech	*mech;
	User	*user,*senduser;
	int	i;

	i = a2i(rest);
	if (errno)
		return;
#ifdef DEBUG
	debug("(ushareTick) remote bot ticked %i\n",i);
#endif /* DEBUG */
	senduser = 0;
	for(mech=botlist;mech;mech=mech->next)
	{
		for(user=mech->userlist;user;user=user->next)
		{
			if (i <= user->tick)
			{
				if (!senduser)
					senduser = user;
				/* user->guid != bn->guid :: dont send users back to the bot we got them from */
				if (user->tick < senduser->tick && user->guid != bn->guid)
					senduser = user;
			}
		}
	}
	if (senduser)
	{
		char	*p,temp[8];

		user = senduser;
#ifdef DEBUG
		debug("(ushareTick) user %s, user->tick = %i (%i)\n",user->name,user->tick,i);
#endif /* DEBUG */
		to_file(bn->sock,"UU+%i %i %i %s %s %s\n",user->tick,user->modcount,
			user->access,user->name,(user->chan) ? user->chan : "*",(user->pass) ? user->pass : "none");
		*(p = temp) = 0;
		if (user->aop)
			*(p++) = 'a';
#ifdef BOUNCE
		if (user->bounce)
			*(p++) = 'b';
#endif /* BOUNCE */
		if (user->echo)
			*(p++) = 'e';
		if (user->avoice)
			*(p++) = 'v';
		*(p++) = 'p';
		*(p++) = '0' + user->prot;
		*p = 0;
		to_file(bn->sock,"UU!%i %s\n",user->tick,temp);
		for(pp=user->mask;pp;pp=pp->next)
		{
			to_file(bn->sock,"UU*%i %s\n",user->tick,pp->p);
		}
		to_file(bn->sock,"UU-%i\n",user->tick);
		return;
	}
}

/*
 *
 */

void botnet_parse(BotNet *bn, char *rest)
{
	const LinkCmd *cmdlist;
	int	i;

	if (*rest == 'B')
	{
		cmdlist = basicProto;
		goto jmp1;
	}

	if (bn->status != BN_LINKED)
		return;

	switch(*rest)
	{
	case 'C':
		cmdlist = chanProto;
		break;
	case 'P':
		cmdlist = partyProto;
		break;
	case 'U':
		cmdlist = ushareProto;
		break;
	default:
		goto jmp2;
	}

jmp1:
	for(i=0;cmdlist[i].command;i++)
	{
		if (rest[1] == cmdlist[i].command)
		{
			cmdlist[i].func(bn,rest+2);
			return;
		}
	}

jmp2:
	;
	/*
	 *  relay to bots that know/want the protocol
	 */
}

void botnet_newsock(void)
{
	BotNet	*bn;
	int	s;

	/*
	 *  accept the connection
	 */
	if ((s = SockAccept(linksock.sock)) < 0)
		return;

#ifdef DEBUG
	debug("(botnet_newsock) {%i} new socket\n",s);
#endif /* DEBUG */

	set_mallocdoer(botnet_newsock);
	bn = (BotNet*)Calloc(sizeof(BotNet));

	bn->sock = s;
	bn->status = BN_UNKNOWN;
	bn->lsid = rand();
	bn->when = now;

	bn->next = botnetlist;
	botnetlist = bn;

	/*
	 *  crude... but, should work
	 */
	last_autolink = now + AUTOLINK_DELAY;
}

/*
 *
 */

void select_botnet(void)
{
	BotNet	*bn;

	/*
	 *  handle incoming connections
	 */
	if (linkport && linksock.sock == -1)
	{
		linksock.sock = SockListener(linkport);
		if (linksock.sock != -1)
		{
			linksock.next = botnetlist;
			botnetlist = (BotNet*)&linksock;
#ifdef DEBUG
			debug("(doit) {%i} Linksocket is active (%i)\n",linksock.sock,linkport);
#endif /* DEBUG */
		}
	}

	/*
	 *  autolink
	 */
	if (autolink && (now > last_autolink))
	{
		last_autolink = now + AUTOLINK_DELAY;

		if (autolink_cfg)
			autolink_cfg = autolink_cfg->next;
		if (!autolink_cfg)
			autolink_cfg = netcfglist;

		if (autolink_cfg && !autolink_cfg->linked &&
			autolink_cfg->host && autolink_cfg->pass)
		{
			/*
			 *  this thing isnt linked yet!
			 */
			connect_to_bot(autolink_cfg);
		}
	}

	short_tv &= ~TV_BOTNET;
	for(bn=botnetlist;bn;bn=bn->next)
	{
		chkhigh(bn->sock);
		if (bn->status == BN_CONNECT)
		{
			FD_SET(bn->sock,&write_fds);
			short_tv |= TV_BOTNET;
		}
		else
		{
			FD_SET(bn->sock,&read_fds);
		}
	}
}

void process_botnet(void)
{
	BotNet	*bn,**pp;
	BotInfo	*binfo;
	Mech	*netbot;
	char	*rest,linebuf[MSGLEN];

	for(bn=botnetlist;bn;bn=bn->next)
	{
		/*
		 *  usersharing tick, 10 minute period
		 */
		if (bn->status == BN_LINKED && (bn->tick_last + 600) < now)
		{
#ifdef DEBUG
			debug("(process_botnet) {%i} periodic ushare tick\n",bn->sock);
#endif /* DEBUG */
			bn->tick_last = now;
			to_file(bn->sock,"UT%i\n",bn->tick);
		}

		/*
		 *
		 */
		if (bn->has_data)
			goto has_data;

		/*
		 *  outgoing connection established
		 */
		if (FD_ISSET(bn->sock,&write_fds))
		{
			bn->lsid = rand();
			bn->controller = netbot = get_netbot();
			if (to_file(bn->sock,"BB%i %i PTA" md5banneropt "\n",netbot->guid,bn->lsid) < 0)
			{
				botnet_deaduplink(bn);
			}
			else
			{
				bn->status = BN_BANNERSENT;
				bn->when = now;
			}
			/* write_fds is only set for sockets where reading is not needed */
			continue;
		}

		/*
		 *  incoming data read
		 */
		if (FD_ISSET(bn->sock,&read_fds))
		{
			/*
			 *  ye trusty old linksock
			 */
			if (bn->status == BN_LINKSOCK)
				botnet_newsock();
			else
			{
		has_data:
				/*
				 *  Commands might change the botnetlist,
				 *  so returns here are needed
				 */
				rest = sockread(bn->sock,bn->sd,linebuf);
				bn->has_data = (rest) ? TRUE : FALSE;
				if (rest)
				{
					botnet_parse(bn,rest);
					if (!deadlinks)
						goto has_data; /* process more lines if link list is unchanged */
					goto clean;
				}
				switch(errno)
				{
				default:
#ifdef DEBUG
					debug("(process_botnet) {%i} sockread() errno = %i\n",bn->sock,errno);
#endif /* DEBUG */
					botnet_deaduplink(bn);
				case EAGAIN:
				case EINTR:
					goto clean;
				}
			}
		}

		if ((bn->status == BN_CONNECT) && ((now - bn->when) > LINKTIME))
		{
#ifdef DEBUG
			debug("(process_botnet) {%i} Life is good; but not for this guy (guid == %i). Timeout!\n",
				bn->sock,bn->guid);
#endif /* DEBUG */
			botnet_deaduplink(bn);
		}
	}
clean:
	/*
	 *  quit/delete BN_DEAD links
	 */
	if (!deadlinks)
		return;

	pp = &botnetlist;
	while(*pp)
	{
		bn = *pp;
		if (bn->status == BN_DEAD)
		{
			*pp = bn->next;
			send_global("botnet quit: Lost guid %i",bn->guid);
#ifdef DEBUG
			debug("(process_botnet) botnet quit: guid %i\n",bn->guid);
#endif /* DEBUG */
			while((binfo = bn->subs))
			{
				bn->subs = binfo->next;
#ifdef DEBUG
				debug("(process_botnet) botnet quit: guid %i child of %i on socket %i\n",
					binfo->guid,bn->guid,bn->sock);
#endif /* DEBUG */
				if (bn->list_complete)
				{
					send_global("botnet quit: Lost guid %i (child of %i)",
						binfo->guid,bn->guid);
					botnet_relay(bn,"BQ%i\n",binfo->guid);
					reset_linkable(binfo->guid);
				}
				Free((char**)&binfo);
			}
			if (bn->list_complete)
			{
				botnet_relay(bn,"BQ%i\n",bn->guid);
			}
			close(bn->sock);
			Free((char**)&bn);
			continue;
		}
		pp = &bn->next;
	}
	deadlinks = FALSE;
}

/*
 *
 *  commands related to botnet
 *
 */

void do_addlink(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS + CARGS
	 */
	NetCfg	*cfg;
	char	*guid,*pass,*host,*port;
	int	sz,iguid,iport;

	guid = chop(&rest);
	pass = chop(&rest);
	host = chop(&rest);
	port = chop(&rest);

	if (!guid || !pass)
	{
usage:
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	iguid = a2i(guid);
	if (errno)
		goto usage;

	sz = sizeof(NetCfg) + strlen(pass);

	if (host && port)
	{
		sz += strlen(host);
		iport = a2i(port);
		if (errno || iport < 1 || iport > 65535)
			goto usage;
	}
	else
	{
		host = NULL;
		iport = -1;
	}

	set_mallocdoer(do_addlink);
	cfg = (NetCfg*)Calloc(sz);

	cfg->guid = iguid;
	cfg->port = iport;
	cfg->host = Strcat(cfg->pass,pass) + 1;

	if (host)
		Strcpy(cfg->host,host);
	else
		cfg->host = NULL;

	cfg->next = netcfglist;
	netcfglist = cfg;
}

void do_dellink(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS + CARGS
	 */
	NetCfg	*cfg,**pp;
	char	*guid;
	int	iguid;

	guid = chop(&rest);
	iguid = a2i(guid);
	if (errno)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	pp = &netcfglist;
	while((cfg = *pp))
	{
		if (cfg->guid == iguid)
			break;
		pp = &cfg->next;
	}

	if (!cfg)
	{
		to_user(from,"unknown guid: %i",iguid);
		return;
	}

	*pp = cfg->next;
	Free((char**)&cfg);
}

void do_link(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS
	 */
	NetCfg	*cfg;
	char	numbuf[12];
	char	*guid;
	int	iguid;
	int	a,b,c;
	int	n;

	/*
	 *  list all the known links
	 */
	if (!rest)
	{
		a = b = c = 0;
		for(cfg=netcfglist;cfg;cfg=cfg->next)
		{
			sprintf(numbuf,"%i%n",cfg->guid,&n);
			if (n > a) a = n;
			n = (cfg->pass) ? strlen(cfg->pass) : 0;
			if (n > b) b = n;
			n = (cfg->host) ? strlen(cfg->host) : 0;
			if (n > c) c = n;
		}
		if (4 > a) a = 4;
		if (4 > b) b = 4;
		if (4 > c) c = 4;
		a += 2;
		b += 2;
		c += 2;
		to_user(from,"%-*s%-*s%-*s%s",a,"guid",b,"pass",c,"host","port");
		for(cfg=netcfglist;cfg;cfg=cfg->next)
		{
			to_user(from,"%-*i%-*s%-*s%i",a,cfg->guid,b,(cfg->pass) ? cfg->pass : "",
				c,(cfg->host) ? cfg->host : "",cfg->port);
		}
		return;
	}

	guid = chop(&rest);
	iguid = a2i(guid);
	if (errno)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	for(cfg=netcfglist;cfg;cfg=cfg->next)
	{
		if (cfg->guid == iguid)
			break;
	}

	if (!cfg)
	{
		to_user(from,"unknown guid: %i",iguid);
		return;
	}

	if (!cfg->host)
	{
		to_user(from,"unknown host/port for guid %i",iguid);
		return;
	}

	if (cfg->linked)
	{
		to_user(from,"guid %i is already connected",iguid);
		return;
	}

	if (connect_to_bot(cfg) < 0)
	{
		to_user(from,"unable to create connection");
		return;
	}

	send_global("botnet: connecting -- guid %i [%s:%i]",iguid,cfg->host,cfg->port);
}

#endif /* BOTNET */
