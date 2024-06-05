/*

    EnergyMech, IRC bot software
    Copyright (c) 2001-2004 proton

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
#ifndef SETTINGS_H
#define SETTINGS_H 1
#ifdef VARS_C

#define DEFAULTCMDCHAR	'-'

#define ZERO		0
#define INTCAST(x)	(void*)((int)x)
#define CHARCAST	(void*)((int)DEFAULTCMDCHAR)

LS const Setting VarName[SIZE_VARS] =
{
/*
 *  all channel settings in the beginning
 */
/* TYPE		DEFAULT		NAME		MIN	MAX		*/
{ TOG_VAR,	ZERO,		"ABK",		0,	1		},
{ TOG_VAR,	ZERO,		"AOP",		0,	1		},	/* autoop enable */
{ INT_VAR,	ZERO,		"AUB",		0,	86400		},
{ INT_VAR,	INTCAST(1),	"AVOICE",	0,	2		},
{ STR_VAR,	NULL,		"CHANMODES"				},	/* modes to enforce, +ENFM to enable */
{ INT_VAR,	ZERO,		"CKL",		0,	20		},
{ TOG_VAR,	ZERO,		"CTL",		0,	1		},
#ifdef DYNAMODE
{ STR_VAR,	NULL,		"DYNLIMIT",	0,	1		},	/* settings for dynamode: `delay:window:minwin' */
#endif /* DYNAMODE */
{ TOG_VAR,	ZERO,		"ENFM",		0,	1		},
{ INT_VAR,	INTCAST(6),	"FL",		0,	20		},	/* number of lines that counts as a text flood */
{ INT_VAR,	ZERO,		"FPL",		0,	2		},
{ INT_VAR,	INTCAST(0),	"IKT",		0,	40320		},	/* idle-kick: minutes of idle-time (max 4 weeks) */
{ TOG_VAR,	ZERO,		"KS",		0,	1		},	/* kicksay enable */
{ INT_VAR,	INTCAST(90),	"MAL",		0,	200		},
{ INT_VAR,	INTCAST(7),	"MBL",		2,	20		},
{ INT_VAR,	INTCAST(7),	"MDL",		2,	20		},
{ INT_VAR,	INTCAST(7),	"MKL",		2,	20		},
{ INT_VAR,	INTCAST(1),	"MPL",		0,	2		},	/* mass action levels: 0=off, 1=kick, 2=kickban */
{ INT_VAR,	INTCAST(20),	"NCL",		2,	20		},
{ INT_VAR,	INTCAST(4),	"PROT",		0,	4		},	/* max enforced protection level */
{ TOG_VAR,	INTCAST(1),	"PUB",		0,	1		},	/* public commands */
{ TOG_VAR,	ZERO,		"RK",		0,	1		},	/* revenge kick enable */
{ TOG_VAR,	ZERO,		"SD",		0,	1		},	/* server-op deop enable */
{ TOG_VAR,	INTCAST(1),	"SHIT",		0,	1		},	/* shitlist enable */
{ TOG_VAR,	ZERO,		"SO",		0,	1		},	/* safe-op enable */
#ifdef STATS
{ STR_VAR,	NULL,		"STATS"					},	/* statistics log file */
#endif /* STATS */
{ TOG_VAR,	ZERO,		"TOP",		0,	1		},
{ TOG_VAR,	ZERO,		"ENFPASS",	0,	1		},	/* enforce passwords in this channel */
/*
 *  all channel global variables
 */
