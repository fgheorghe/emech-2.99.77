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
#define DEBUG_C
#include "config.h"

#ifdef DEBUG

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "settings.h"

#define boolstr(x)	(x) ? "TRUE" : "FALSE"

LS const char tabs[20] = "\t\t\t\t\t\t\t\t\t\t";

LS const struct
{
	char	*name;
	int	size;

} StructList[] =
{
{ "Auth",		sizeof(Auth)		},
{ "Ban\t",		sizeof(Ban)		},
{ "Chan",		sizeof(Chan)		},
{ "ChanStats",		sizeof(ChanStats)	},
{ "ChanUser",		sizeof(ChanUser)	},
{ "Client",		sizeof(Client)		},
{ "ircLink",		sizeof(ircLink)		},
{ "IReq",		sizeof(IReq)		},
{ "KillSock",		sizeof(KillSock)	},
{ "Mech",		sizeof(Mech)		},
#ifdef NOTIFY
{ "Notify",		sizeof(Notify)		},
#endif /* NOTIFY */
#ifdef SEEN
{ "Seen",		sizeof(Seen)		},
#endif /* SEEN */
{ "Server",		sizeof(Server)		},
{ "Setting",		sizeof(Setting)		},
{ "Strp",		sizeof(Strp)		},
{ "TrivScore",		sizeof(TrivScore)	},
{ "User",		sizeof(User)		},
{ "OnMsg",		sizeof(OnMsg)		},
{ "Spy\t",		sizeof(Spy)		},
{ NULL, }};

LS struct
{
	void	*func;
	char	*name;
	int	num;
	int	size;
	int	mall_size;

} ProcList[] =
{
{	SockConnect,			"SockConnect"			},
{	add_bot,			"add_bot"			},
{	add_server,			"add_server"			},
{	add_shit,			"add_shit"			},
{	add_user,			"add_user"			},
{	addmasktouser,			"addmasktouser"			},
{	cfg_greet,			"cfg_greet"			},
{	cfg_opt,			"cfg_opt"			},
{	cfg_pass,			"cfg_pass"			},
{	cfg_user,			"cfg_user"			},
{	change_authnick,		"change_authnick"		},
{	ctcp_dcc,			"ctcp_dcc"			},
{	copy_vars,			"copy_vars"			},
{	dcc_chat,			"dcc_chat"			},
{	do_auth,			"do_auth"			},
{	do_die,				"do_die"			},
{	do_greet,			"do_greet"			},
{	do_kicksay,			"do_kicksay"			},
{	do_set_report,			"do_set_report"			},
{	do_spy,				"do_spy"			},
{	join_channel,			"join_channel"			},
{	killsock,			"killsock"			},
{	make_ban,			"make_ban"			},
{	make_chanuser,			"make_chanuser"			},
{	make_ireq,			"make_ireq"			},
{	on_join,			"on_join"			},
{	on_kick,			"on_kick"			},
{	on_mode,			"on_mode"			},
{	on_msg,				"on_msg"			},
{	on_nick,			"on_nick"			},
{	randstring_getline,		"randstring_getline"		},
{	readcfgfile,			"readcfgfile"			},
{	reverse_topic,			"reverse_topic"			},
{	to_user,			"to_user"			},
{	send_kick,			"send_kick"			},
{	send_mode,			"send_mode"			},
{	set_str_varc,			"set_str_varc"			},
{	setbotnick,			"setbotnick"			},
{	sig_hup,			"sig_hup"			},
#ifdef ALIAS
{	do_alias,			"do_alias"			},
#endif /* ALIAS */
#ifdef BOTNET
{	botnet_newsock,			"botnet_newsock"		},
{	connect_to_bot,			"connect_to_bot"		},
{	do_addlink,			"do_addlink"			},
{	make_botinfo,			"make_botinfo"			},
#endif /* BOTNET */
#ifdef BOUNCE
{	bounce_parse,			"bounce_parse"			},
{	process_bounce,			"process_bounce"		},
#endif /* BOUNCE */
#ifdef NOTE
{	catch_note,			"catch_note"			},
{	do_note,			"do_note"			},
#endif /* NOTE */
#ifdef NOTIFY
{	catch_whois,			"catch_whois"			},
{	notify_callback,		"notify_callback"		},
{	notifylog_callback,		"notifylog_callback"		},
#endif /* NOTIFY */
#ifdef RAWDNS
{	rawdns,				"rawdns"			},
{	parse_query,			"parse_query"			},
{	read_dnsroot,			"read_dnsroot"			},
#endif /* RAWDNS */
#ifdef SEEN
{	make_seen,			"make_seen"			},
#endif /* SEEN */
#ifdef STATS
{	stats_plusminususer,		"stats_plusminususer"		},
#endif /* STATS */
#ifdef TCL
{	tcl_hook,			"tcl_hook"			},
#endif /* TCL */
#ifdef TOYBOX
{	read_bigcharset_callback,	"read_bigcharset_callback"	},
#endif /* TOYBOX */
#ifdef TRIVIA
{	trivia_check,			"trivia_check"			},
{	trivia_question,		"trivia_question"		},
{	trivia_score_callback,		"trivia_score_callback"		},
#endif /* TRIVIA */
#ifdef UPTIME
{	init_uptime,			"init_uptime"			},
{	send_uptime,			"send_uptime"			},
#endif /* UPTIME */
{	0,				"(unknown)"			},
{ NULL, }};

