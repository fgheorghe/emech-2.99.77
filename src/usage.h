/*

    EnergyMech, IRC bot software
    Copyright (c) 2000-2004 proton

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

typedef struct
{
	const char *command;
	char *usage;

} UsageList;

LS const UsageList ulist[] =
{
{ C_ACCESS,	"[channel] [nick|userhost]"						},
{ C_ADD,	"<handle> <channel|*> <nick|mask> <level> [password]"			},
#ifdef BOTNET
{ C_ADDLINK,	"<guid> <pass> [host] [port]"						},
#endif /* BOTNET */
{ C_ADDSERVER,	"<host> [port]"								},
#ifdef ALIAS
{ C_ALIAS,	"[alias [replacement format]]"						},
#endif /* ALIAS */
{ C_AWAY,	"[message]"								},
{ C_BAN,	"[channel] <nick|mask>"							},
{ C_BANLIST,	"[channel]"								},
#ifdef TOYBOX
{ C_BIGSAY,	"<message>"								},
#endif /* TOYBOX */
{ C_BYE,	NULL									},
{ C_CCHAN,	"[channel]"								},
#ifdef DYNCMD
{ C_CHACCESS,	"<command> [DISABLE|level]"						},
#endif /* DYNCMD */
{ C_CHANNELS,	NULL									},
{ C_CHAT,	NULL									},
{ C_CLEARSHIT,	NULL									},
{ C_CORE,	NULL									},
{ C_CSERV,	NULL									},
#ifdef CTCP
{ C_CTCP,	"<nick|channel> <ctcp command>"						},
#endif /* CTCP */
{ C_CYCLE,	"[channel]"								},
#ifdef DEBUG
{ C_DEBUG,	NULL									},
#endif /* DEBUG */
{ C_DEL,	"<handle>"								},
#ifdef BOTNET
{ C_DELLINK,	"<guid>"								},
#endif /* BOTNET */
{ C_DELSERVER,	"<host> [port]"								},
{ C_DEOP,	"[channel] <nick|pattern [...]>"					},
{ C_DIE,	"[reason]"								},
#ifdef RAWDNS
{ C_DNSSERVER,	"[+|-serverip]"								},
#endif /* RAWDNS */
{ C_DO,		"<raw_irc>"								},
{ C_DOWN,	"[channel]"								},
{ C_ECHO,	"<ON|OFF>"								},
{ C_ESAY,	"[channel] <message>"							},
{ C_FORGET,	"<channel>"								},
#ifdef GREET
{ C_GREET,	"<handle> [greeting | @greetfile]"					},
#endif /* GREET */
{ C_HELP,	"[topic|command|level|pattern]"						},
{ C_HOST,	"<ADD|DEL> <handle> <mask>"						},
{ C_IDLE,	"<nick>"								},
#ifdef TOYBOX
{ C_INSULT,	"[channel|nick]"							},
#endif /* TOYBOX */
{ C_INVITE,	"[channel] [nick]"							},
{ C_JOIN,	"<channel> [key]"							},
{ C_KB,		"[channel] <nick> [reason]"						},
{ C_KICK,	"[channel] <nick> [reason]"						},
{ C_KS,		"<channel> <\"pattern\"> <reason>"					},
{ C_KSLIST,	NULL									},
{ C_LAST,	"[number of commands]"							},
#ifdef BOTNET
{ C_LINK,	"<guid>"								},
#endif /* BOTNET */
{ C_LOAD,	NULL									},
{ C_LUSERS,	NULL									},
{ C_ME,		"[channel] <action>"							},
{ C_MODE,	"[channel|botnick] <mode ...>"						},
{ C_MSG,	"<nick|channel> <message>"						},
{ C_NAMES,	"[channel]"								},
{ C_NEXTSERVER,	NULL									},
{ C_NICK,	"[guid] <nick>"								},
#ifdef NOTE
{ C_NOTE,	"..."									},	/* */
#endif /* NOTE */
#ifdef NOTIFY
{ C_NOTIFY,	"[options] [nick]"							},	/* */
#endif /* NOTIFY */
{ C_ONTIME,	NULL									},
{ C_OP,		"[channel] [nick|mask]"							},
{ C_PART,	"<channel>"								},
{ C_PASSWD,	"[oldpassword] <newpassword>"						},
#ifdef TOYBOX
{ C_PICKUP,	"[channel|nick]"							},
#endif /* TOYBOX */
#ifdef CTCP
{ C_PING,	"<channel|nick>"							},
#endif /* CTCP */
{ C_QSHIT,	"<nick> [reason]"							},
{ C_RESET,	NULL									},
{ C_RKS,	"<channel> <pattern>"							},
#ifdef TOYBOX
{ C_RSAY,	"..."									},	/* */
#endif /* TOYBOX */
{ C_RSHIT,	"<channel> <nick|mask>"							},
{ C_RSPY,	"<channel|STATUS|MESSAGE|RAWIRC> [nick|channel|\">\" filename]"		},
#ifdef TOYBOX
{ C_RT,		"<channel>"								},
#endif /* TOYBOX */
{ C_SAVE,	NULL									},
{ C_SAY,	"<channel> <message>"							},
{ C_SCREW,	"[channel] <nick> [reason]"						},
#ifdef SEEN
{ C_SEEN,	"<nick>"								},
#endif /* SEEN */
#ifdef DCC_FILE
{ C_SEND,	"[to] <filename>"							},
#endif /* DCC_FILE */
{ C_SERVER,	"[servername] [port]"							},
{ C_SET,	"[channel|*] [setting [value]]"						},
{ C_SETPASS,	"<handle> <password>"							},
{ C_SHIT,	"<channel> <nick|userhost> <level> [expire] <reason>"			},
{ C_SHITLIST,	NULL									},
{ C_SHOWIDLE,	"[seconds]"								},
{ C_SHUTDOWN,	NULL									},
{ C_SITEBAN,	"[channel] <nick|userhost>"						},
{ C_SITEKB,	"[channel] <nick> [reason]"						},
{ C_SPY,	"<channel|STATUS|MESSAGE|RAWIRC> [channel|\">\" filename]"		},
{ C_SPYLIST,	"<channel>"								},
{ C_STATS,	"<type> [servername]"							},
#ifdef TCL
#ifdef PLEASE_HACK_MY_SHELL
{ C_TCL,	"<command>"								},
#endif /* PLEASE_HACK_MY_SHELL */
{ C_TCLSCRIPT,	"<filename>"								},
#endif /* TCL */
{ C_TIME,	NULL									},
{ C_TOPIC,	"[channel] <text>"							},
#ifdef ALIAS
{ C_UNALIAS,	"<alias>"								},
#endif /* ALIAS */
{ C_UNBAN,	"[channel] [nick|userhost]"						},
{ C_UNVOICE,	"[channel] <nick|pattern [...]>"					},
{ C_UP,		"[channel]"								},
#ifdef UPTIME
{ C_UPSEND,	NULL									},
#endif /* UPTIME */
{ C_UPTIME,	NULL									},
{ C_USAGE,	"<command>"								},
{ C_USER,	"<handle> <modifiers [...]>"						},
{ C_USERHOST,	"<nick>"								},
{ C_USERLIST,	"[+minlevel] [-maxlevel] [channel] [mask] [-B] [-C]"			},
{ C_VER,	NULL									},
{ C_VERIFY,	"<password>"								},
{ C_VOICE, 	"[channel] [nick|pattern [...]]"					},
{ C_WALL,	"[channel] <message>"							},
{ C_WHO,	"<channel> [-ops|-nonops] [pattern]"					},
{ C_WHOIS,	"<nick>"								},
{ C_WHOM,	NULL									},
{ NULL, }};