/* TYPE		DEFAULT		NAME		MIN	MAX		*/
{ INT_GLOBAL,	ZERO,		"AAWAY",	0,	1440		},	/* set auto-away after ___ minutes */
{ STR_GLOBAL,	NULL,		"ALTNICK"				},	/* alternative nick */
#ifdef BOTNET
{ TOG_PROC,	(&autolink),	"AUTOLINK",	0,	1		},	/* establish links automagically */
#endif /* BOTNET */
{ INT_GLOBAL,	INTCAST(3),	"BANMODES",	3,	6		},	/* max number of ban modes to send */
#ifdef BOUNCE
{ INT_PROC,	(&bounce_port),	"BNCPORT",	0,	65535		},	/* irc proxy port to listen on */
#endif /* BOUNCE */
{ TOG_GLOBAL,	INTCAST(1),	"CC",		0,	1		},	/* require command char */
{ CHR_GLOBAL,	CHARCAST,	"CMDCHAR",	1,	255		},	/* command char */
#ifdef CTCP
{ TOG_GLOBAL,	INTCAST(1),	"CTCP",		0,	1		},	/* ctcp replies enable */
#endif /* CTCP */
{ INT_PROC,	(&ctimeout),	"CTIMEOUT",	10,	3600		},	/* how long to wait between connect attempts */
#ifdef DCC_FILE
{ INT_GLOBAL,	INTCAST(4),	"DCCANON",	0,	100		},	/* anonymous (non user) DCC slots */
{ INT_GLOBAL,	INTCAST(4),	"DCCUSER",	0,	100		},	/* user DCC slots */
{ STR_GLOBAL,	NULL,		"DCCFILES"				},	/* string with space separated masks for auto-accepted filenames */
#endif /* DCC_FILE */
{ STR_GLOBAL,	NULL,		"IDENT"					},	/* register with this in the `user' field */
{ STR_GLOBAL,	NULL,		"IRCNAME"				},	/* register with this in the `real name' field */
#ifdef NOTIFY
{ INT_GLOBAL,	INTCAST(30),	"ISONDELAY",	10,	600		},	/* seconds between each ISON */
#endif /* NOTIFY */
#ifdef BOTNET
{ INT_PROC,	(&linkport),	"LINKPORT",	0,	65535		},	/* listen on <linkport> for botnet connections */
{ STR_PROC,	(&netpass),	"NETPASS"				},	/* local process netpass */
{ TOG_GLOBAL,	INTCAST(1),	"NETUSERS",	0,	1		},	/* this bot accepts shared users (on by default) */
#endif /* BOTNET */
{ TOG_GLOBAL,	ZERO,		"NOIDLE",	0,	1		},	/* dont idle */
#ifdef NOTIFY
{ STR_GLOBAL,	NULL,		"NOTIFYFILE"				},	/* read notify settings from <notifyfile> */
#endif /* NOTIFY */
{ TOG_GLOBAL,	ZERO,		"ONOTICE",	0,	1		},	/* ircd has /notice @#channel */
{ INT_GLOBAL,	INTCAST(3),	"OPMODES",	3,	6		},	/* max number of channel modes to send */
#ifdef TRIVIA
{ STR_PROC,	(&triv_qfile),	"QFILE"					},	/* load questions from <qfile> */
{ INT_PROC,	(&triv_qdelay),	"QDELAY",	1,	3600		},	/* seconds between each question */
{ CHR_PROC,	(&triv_qchar),	"QCHAR"					},	/* use <qchar> as mask char when displaying answer */
#endif /* TRIVIA */
#ifdef CTCP
{ TOG_GLOBAL,	ZERO,		"RF",		0,	1		},	/* random ctcp finger reply */
{ TOG_GLOBAL,	ZERO,		"RV",		0,	1		},	/* random ctcp version reply */
#endif /* CTCP */
#ifdef SEEN
{ STR_PROC,	(&seenfile),	"SEENFILE"				},	/* load/save seen database from <seenfile> */
#endif /* SEEN */
{ TOG_GLOBAL,	ZERO,		"SPY",		0,	1		},	/* send info about executed commands to status channel */
{ STR_GLOBAL,	NULL,		"UMODES"				},	/* send these modes on connect */
#ifdef UPTIME
{ STR_PROC,	(&uptimehost),	"UPHOST"				},	/* send uptime packets to <uphost> */
{ INT_PROC,	(&uptimeport),	"UPPORT",	0,	65535		},	/* send packets to port <upport> */
{ STR_PROC,	(&uptimenick),	"UPNICK"				},	/* send <upnick> as identifier instead of bots nick */
#endif /* UPTIME */
{ STR_GLOBAL,	NULL,		"USERFILE"				},	/* what file to load/save userlist from/to */
{ STR_GLOBAL,	NULL,		"VIRTUAL"				},	/* vistual host */
#ifdef WINGATE
{ STR_GLOBAL,	NULL,		"WINGATE"				},	/* wingate hostname */
{ INT_GLOBAL,	ZERO,		"WINGPORT",	0,	65535		},	/* wingate port */
#endif /* WINGATE */
#ifdef WEB
{ INT_PROC,	(&webport),	"WEBPORT",	0,	65535		},	/* httpd should listen on... */
#endif /* WEB */
{ 0, }};

#undef ZERO
#undef INTCAST
#undef CHARCAST

#else /* VARS_C */

extern const Setting VarName[];

#endif /* VARS_C */
#endif /* SETTINGS_H */