#ifdef TCL
LS const DEFstruct TCLdefs[] =
{
{ HOOK_PARSE,		"HOOK_PARSE"		},
{ HOOK_TIMER,		"HOOK_TIMER"		},
{ HOOK_COMMAND,		"HOOK_COMMAND"		},
{ HOOK_BOTNET,		"HOOK_BOTNET"		},
{ HOOK_DCC_COMPLETE,	"HOOK_DCC_COMPLETE"	},
{ (int)tcl_timer_jump,	"tcl_timer_jump"	},
{ (int)tcl_parse_jump,	"tcl_parse_jump"	},
{ 0, }};
#endif /* TCL */

LS const DEFstruct CNdefs[] =
{
{ CN_NOSOCK,		"CN_NOSOCK"		},
{ CN_DNSLOOKUP,		"CN_DNSLOOKUP"		},
{ CN_TRYING,		"CN_TRYING"		},
{ CN_CONNECTED,		"CN_CONNECTED"		},
{ CN_ONLINE,		"CN_ONLINE"		},
{ CN_DISCONNECT,	"CN_DISCONNECT"		},
{ CN_BOTDIE,		"CN_BOTDIE"		},
{ CN_NEXTSERV,		"CN_NEXTSERV"		},
{ 0, }};

LS const DEFstruct SPdefs[] =
{
{ SP_NULL,		"SP_NULL"		},
{ SP_NOAUTH,		"SP_NOAUTH"		},
{ SP_KLINED,		"SP_KLINED"		},
{ SP_FULLCLASS,		"SP_FULLCLASS"		},
{ SP_TIMEOUT,		"SP_TIMEOUT"		},
{ SP_ERRCONN,		"SP_ERRCONN"		},
{ SP_DIFFPORT,		"SP_DIFFPORT"		},
{ 0, }};

#ifdef NOTIFY
LS const DEFstruct NFdefs[] =
{
{ NF_OFFLINE,		"NF_OFFLINE"		},
{ NF_WHOIS,		"NF_WHOIS"		},
{ NF_MASKONLINE,	"NF_MASKONLINE"		},
{ NF_NOMATCH,		"NF_NOMATCH"		},
{ 0, }};
#endif /* NOTIFY */

#ifdef SEEN
LS const DEFstruct SEdefs[] =
{
{ SEEN_PARTED,		"SEEN_PARTED"		},
{ SEEN_QUIT,		"SEEN_QUIT"		},
{ SEEN_NEWNICK,		"SEEN_NEWNICK"		},
{ SEEN_KICKED,		"SEEN_KICKED"		},
{ 0, }};
#endif /* SEEN */

#ifdef BOTNET
LS const DEFstruct BNdefs[] =
{
{ BN_UNKNOWN,		"BN_UNKNOWN"		},
{ BN_DEAD,		"BN_DEAD"		},
{ BN_LINKSOCK,		"BN_LINKSOCK"		},
{ BN_CONNECT,		"BN_CONNECT"		},
{ BN_BANNERSENT,	"BN_BANNERSENT"		},
{ BN_WAITAUTH,		"BN_WAITAUTH"		},
{ BN_WAITLINK,		"BN_WAITLINK"		},
{ BN_LINKED,		"BN_LINKED"		},
{ 0, }};
#endif /* BOTNET */

const char *strdef(const DEFstruct *dtab, int num)
{
	int	i;

	for(i=0;(dtab[i].idstr);i++)
	{
		if (dtab[i].id == num)
			return(dtab[i].idstr);
	}
	return("UNKNOWN");
}

void memreset(void)
{
	aMEA	*mp;
	int	i;

	for(mp=mrrec;mp;mp=mp->next)
	{
		for(i=0;i<MRSIZE;i++)
			mp->mme[i].touch = FALSE;
	}
}

LS void *mem_lowptr;
LS void *mem_hiptr;

void memtouch(void *addr)
{
	aMEA	*mp;
	int	i;

	if (addr == NULL)
		return;
	for(mp=mrrec;mp;mp=mp->next)
	{
		for(i=0;i<MRSIZE;i++)
		{
			if (mp->mme[i].area == addr)
			{
				mp->mme[i].touch = TRUE;
				if (mem_hiptr < addr)
					mem_hiptr = addr;
				if (mem_lowptr > addr)
					mem_lowptr = addr;
				return;
			}
		}
	}
}

const char *proc_lookup(void *addr, int size)
{
	int	i;

	for(i=0;ProcList[i].name;i++)
	{
		if (ProcList[i].func == addr)
		{
			ProcList[i].size += size;
			ProcList[i].mall_size += size + 8 + ((size & 7) ? (8 - (size & 7)) : 0);
			ProcList[i].num++;
			return(ProcList[i].name);
		}
	}
	return(NULL);
}

char *atime(time_t when)
{
	char	*pt,*zp;

	pt = ctime(&when);
	zp = STRCHR(pt,'\n');
	*zp = 0;
	return(pt);
}

void debug_server(Server *sp, char *pad)
{
	char	*pl;

	if (!sp)
	{
		debug("%s; ---\n",pad);
		return;
	}
	pl = (strlen(pad) == 2) ? "\t" : "";
	debug("%s; Server*\t\t"mx_pfmt"\n",pad,(mx_ptr)sp);
	debug("%s; next\t\t"mx_pfmt"\n",pad,(mx_ptr)sp->next);
	debug("%s; ident\t\t%i\n",pad,sp->ident);
	debug("%s; name\t\t\"%s\"\n",pad,nullbuf(sp->name));
	debug("%s; pass\t\t\"%s\"\n",pad,nullbuf(sp->pass));
	debug("%s; realname\t\t\"%s\"\n",pad,nullbuf(sp->realname));
	debug("%s; usenum\t\t%i\n",pad,sp->usenum);
	debug("%s; port\t\t%i\n",pad,sp->port);
	debug("%s; err\t\t%s%s (%i)\n",pad,pl,strdef(SPdefs,sp->err),sp->err);
	debug("%s; lastconnect\t%s%s (%lu)\n",pad,pl,atime(sp->lastconnect),sp->lastconnect);
	debug("%s; lastattempt\t%s%s (%lu)\n",pad,pl,atime(sp->lastattempt),sp->lastattempt);
	debug("%s; maxontime\t\t%s (%lu)\n",pad,idle2str(sp->maxontime,FALSE),sp->maxontime);
	debug("%s; ---\n",pad);
}

#define DSET_PROC	0
#define DSET_GLOBAL	1
#define DSET_CHAN	2

void debug_settings(UniVar *setting, int type)
{
	UniVar	*varval;
	const char *tpad;
	char	*pad;
	int	i,n;

	i = CHANSET_SIZE;
	switch(type)
	{
	case DSET_PROC:
		pad = "  ";
		break;
	case DSET_GLOBAL:
		pad = "    ";
		break;
	default:
		i = 0;
		pad = "      ";
	}

	debug("%s> setting\n",pad+2);
	for(;VarName[i].name;i++)
	{
		if ((type == DSET_CHAN) && (i >= CHANSET_SIZE))
			break;
		if (IsProc(i))
		{
			if (type == DSET_GLOBAL)
				continue;
		}
		else
		{
			if (type == DSET_PROC)
				continue;
		}

		tpad = STREND(tabs);
		n = 24 - (strlen(pad) + strlen(VarName[i].name) + 2);
		while(n >= 8)
		{
			n = n - 8;
			tpad--;
		}
		if (n > 0)
			tpad--;

		varval = (IsProc(i)) ? setting[i].proc_var : &setting[i];

		if (IsChar(i))
		{
			debug("%s; %s%s`%c'\n",pad,VarName[i].name,tpad,setting[i].int_var);
		}
		else
		if (IsTog(i))
		{
			debug("%s; %s%s%s\n",pad,VarName[i].name,tpad,boolstr(setting[i].int_var));
		}
		else
		if (IsInt(i))
		{
			debug("%s; %s%s%i\n",pad,VarName[i].name,tpad,setting[i].int_var);
		}
		else
		if (IsStr(i))
		{
			memtouch(setting[i].str_var);
			debug("%s; %s%s"mx_pfmt" \"%s\"\n",pad,VarName[i].name,tpad,
				(mx_ptr)setting[i].str_var,nullstr(setting[i].str_var));
		}
	}
	debug("%s; ---\n",pad);
}

void debug_memory(void)
{
#ifdef ALIAS
	Alias	*alias;
#endif /* ALIAS */
	Chan	*chan;
	Mech	*bot;
	aMEA	*mea;
	char	t[100],*pt,*funcname;
	int	i,n;

#ifdef ALIAS
	for(alias=aliaslist;alias;alias=alias->next)
	{
		memtouch(alias);
		memtouch(alias->alias);
		memtouch(alias->format);
	}
#endif /* ALIAS */
	for(bot=botlist;bot;bot=bot->next)
	{
		for(i=CHANSET_SIZE;VarName[i].name;i++)
		{
			if (IsStr(i))
				memtouch(bot->setting[i].str_var);
		}
		for(chan=bot->chanlist;chan;chan=chan->next)
		{
			for(i=0;i<CHANSET_SIZE;i++)
			{
				if (IsStr(i))
					memtouch(chan->setting[i].str_var);
			}
		}
		for(i=0;i<LASTCMDSIZE;i++)
		{
			memtouch(bot->lastcmds[i]);
		}
	}
	debug("> Memory allocations\n");
	for(mea=mrrec;(mea);mea=mea->next)
	{
		for(i=0;i<MRSIZE;i++)
		{
			if (mea->mme[i].area)
			{
				funcname = (char*)proc_lookup(mea->mme[i].doer,mea->mme[i].size);
				if (funcname)
				{
					Strcpy(t,funcname);
				}
				else
				{
					sprintf(t,mx_pfmt,(mx_ptr)mea->mme[i].doer);
				}
				pt = (char*)&tabs[10 - ((31 - strlen(t)) / 8)];
				debug("  ; "mx_pfmt"\t\t%s%s\t%i\t%s\t%s (%lu)\n",
					(mx_ptr)mea->mme[i].area,
					t,pt,mea->mme[i].size,(mea->mme[i].touch) ? "" : "(Leak)",
					atime(mea->mme[i].when),mea->mme[i].when);
			}
		}
	}
	debug("  ; ---\n");
	debug("> Memory by Function\n");
	for(i=0;ProcList[i].name;i++)
	{
		if (ProcList[i].num == 0)
			continue;

		sprintf(t,"  ; "mx_pfmt"",(mx_ptr)ProcList[i].func);

		n = 24 - strlen(t);
		while(n > 0)
		{
			n = n - 8;
			Strcat(t,"\t");
		}
		Strcat(t,ProcList[i].name);
		n = 32 - strlen(ProcList[i].name);
		while(n > 0)
		{
			n = n - 8;
			Strcat(t,"\t");
		}

		debug("%s%i\t\t%i\t\t%i\n",t,ProcList[i].num,ProcList[i].size,ProcList[i].mall_size);
	}
	debug("  ; ---\n");
	debug("; mem_lowptr\t\t"mx_pfmt"\n",(mx_ptr)mem_lowptr);
	debug("; mem_hiptr\t\t"mx_pfmt"\n",(mx_ptr)mem_hiptr);
	debug("; Memory Span\t\t%i bytes\n",(int)(mem_hiptr - mem_lowptr));
}

#ifdef BOTNET

void debug_botinfo(BotInfo *binfo)
{
	for(;binfo;binfo=binfo->next)
	{
		debug("    ; BotInfo*\t\t"mx_pfmt"\n",(mx_ptr)binfo);
		debug("    ; next\t\t"mx_pfmt"\n",(mx_ptr)binfo->next);
		debug("    ; guid\t\t%i\n",binfo->guid);
		debug("    ; hops\t\t%i\n",binfo->hops);
		debug("    ; version\t\t\"%s\"\n",nullstr(binfo->version));
		debug("    ; server\t\t\"%s\"\n",nullstr(binfo->server));
		debug("    ; nuh\t\t\"%s\"\n",nullbuf(binfo->nuh));
		debug("    ; ---\n");
	}
}

void debug_botnet(void)
{
	struct	sockaddr_in sai;
	BotNet	*bn;
	NetCfg	*cfg;
	int	sz;

	debug("> netcfglist\t\t"mx_pfmt"\n",(mx_ptr)netcfglist);
	if (netcfglist)
	{
		for(cfg=netcfglist;cfg;cfg=cfg->next)
		{
			memtouch(cfg);
			debug("  ; NetCfg*\t\t"mx_pfmt"\n",(mx_ptr)cfg);
			debug("  ; next\t\t"mx_pfmt"\n",(mx_ptr)cfg->next);
			debug("  ; guid\t\t%i\n",cfg->guid);
			debug("  ; port\t\t%i\n",cfg->port);
			debug("  ; linked\t\t%s\n",boolstr(cfg->linked));
			debug("  ; host\t\t\"%s\"\n",nullstr(cfg->host));
			debug("  ; pass\t\t\"%s\"\n",nullstr(cfg->pass));
			if (cfg->next)
				debug("  ; ---\n");
		}
		debug("; ---\n");
	}
	debug("> botnetlist\t\t"mx_pfmt"\n",(mx_ptr)botnetlist);
	if (botnetlist)
	{
		for(bn=botnetlist;bn;bn=bn->next)
		{
			memtouch(bn);
			debug("  ; BotNet*\t\t"mx_pfmt"\n",(mx_ptr)bn);
			debug("  ; next\t\t"mx_pfmt"\n",(mx_ptr)bn->next);
			sz = sizeof(sai);
			if (getpeername(bn->sock,(struct sockaddr*)&sai,&sz) < 0)
			{
				debug("  ; sock\t\t%i [ getpeername == -1 ]\n",bn->sock);
			}
			else
			{
				debug("  ; sock\t\t%i [ %s:%i ]\n",bn->sock,
					inet_ntoa(sai.sin_addr),ntohs(sai.sin_port));
			}
			debug("  ; status\t\t%s (%i)\n",strdef(BNdefs,bn->status),bn->status);
			if (bn->status != BN_LINKSOCK)
			{
				debug("  ; has_data\t\t%i\n",bn->has_data);
				debug("  ; guid\t\t%i\n",bn->guid);
				debug("  ; lsid\t\t%i\n",bn->lsid);
				debug("  ; rsid\t\t%i\n",bn->rsid);
				debug("  ; opt.pta\t\t%s\n",boolstr(bn->opt.pta));
				debug("  ; controller\t\t"mx_pfmt" { guid = %i }\n",(mx_ptr)bn->controller,bn->controller->guid);
				debug("  ; when\t\t%s (%lu)\n",atime(bn->when),bn->when);
				debug("  > rbot\t\t"mx_pfmt"\n",(mx_ptr)bn->rbot);
				debug_botinfo(bn->rbot);
				debug("  > subs\t\t"mx_pfmt"\n",(mx_ptr)bn->subs);
				debug_botinfo(bn->subs);
				debug("  > users\t\t"mx_pfmt"\n",(mx_ptr)bn->users);
			}
			if (bn->next)
				debug("  ; ---\n");
		}
	}
}

#endif /* BOTNET */

void debug_core(void)
{
	Auth	*auth;
	Ban	*ban;
	Chan	*chan;
	ChanUser *CU;
	Client	*client;
	IReq	*ir;
	Mech	*bot;
#ifdef NOTIFY
	nfLog	*nfl;
	Notify	*nf;
#endif /* NOTIFY */
#ifdef SEEN
	Seen	*seen;
#endif /* SEEN */
	Server	*sp;
	Spy	*spy;
	Strp	*st;
#ifdef TRIVIA
	TrivScore *triv;
#endif /* TRIVIA */
	User	*user;
	int	i;

	debug("; VERSION\t\t\"%s\"\n",VERSION);
	debug("; SRCDATE\t\t\"%s\"\n",SRCDATE);
	debug("; BOTLOGIN\t\t\"%s\"\n",BOTLOGIN);
	debug("; BOTCLASS\t\t\"%s\"\n",BOTCLASS);
	debug("> StructList\n");
	for(i=0;StructList[i].name;i++)
	{
		debug("  ; %s\t\t%i\n",StructList[i].name,StructList[i].size);
	}
	debug("  ; ---\n");
	if (current)
		debug("; current\t\t"mx_pfmt" \"%s\"\n",(mx_ptr)current,nullstr(current->nick));
	else
		debug("; current\t\t"mx_pfmt"\n",(mx_ptr)current);
	debug("; executable\t\t\"%s\"\n",executable);
	debug("; configfile\t\t\"%s\"\n",configfile);
	debug("; uptime\t\t%s (%lu)\n",atime(uptime),uptime);
	debug("; short_tv\t\t%s (%is wait)\n",boolstr(short_tv),(short_tv) ? 1 : 30);
	debug("> serverlist\t\t"mx_pfmt"\n",(mx_ptr)serverlist);
	for(sp=serverlist;sp;sp=sp->next)
	{
		memtouch(sp);
		debug_server(sp,"  ");
	}
	debug("> botlist\t\t"mx_pfmt"\n",(mx_ptr)botlist);
	for(bot=botlist;bot;bot=bot->next)
	{
		memtouch(bot);
		debug("  ; Mech*\t\t"mx_pfmt"\n",(mx_ptr)bot);
		debug("  ; next\t\t"mx_pfmt"\n",(mx_ptr)bot->next);
		debug("  ; connect\t\t%s (%i)\n",strdef(CNdefs,bot->connect),bot->connect);
		debug("  ; sock\t\t%i\n",bot->sock);
		debug("  ; ip\t\t\t%s\n",inet_ntoa(bot->ip));
		debug("  > server\t\t%i\n",bot->server);
		sp = find_server(bot->server);
		if (sp)
		{
			debug_server(sp,"    ");
		}
		debug("  > nextserver\t\t%i\n",bot->nextserver);
		sp = find_server(bot->nextserver);
		if (sp)
		{
			debug_server(sp,"    ");
		}
		debug("  ; nick\t\t\"%s\"\n",nullstr(bot->nick));
		debug("  ; wantnick\t\t\"%s\"\n",nullstr(bot->wantnick));

		debug_settings(bot->setting,DSET_GLOBAL);

		/*
		 *  userlist
		 */
		debug("  > userlist\t\t"mx_pfmt"\n",(mx_ptr)bot->userlist);
		for(user=bot->userlist;user;user=user->next)
		{
			memtouch(user);
			debug("    ; User*\t\t"mx_pfmt"\n",(mx_ptr)user);
			debug("    ; next\t\t"mx_pfmt"\n",(mx_ptr)user->next);
			debug("    ; chan\t\t\"%s\"\n",nullstr(user->chan));
			debug("    ; name\t\t\"%s\"\n",user->name);
			debug("    ; pass\t\t\"%s\"\n",nullstr(user->pass));
			debug("    > mask\t\t"mx_pfmt"\n",(mx_ptr)user->mask);
			for(st=user->mask;st;st=st->next)
			{
				memtouch(st);
				debug("      ; Strp*\t\t"mx_pfmt" { "mx_pfmt", \"%s\" }\n",
					(mx_ptr)st,(mx_ptr)st->next,st->p);
			}
#ifdef GREET
			memtouch(user->greet);
			debug("    ; greet\t\t"mx_pfmt" \"%s\"\n",(mx_ptr)user->greet,nullstr(user->greet));
#endif /* GREET */
#ifdef NOTE
			debug("    > note\t\t"mx_pfmt"\n",(mx_ptr)user->note);
			for(st=user->note;st;st=st->next)
			{
				memtouch(st);
				debug("      ; Strp*\t\t"mx_pfmt" { "mx_pfmt", \"%s\" }\n",
					(mx_ptr)st,(mx_ptr)st->next,st->p);
			}
#endif /* NOTE */
			debug("    ; ---\n");
		}

		debug("  > parselist\t\t"mx_pfmt"\n",(mx_ptr)bot->parselist);
		for(ir=bot->parselist;ir;ir=ir->next)
		{
			memtouch(ir);
			debug("    ; IReq*\t\t"mx_pfmt"\n",(mx_ptr)ir);
			debug("    ; next\t\t"mx_pfmt"\n",(mx_ptr)ir->next);
			debug("    ; ---\n");
		}

		/*
		 *  spylist
		 */
		debug("  > spylist\t\t"mx_pfmt"\n",(mx_ptr)bot->spylist);
		for(spy=bot->spylist;spy;spy=spy->next)
		{
			memtouch(spy);
			debug("    ; Spy*\t\t"mx_pfmt"\n",(mx_ptr)spy);
			debug("    ; next\t\t"mx_pfmt"\n",(mx_ptr)spy->next);
			debug("    ; t_src\t\t%i\n",spy->t_src);
			debug("    ; t_dest\t\t%i\n",spy->t_dest);
			debug("    ; dcc\t\t"mx_pfmt"\n",(mx_ptr)spy->dcc);
			debug("    ; src\t\t"mx_pfmt" \"%s\"\n",(mx_ptr)spy->src,nullstr(spy->src));
			debug("    ; dest\t\t"mx_pfmt" \"%s\"\n",(mx_ptr)spy->dest,nullstr(spy->dest));
			debug("    ; ---\n");
		}
		debug("  > struct spy\n");
		debug("    ; rawirc\t\t%s\n",boolstr(bot->spy.rawirc));
		debug("    ; message\t\t%s\n",boolstr(bot->spy.message));
		debug("    ; status\t\t%s\n",boolstr(bot->spy.status));
		debug("    ; channel\t\t%s\n",boolstr(bot->spy.channel));
		debug("    ; ---\n");

#ifdef NOTIFY
		debug("  > notifylist\t\t"mx_pfmt"\n",(mx_ptr)bot->notifylist);
		for(nf=bot->notifylist;nf;nf=nf->next)
		{
			memtouch(nf);
			debug("    ; Notify*\t\t"mx_pfmt"\n",(mx_ptr)nf);
			debug("    ; next\t\t"mx_pfmt"\n",(mx_ptr)nf->next);
			debug("    ; status\t\t%s (%i)\n",strdef(NFdefs,nf->status),nf->status);
			debug("    ; checked\t\t%s (%lu)\n",atime(nf->checked),nf->checked);

			debug("    > log\t\t"mx_pfmt"\n",(mx_ptr)nf->log);
			for(nfl=nf->log;nfl;nfl=nfl->next)
			{
				memtouch(nfl);
				debug("      ; nfLog*\t\t"mx_pfmt"\n",(mx_ptr)nfl);
				debug("      ; next\t\t"mx_pfmt"\n",(mx_ptr)nfl->next);
				debug("      ; signon\t\t%s (%lu)\n",atime(nfl->signon),nfl->signon);
				debug("      ; signoff\t\t%s (%lu)\n",atime(nfl->signoff),nfl->signoff);
				debug("      ; realname\t\"%s\"\n",nullstr(nfl->realname));
				debug("      ; userhost\t\"%s\"\n",nullstr(nfl->userhost));
				debug("      ; ---\n");
			}

			debug("    ; info\t\t\"%s\"\n",nullstr(nf->info));
			debug("    ; endofmask\t\t"mx_pfmt"\n",(mx_ptr)nf->endofmask);
			debug("    ; mask\t\t\"%s\"\n",nullstr(nf->mask));
			debug("    ; nick\t\t\"%s\"\n",nf->nick);
			debug("    ; ---\n");
		}
#endif /* NOTIFY */

		debug("  > authlist\t\t"mx_pfmt"\n",(mx_ptr)bot->authlist);
		for(auth=bot->authlist;auth;auth=auth->next)
		{
			memtouch(auth);
			debug("    ; Auth*\t\t"mx_pfmt"\n",(mx_ptr)auth);
			debug("    ; next\t\t"mx_pfmt"\n",(mx_ptr)auth->next);
			debug("    ; active\t\t%s (%lu)\n",atime(auth->active),auth->active);
			debug("    ; user\t\t"mx_pfmt" { \"%s\", ... }\n",(mx_ptr)auth->user,auth->user->name);
			debug("    ; nuh\t\t\"%s\"\n",nullstr(auth->nuh));
			debug("    ; ---\n");
		}

		debug("  > chanlist\t\t"mx_pfmt"\n",(mx_ptr)bot->chanlist);
		for(chan=bot->chanlist;chan;chan=chan->next)
		{
			memtouch(chan);
			memtouch(chan->name);
			memtouch(chan->key);
			memtouch(chan->topic);
			memtouch(chan->kickedby);
			debug("    ; Chan*\t\t"mx_pfmt"\n",(mx_ptr)chan);
			debug("    ; next\t\t"mx_pfmt"\n",(mx_ptr)chan->next);
			debug("    ; name\t\t"mx_pfmt" \"%s\"\n",(mx_ptr)chan->name,nullstr(chan->name));
			debug("    ; key\t\t"mx_pfmt" \"%s\"\n",(mx_ptr)chan->key,nullstr(chan->key));
			debug("    ; topic\t\t"mx_pfmt" \"%s\"\n",(mx_ptr)chan->topic,nullstr(chan->topic));
			debug("    ; kickedby\t\t"mx_pfmt" \"%s\"\n",(mx_ptr)chan->kickedby,nullstr(chan->kickedby));
			debug("    > Chan->banlist\n");
			for(ban=chan->banlist;ban;ban=ban->next)
			{
				memtouch(ban);
				debug("      ; Ban*\t\t"mx_pfmt" { "mx_pfmt", \"%s\", \"%s\", %s (%lu) }\n",
					(mx_ptr)ban,(mx_ptr)ban->next,nullstr(ban->banstring),
					nullstr(ban->bannedby),atime(ban->time),ban->time);
			}
			debug("      ; ---\n");
			debug("    > Chan->users\n");
			for(CU=chan->users;CU;CU=CU->next)
			{
				memtouch(CU);
				memtouch(CU->nick);
				debug("      ; ChanUser*\t"mx_pfmt" { "mx_pfmt", \"%s\", \"%s\", ... }\n",
					(mx_ptr)CU,(mx_ptr)CU->next,nullstr(CU->nick),nullstr(CU->userhost));
			}
			debug("      ; ---\n");
			if (chan->cacheuser)
			{
				debug("    ; cacheuser\t\t"mx_pfmt" { "mx_pfmt", \"%s\", \"%s\", ... }\n",
					(mx_ptr)chan->cacheuser,(mx_ptr)chan->cacheuser->next,
					nullstr(chan->cacheuser->nick),nullstr(chan->cacheuser->userhost));
			}
			else
			{
				debug("    ; cacheuser\t\t"mx_pfmt"\n",(mx_ptr)chan->cacheuser);
			}

			debug_settings(chan->setting,DSET_CHAN);

			debug("    ; limit\t\t%i\n",chan->limit);

			debug("    ; private\t\t%s\n",boolstr(chan->private));
			debug("    ; secret\t\t%s\n",boolstr(chan->secret));
			debug("    ; moderated\t\t%s\n",boolstr(chan->moderated));
			debug("    ; topprot\t\t%s\n",boolstr(chan->topprot));
			debug("    ; limitmode\t\t%s\n",boolstr(chan->limitmode));
			debug("    ; invite\t\t%s\n",boolstr(chan->invite));
			debug("    ; nomsg\t\t%s\n",boolstr(chan->nomsg));
			debug("    ; keymode\t\t%s\n",boolstr(chan->keymode));
			debug("    ; hiddenkey\t\t%s\n",boolstr(chan->hiddenkey));
			debug("    ; sync\t\t%s\n",boolstr(chan->sync));
			debug("    ; wholist\t\t%s\n",boolstr(chan->wholist));
			debug("    ; bot_is_op\t\t%s\n",boolstr(chan->bot_is_op));
			debug("    ; active\t\t%s\n",boolstr(chan->active));
			debug("    ; rejoin\t\t%s\n",boolstr(chan->rejoin));

			debug("    ; this10\t\t%i\n",chan->this10);
			debug("    ; last10\t\t%i\n",chan->last10);
			debug("    ; this60\t\t%i\n",chan->this60);
			debug("    ; last60\t\t%i\n",chan->last60);

#ifdef DYNAMODE
			debug("    ; lastlimit\t\t%s (%lu)\n",atime(chan->lastlimit),chan->lastlimit);
#endif /* DYNAMODE */
			if (chan->stats)
			{
				debug("    > ChanStats\t\t"mx_pfmt"\n",(mx_ptr)chan->stats);
				debug("      ; userseconds\t\t%i\n",chan->stats->userseconds);
				debug("      ; lasthouruserseconds\t%i\n",chan->stats->lasthouruserseconds);
				debug("      ; users\t\t%i\n",chan->stats->users);
				debug("      ; lastuser\t\t%s (%lu)\n",atime(chan->stats->lastuser),chan->stats->lastuser);
				if (chan->stats->flags & CSTAT_PARTIAL)
					debug("      ; flags\t\tCSTAT_PARTIAL (%i)\n",chan->stats->flags);
				else
					debug("      ; flags\t\tNO_FLAGS (%i)\n",chan->stats->flags);
				debug("      ; userpeak\t\t%i\n",chan->stats->userpeak);
				debug("      ; userlow\t\t%i\n",chan->stats->userlow);
				debug("      ; kicks\t\t%i\n",chan->stats->kicks);
				debug("      ; joins\t\t%i\n",chan->stats->joins);
				debug("      ; parts\t\t%i\n",chan->stats->parts);
				debug("      ; quits\t\t%i\n",chan->stats->quits);
				debug("      ; privmsg\t\t%i\n",chan->stats->privmsg);
				debug("      ; notice\t\t%i\n",chan->stats->notice);
			}
			debug("    ; ---\n");
		}
		debug("  ; activechan\t\t"mx_pfmt"\n",(mx_ptr)bot->activechan);
		debug("  > clientlist\t\t"mx_pfmt"\n",(mx_ptr)bot->clientlist);
		for(client=bot->clientlist;client;client=client->next)
		{
			memtouch(client);
			debug("    ; Client*\t\t"mx_pfmt"\n",(mx_ptr)client);
			debug("    ; next\t\t"mx_pfmt"\n",(mx_ptr)client->next);
			if (client->user)
				debug("    ; user\t\t"mx_pfmt" { \"%s\", ... }\n",(mx_ptr)client->user,client->user->name);
			else
				debug("    ; user\t\tNULL\n");
			debug("    ; sock\t\t%i\n",client->sock);
#ifdef DCC_FILE
			debug("    ; fileno\t\t%i\n",client->fileno);
			debug("    ; fileend\t\t%i\n",client->fileend);
#endif /* DCC_FILE */
			debug("    ; flags\t\t%i\n",client->flags);
			debug("    ; inputcount\t%i\n",client->inputcount);
			debug("    ; lasttime\t%s (%lu)\n",atime(client->lasttime),client->lasttime);
			debug("    ; ---\n");
		}
		debug("  ; lastreset\t\t%s (%lu)\n",atime(bot->lastreset),bot->lastreset);
		debug("  ; lastantiidle\t%s (%lu)\n",atime(bot->lastantiidle),bot->lastantiidle);
		debug("  ; lastrejoin\t\t%s (%lu)\n",atime(bot->lastrejoin),bot->lastrejoin);
		debug("  ; conntry\t\t%s (%lu)\n",atime(bot->conntry),bot->conntry);
		debug("  ; activity\t\t%s (%lu)\n",atime(bot->activity),bot->activity);
		debug("  ; uptime\t\t%s (%lu)\n",atime(bot->uptime),bot->uptime);
		debug("  ; ontime\t\t%s (%lu)\n",atime(bot->ontime),bot->ontime);
#ifdef IRCD_EXTENSIONS
		{
			char temp[200];

			*temp = 0;
			if (bot->ircx_flags & IRCX_WALLCHOPS)
				Strcat(temp,"|IRCX_WALLCHOPS");
			if (bot->ircx_flags & IRCX_WALLVOICES)
				Strcat(temp,"|IRCX_WALLVOICES");
			if (bot->ircx_flags)
			{
				debug("  ; ircx_flags\t\t(%s)\n",temp+1);
			}
			else
				debug("  ; ircx_flags\t\t0\n");
		}
#endif /* IRCD_EXTENSIONS */
		debug("  ; ---\n");
	}
#ifdef SEEN
	debug("> seenlist\t\t"mx_pfmt"\n",(mx_ptr)seenlist);
	if (seenlist)
	{
		for(seen=seenlist;seen;seen=seen->next)
		{
			memtouch(seen);
			debug("  ; Seen*\t\t"mx_pfmt" { "mx_pfmt", \"%s\", \"%s\", \"%s\", \"%s\", %s (%lu), %s (%i) }\n",
				(mx_ptr)seen,
				(mx_ptr)seen->next,
				nullstr(seen->nick),
				nullstr(seen->userhost),
				nullstr(seen->pa),
				nullstr(seen->pb),
				atime(seen->when),seen->when,
				strdef(SEdefs,seen->t),
				seen->t
				);
		}
		debug("  ; ---\n");
	}
#endif /* SEEN */
#ifdef TRIVIA
	debug("> scorelist\t\t"mx_pfmt"\n",(mx_ptr)scorelist);
	for(triv=scorelist;triv;triv=triv->next)
	{
		memtouch(triv);
		debug("  ; TrivScore*\t\t"mx_pfmt" { "mx_pfmt", %i, %i, %i, %i, %i, %i, \"%s\" }\n",
			(mx_ptr)triv,
			(mx_ptr)triv->next,
			triv->score_wk,
			triv->score_last_wk,
			triv->week_nr,
			triv->score_mo,
			triv->score_last_mo,
			triv->month_nr,
			triv->nick);
	}
	if (scorelist)
		debug("  ; ---\n");
#endif /* TRIVIA */
}

#ifdef RAWDNS

void debug_rawdns(void)
{
	dnsAuthority *da;
	dnsList *dns;

	for(da=dnsroot;da;da=da->next)
	{
		memtouch(da);
	}

	debug("> dnslist\t\t"mx_pfmt"\n",(mx_ptr)dnslist);
	for(dns=dnslist;dns;dns=dns->next)
	{
		memtouch(dns);
		debug("  ; dnsList*\t\t"mx_pfmt"\n",(mx_ptr)dns);
		debug("  ; next\t\t"mx_pfmt"\n",(mx_ptr)dns->next);
		debug("  ; when\t\t%s (%lu)\n",atime(dns->when),dns->when);
		if (dns->ip.s_addr)
		{
			debug("  ; ip\t\t\t%s\n",inet_ntoa(dns->ip));
		}
		else
			debug("  ; ip\t\t\t0\n");
		debug("  ; id\t\t\t%i\n",dns->id);
		debug("  ; cname\t\t\"%s\"\n",nullstr(dns->cname));
		debug("  ; host\t\t\"%s\"\n",dns->host);
		if (dns->next)
			debug("  ; ---\n");
	}
}

#endif /* RAWDNS */

#ifdef TCL

#if 0
typedef struct
{
        time_t          last;
        time_t          next;
        ulong           second1;        //:30;
        ulong           second2;        //:30;
        ulong           minute1;        //:30;
        ulong           minute2;        //:30;
        ulong           hour;           //:24;
        ulong           weekday;        //:7;
                        
} HookTimer;
#endif /* 0 */

char binstr[33];

char *ulong2bin(int limit, ulong x)
{
	char	*dst = binstr;
	int	n;

	if (limit > 30)
		limit -= 30;
	x <<= (32 - limit);

	for(n=0;n<limit;n++)
	{
		if (x & 0x80000000)
			*(dst++) = '1';
		else
			*(dst++) = '0';
		x <<= 1;
	}
	*dst = 0;
	return(binstr);
}

void debug_tcl(void)
{
	Hook	*h;

	debug("> hooklist (TCL)\n");
	for(h=hooklist;h;h=h->next)
	{
		memtouch(h);
		debug("  ; func\t\t"mx_pfmt" %s\n",(mx_ptr)h->func,strdef(TCLdefs,(int)h->func));
		debug("  ; guid\t\t%i\n",h->guid);
		debug("  ; flags\t\t%s (%i)\n",strdef(TCLdefs,h->flags),h->flags);
		if (h->flags == HOOK_TIMER)
		{
			debug("  ; timer\t\t"mx_pfmt"\n",(mx_ptr)h->type.timer);
			debug("  ; timer.second1\t%s ( 0..29)\n",ulong2bin(30,h->type.timer->second1));
			debug("  ; timer.second2\t%s (30..59)\n",ulong2bin(30,h->type.timer->second2));
			debug("  ; timer.minute1\t%s ( 0..29)\n",ulong2bin(30,h->type.timer->minute1));
			debug("  ; timer.minute2\t%s (30..59)\n",ulong2bin(30,h->type.timer->minute2));
			debug("  ; timer.hour\t\t%s (0..23)\n",ulong2bin(24,h->type.timer->hour));
			debug("  ; timer.weekday\t%s (0..6)\n",ulong2bin(7,h->type.timer->weekday));
		}
		debug("  ; self\t\t\"%s\"\n",nullstr(h->self));
		if (h->next)
			debug("  ; ---\n");
	}
}

#endif /* TCL */

void run_debug(void)
{
	memreset();

	mem_lowptr = ((void*)-1);
	mem_hiptr = NULL;

#ifdef SEEN
	memtouch(seenfile);
#endif /* SEEN */

#ifdef TRIVIA
	memtouch(triv_qfile);
#endif /* TRIVIA */

#ifdef UPTIME
	memtouch(uptimehost);
	memtouch(uptimenick);
#endif /* UPTIME */

	debug_core();
#ifdef BOTNET
	debug_botnet();
#endif /* BOTNET */
#ifdef RAWDNS
	debug_rawdns();
#endif /* RAWDNS */
#ifdef TCL
	debug_tcl();
#endif /* TCL */

	debug_memory();
}

int wrap_debug(void)
{
	char	fname[20];
	int	fd,backup_fd,backup_dodebug;

	debug("(wrap_debug) init...\n");

	backup_dodebug = dodebug;
	backup_fd = debug_fd;

	sprintf(fname,"debug.%lu",now);
	if ((fd = open(fname,O_WRONLY|O_CREAT|O_TRUNC,NEWFILEMODE)) < 0)
		return(0);
	debug_fd = fd;
	dodebug = TRUE;

	run_debug();

	close(fd);
	debug_fd = backup_fd;
	dodebug = backup_dodebug; 

	debug("(wrap_debug) all done.\n");
	return(1);
}

void do_debug(COMMAND_ARGS)
{
	if (wrap_debug())
		to_user(from,"Debug information has been written to file");
	else
		to_user(from,"Unable to write debug information to file");
}

void debug(char *format, ...)
{
	va_list msg;

	if (!dodebug)
		return;

	if (debug_fd == -1)
	{
		if (debugfile)
		{
			if ((debug_fd = open(debugfile,O_CREAT|O_TRUNC|O_WRONLY,SECUREFILEMODE)) < 0)
			{
				dodebug = FALSE;
				return;
			}
		}
		else
		{
			debug_fd = 0;
		}
		CoreClient.sock = debug_fd;
	}

	va_start(msg,format);
	vsprintf(debugbuf,format,msg);
	va_end(msg);

	if ((write(debug_fd,debugbuf,strlen(debugbuf))) < 0)
		dodebug = FALSE;
}

#endif /* DEBUG */
