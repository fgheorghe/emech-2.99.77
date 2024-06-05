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
#define USERLIST_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

/*
 *
 *  reading and writing userlists
 *
 */

/*
 *  functions that handle userlist stuff (userlist_cmds[])
 */

void cfg_user(char *rest)
{
	if (cfgUser)
	{
		if (cfgUser->chan)
			Free((char**)&cfgUser->chan);
		if (cfgUser->pass)
			Free((char**)&cfgUser->pass);
		Free((char**)&cfgUser);
	}

	set_mallocdoer(cfg_user);
	cfgUser = Calloc(sizeof(User) + strlen(rest));
	Strcpy(cfgUser->name,rest);
}

#ifdef BOTNET

void cfg_modcount(char *rest)
{
	int	i;

	i = a2i(rest);
	if (errno == 0 && i > 0)
		cfgUser->modcount = i;
}

#endif /* BOTNET */

void cfg_pass(char *rest)
{
	if (cfgUser->pass)
		Free((char**)&cfgUser->pass);

	set_mallocdoer(cfg_pass);
	cfgUser->pass = Strdup(rest);
}

void cfg_mask(char *rest)
{
	addmasktouser(cfgUser,rest);
}

void cfg_opt(char *rest)
{
	char	*anum = NULL;

	/*
	 *  `OPT aegp0vu100 #chan'
	 */
loop:
	switch(*rest)
	{
	case 'a':
		cfgUser->aop = TRUE;
		break;
#ifdef BOUNCE
	case 'b':
		cfgUser->bounce = TRUE;
		break;
#endif /* BOUNCE */
	case 'e':
		cfgUser->echo = TRUE;
		break;
#ifdef GREET
	case 'g':
		cfgUser->greetfile = TRUE;
		break;
	case 'r':
		cfgUser->randline = TRUE;
		break;
#endif /* GREET */
	case 'p':
		if (attrtab[(uchar)rest[1]] & NUM)
			cfgUser->prot = rest[1] - '0';
		break;
	case 'u':
		anum = rest+1;
		break;
	case 'v':
		cfgUser->avoice = TRUE;
		break;
	case ' ':
		*(rest++) = 0;
		cfgUser->access = a2i(anum);
		if (cfgUser->chan)
			Free((char**)&cfgUser->chan);
		set_mallocdoer(cfg_opt);
		cfgUser->chan = Strdup(rest);
		return;
	}
	rest++;
	if (*rest)
		goto loop;
}

void cfg_shit(char *rest)
{
	char	*channel,*mask,*from;
	time_t	save_now,expire,when;
	int	shitlevel;

	channel = chop(&rest);
	mask    = chop(&rest);
	from    = chop(&rest);

	/*
	 *  quick way of getting the shitlevel
	 *  also, if channel, mask or from is NULL, this will fail (cuz *rest == 0)
	 */
	if (*rest < '0' || *rest > MAXSHITLEVELCHAR)
		return;
	shitlevel = *rest - '0';
	chop(&rest);	/* discard shitlevel argument */

	/*
	 *  convert the expiry time
	 */
	expire = a2i(chop(&rest));	/* a2i() can handle NULLs */
	if (errno || expire < now)
		return;

	/*
	 *  convert time when the shit was added
	 */
	when = a2i(chop(&rest));
	if (errno || *rest == 0)	/* if *rest == 0, the reason is missing */
		return;

	/*
	 *  finally, add the sucker
	 */
	save_now = now;
	now = when;
	add_shit(from,channel,mask,rest,shitlevel,expire);
	now = save_now;
}

void cfg_kicksay(char *rest)
{
	Client	*backup;

	backup = CurrentDCC;
	CurrentDCC = (Client*)&CoreClient;
	do_kicksay((char*)CoreUser.name,NULL,rest,0);
	CurrentDCC = backup;
}

#ifdef GREET

void cfg_greet(char *rest)
{
	if (cfgUser->greet)
		Free((char**)&cfgUser->greet);

	set_mallocdoer(cfg_greet);
	cfgUser->greet = Strdup(rest);
}

#endif /* GREET */

#ifdef NOTE

void cfg_note(char *rest)
{
	Strp	*sp,**np;

	np = &cfgUser->note;
	while(*np)
		np = &(*np)->next;
	*np = sp = Malloc(sizeof(Strp) + strlen(rest));
	sp->next = NULL;
	Strcpy(sp->p,rest);
}

#endif /* NOTE */

void user_sync(void)
{
	User	*user;

	user = add_user(cfgUser->name,cfgUser->chan,cfgUser->pass,cfgUser->access);

	user->mask = cfgUser->mask;

	user->aop = cfgUser->aop;
	user->avoice = cfgUser->avoice;
	user->prot = cfgUser->prot;
#ifdef GREET
	user->greet = cfgUser->greet;
	user->greetfile = cfgUser->greetfile;
	user->randline = cfgUser->randline;
#endif /* GREET */
#ifdef BOUNCE
	user->bounce = cfgUser->bounce;
#endif /* BOUNCE */
	user->echo = cfgUser->echo;

#ifdef NOTE
	user->note = cfgUser->note;
#endif /* NOTE */
#ifdef BOTNET
	user->modcount = cfgUser->modcount;
	user->tick = global_tick;
	global_tick++;
#endif /* BOTNET */

	if (cfgUser->chan)
		Free((char**)&cfgUser->chan);
	if (cfgUser->pass)
		Free((char**)&cfgUser->pass);
	Free((char**)&cfgUser);
}

#define FL_ONEARG	1
#define FL_NEEDUSER	2	/* userfile */
#define FL_NEWUSER	4	/* userfile */

typedef struct CommandStruct
{
	char	*name;
	void	(*function)(char *);
	int	flags;

} ConfCommand;

LS const ConfCommand userlist_cmds[] =
{
/*
 *  users
 */
{ "USER",	cfg_user,	FL_NEWUSER  | FL_ONEARG	},
{ "PASS",	cfg_pass,	FL_NEEDUSER | FL_ONEARG	},
{ "MASK",	cfg_mask,	FL_NEEDUSER | FL_ONEARG	},
{ "OPT",	cfg_opt,	FL_NEEDUSER		},
{ "SHIT",	cfg_shit,	0			},
{ "KICKSAY",	cfg_kicksay,	0			},
#ifdef GREET
{ "GREET",	cfg_greet,	FL_NEEDUSER		},
#endif /* GREET */
#ifdef NOTE
{ "NOTE",	cfg_note,	FL_NEEDUSER		},
#endif /* NOTE */
#ifdef BOTNET
{ "MODCOUNT",	cfg_modcount,	FL_NEEDUSER		},
#endif /* BOTNET */
{ NULL, }};

int read_userlist_callback(char *line)
{
	char	*command;
	int	i;

	fix_config_line(line);
	command = chop(&line);
	for(i=0;userlist_cmds[i].name;i++)
	{
		if (!Strcasecmp(command,userlist_cmds[i].name))
			break;
	}
	if (userlist_cmds[i].name)
	{
		if (!cfgUser && (userlist_cmds[i].flags & FL_NEEDUSER))
		{
#ifdef DEBUG
			debug("[RUC] cfgUser is NULL for command that requires it to be set (%s)\n",command);
#endif /* DEBUG */
			return(FALSE);
		}
		if ((userlist_cmds[i].flags & FL_NEWUSER) && (cfgUser))
			user_sync();
		if (userlist_cmds[i].flags & FL_ONEARG)
		{
			command = chop(&line);
			line = command;
			if (!line || !*line)
				return(FALSE);
		}
		userlist_cmds[i].function(line);
	}
	return(FALSE);
}

int read_userlist(char *filename)
{
	User	*user,*u2;
	User	*olduserlist;
	User	*newuserlist;
	int	in;

	if (!filename)
		return(FALSE);
	if ((in = open(filename,O_RDONLY)) < 0)
		return(FALSE);

	purge_shitlist();
	purge_kicklist();

	olduserlist = current->userlist;
	cfgUser = current->userlist = NULL;

	readline(in,&read_userlist_callback);		/* readline closes in */

	/*
	 *  save the last user
	 */
	if (cfgUser)
		user_sync();

	newuserlist = current->userlist;
	current->userlist = olduserlist;

	for(user=newuserlist;user;user=user->next)
	{
		u2 = find_handle(user->name);		/* find user in old userlist, may be NULL */
		reset_userlink(u2,user);
	}

	/*
	 *  remove the old userlist
	 */
	while(current->userlist)
		remove_user(current->userlist);

	/*
	 *  re-apply shitlist that we just loaded
	 */
	check_shit();

	current->userlist = newuserlist;
	current->ul_save = 0;
	return(TRUE);
}

int write_userlist(char *filename)
{
	KickSay	*ks;
	Shit	*shit;
	Strp	*ump;
	User	*user;
	char	*p,flags[7];
	int	f;
#ifdef DEBUG
	int	dodeb;
#endif /* DEBUG */

	if (!filename)
		return(FALSE);

	if (!current->ul_save)
		return(TRUE);

	if ((f = open(filename,O_WRONLY|O_CREAT|O_TRUNC,NEWFILEMODE)) < 0)
		return(FALSE);

	/*
	 *  reset the change-counter
	 */
	current->ul_save = 0;

#ifdef DEBUG
	dodeb = dodebug;
	dodebug = FALSE;
#endif /* DEBUG */

	for(user=current->userlist;user;user=user->next)
	{
		to_file(f,"\nuser\t\t%s\n",user->name);
		for(ump=user->mask;ump;ump=ump->next)
			to_file(f,"mask\t\t%s\n",ump->p);
		/*
		 *  `OPT aegrp0vu100 #chan'
		 */
		p = flags;
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
#ifdef GREET
		if (user->greetfile)
			*(p++) = 'g';
		if (user->randline)
			*(p++) = 'r';
#endif /* GREET */
		*p = 0;
		to_file(f,"opt\t\t%sp%iu%i %s\n",flags,user->prot,(int)user->access,user->chan);
		/*
		 *  `PASS <password>'
		 */
		if (user->pass)
			to_file(f,"pass\t\t%s\n",user->pass);
#ifdef GREET
		if (user->greet)
			to_file(f,"greet\t\t%s\n",user->greet);
#endif /* GREET */
#ifdef NOTE
		for(ump=user->note;ump;ump=ump->next)
			to_file(f,"note\t\t%s\n",ump->p);
#endif /* NOTE */
#ifdef BOTNET
		to_file(f,"modcount\t%i\n",user->modcount);
#endif /* BOTNET */
	}

	to_file(f,"\n");

	for(shit=current->shitlist;shit;shit=shit->next)
	{
		to_file(f,"shit\t\t%s %s %s %i %lu %lu %s\n",
			shit->chan,shit->mask,shit->from,shit->access,
			shit->expire,shit->time,shit->reason);
	}

	to_file(f,"\n");

	for(ks=current->kicklist;ks;ks=ks->next)
	{
		to_file(f,"kicksay\t\t%s \"%s\" %s\n",ks->chan,ks->mask,ks->reason);
	}

	close(f);

#ifdef DEBUG
	dodebug = dodeb;
#endif /* DEBUG */
	return(TRUE);
}

/*
 *
 *  adding and removing masks from user records
 *
 */

void addmasktouser(User *user, char *mask)
{
	Strp	*um,**pp;

	pp = &user->mask;
	while(*pp)
	{
		um = *pp;
		if (!Strcasecmp(um->p,mask))
			return;
		pp = &um->next;
	}

	set_mallocdoer(addmasktouser);
	*pp = um = (Strp*)Malloc(sizeof(Strp) + strlen(mask));
	um->next = NULL;
	Strcpy(um->p,mask);
}

void delmaskfromuser(User *user, char *mask)
{
	Strp	*um,**pp;

	pp = &user->mask;
	while(*pp)
	{
		um = *pp;
		if (!Strcasecmp(um->p,mask))
		{
			*pp = um->next;
			Free((char**)&um);
			return;
		}
		pp = &um->next;
	}
}

/*
 *
 *  adding, removing, matching and searching for user records
 *
 */

void reset_userlink(User *old, User *new)
{
	Auth	*auth,*nx_auth;
	Chan	*chan;
	ChanUser *cu;
	Client	*client,*nx_client;
	Spy	*spy;

	/*
	 *  auth list
	 */
	for(auth=current->authlist;auth;)
	{
		nx_auth = auth->next;
		if (auth->user == old)
		{
			if (new)
				auth->user = new;
			else
				remove_auth(auth);
		}
		auth = nx_auth;
	}

	/*
	 *  client list
	 */
	for(client=current->clientlist;client;)
	{
		nx_client = client->next;
		if (client->user == old)
		{
			if (new)
				client->user = new;
			else
				delete_client(client);
		}
		client = nx_client;
	}

	/*
	 *  spy list
	 */
	if (new)
	{
		for(spy=current->spylist;spy;spy=spy->next)
		{
			if (spy->dest == old->name)
				spy->dest = new->name;
		}
	}

	/*
	 *  channel userlists
	 */
	for(chan=current->chanlist;chan;chan=chan->next)
	{
		for(cu=chan->users;cu;cu=cu->next)
		{
			if (cu->user == old)
				cu->user = new;
		}
	}
}

void remove_user(User *user)
{
	User	**pp;
	Strp	*ump,*nxt;

	pp = &current->userlist;
	for(;(*pp);pp=&(*pp)->next)
	{
		if (*pp == user)
		{
			*pp = user->next;
#ifdef GREET
			Free((char**)&user->greet);
#endif /* GREET */
			for(ump=user->mask;ump;)
			{
				nxt = ump->next;
				Free((char**)&ump);
				ump = nxt;
			}
#ifdef NOTE
			for(ump=user->note;ump;)
			{
				nxt = ump->next;
				Free((char**)&ump);
				ump = nxt;
			}
#endif /* NOTE */
			Free((char**)&user);
			current->ul_save++;
			return;
		}
	}
}

User *add_user(char *handle, char *chan, char *pass, int axs)
{
	User	*user;
	char	*p;
	int	sz;

#ifdef DEBUG
	debug("(add_user) handle = %s; chan = %s; pass = %s; axs = %i\n",
		nullstr(handle),nullstr(chan),nullstr(pass),axs);
#endif /* DEBUG */

	sz = sizeof(User) + strlen(handle);
	if (chan)
		sz += strlen(chan);
	if (pass)
		sz += strlen(pass);

	set_mallocdoer(add_user);
	user = (User*)Calloc(sz);
	user->access = axs;
	user->next = current->userlist;
	current->userlist = user;

	/*
	 *  "name\0chan\0pass\0"
	 */
	p = Strcpy(user->name,handle) + 1;
	if (chan)
	{
		user->chan = p;
		p = Strcpy(user->chan,chan) + 1;
	}
	if (pass)
	{
		user->pass = p;
		Strcpy(user->pass,pass);
	}
	current->ul_save++;
	return(user);
}

/*
 *  find the user record for a named handle
 */
User *find_handle(char *handle)
{
	User 	*user;

	for(user=current->userlist;user;user=user->next)
	{
		if (!Strcasecmp(handle,user->name))
			return(user);
	}
	return(NULL);
}

/*
 *  Find the user that best matches the userhost
 */
User *find_user(char *userhost, char *channel)
{
	Strp	*ump;
	User	*user,*save;
	int	num,best;

	if (CurrentDCC && CurrentDCC->user->name == userhost)
	{
		user = CurrentDCC->user;
		if (!channel || *user->chan == '*' || !Strcasecmp(channel,user->chan))
			return(user);
		return(NULL);
	}

	save = NULL;
	best = 0;
	for(user=current->userlist;user;user=user->next)
	{
		if (!channel || *user->chan == '*' || !Strcasecmp(channel,user->chan))
		{
			for(ump=user->mask;ump;ump=ump->next)
			{
				num = num_matches(ump->p,userhost);
				if (num > best)
				{
					best = num;
					save = user;
				}
			}
		}
	}
	return(save);
}

int get_userlevel(char *userhost, char *channel)
{
	User	*user;

	if (CurrentDCC && CurrentDCC->user->name == userhost)
	{
		user = CurrentDCC->user;
		if (!channel || *user->chan == '*' || !Strcasecmp(channel,user->chan))
			return(user->access);
		return(0);
	}

	if (is_localbot(userhost))
		return(BOTLEVEL);

	if ((user = find_user(userhost,channel)) == NULL)
		return(0);
	return(user->access);
}

int get_maxuserlevel(char *userhost)
{
	Strp	*ump;
	User	*user;
	int	ulevel;

	if (CurrentDCC && CurrentDCC->user->name == userhost)
		return(CurrentDCC->user->access);

	if (is_localbot(userhost))
		return(BOTLEVEL);

	ulevel = 0;
	for(user=current->userlist;user;user=user->next)
	{
		for(ump=user->mask;ump;ump=ump->next)
		{
			if (!matches(ump->p,userhost))
				ulevel = (user->access > ulevel) ? user->access : ulevel;
		}
	}
	return(ulevel);
}

int is_localbot(char *nuh)
{
	Mech	*bot;

	for(bot=botlist;bot;bot=bot->next)
	{
		if (!nickcmp(nuh,bot->nick))
			return(TRUE);
	}
	return(FALSE);
}

int is_user(char *userhost, char *channel)
{
	User	*user;

	if (is_localbot(userhost))
		return(TRUE);
	if ((user = find_user(userhost,channel)) == NULL)
		return(FALSE);
	return(user->access);
}

/*
 *  FIXME: does this apply to local bots?
 */
int get_protuseraccess(Chan *chan, char *userhost)
{
	Strp	*ump;
	User	*user;
	int	prot;

	prot = 0;
	for(user=current->userlist;user;user=user->next)
	{
		if (*user->chan != '*' && Strcasecmp(chan->name,user->chan))
			continue;

		for(ump=user->mask;ump;ump=ump->next)
		{
			if (!matches(userhost,ump->p))
			{
				if (user->prot > prot)
					prot = user->prot;
				break;
			}
		}
	}
	return(prot);
}

/*
 *
 *  user commands related to userlist
 *
 */

void do_userlist(COMMAND_ARGS)
{
	Strp	*ump;
	User	*user;
	char	*chan,*mask;
	int	minlevel = 0;
	int	maxlevel = BOTLEVEL;
	int	botsonly = FALSE;
	int	chanonly = FALSE;
	int	show,count,cmdok;

	chan = NULL;
	mask = NULL;

	if (rest)
	{
		cmdok = FALSE;
		if (*rest == '+')
		{
			rest = &rest[1];
			if ((*rest >= '0') && (*rest <= '9'))
			{
				minlevel = a2i(rest);
				cmdok = TRUE;
			}
		}
		else
		if (*rest == '-')
		{
			rest = &rest[1];
			if ((*rest >= '0') && (*rest <= '9'))
			{
				maxlevel = a2i(rest);
				cmdok = TRUE;
			}
			if ((*rest == 'B') || (*rest == 'b'))
			{
				botsonly = TRUE;
				cmdok = TRUE;
			}
			if ((*rest == 'C') || (*rest == 'c'))
			{
				chanonly = TRUE;
				cmdok = TRUE;
			}
		}
		else
		if ((*rest == '#') || (*rest == '&'))
		{
			chan = rest;
			cmdok = TRUE;
		}
		else
		if (STRCHR(rest,'*') != NULL)
		{
			mask = rest;
			cmdok = TRUE;
		}
		if (!cmdok)
		{
			usage(from);	/* usage for CurrentCmd->name */
			return;
		}
	}

#ifdef DEBUG
	debug("(do_userlist) mask=%s minlevel=%i maxlevel=%i botsonly=%s chanonly=%s\n",
		(mask) ? mask : "NOMASK",minlevel,maxlevel,
		(botsonly) ? "Yes" : "No",(chanonly) ? "Yes" : "No");
#endif /* DEBUG */

	count = 1;
	for(user=current->userlist;user;user=user->next)
	{
		show = TRUE;
		if (user->access < minlevel)
			show = FALSE;
		if (user->access > maxlevel)
			show = FALSE;
		if (chan && Strcasecmp(chan,user->chan) && *user->chan != '*')
			show = FALSE;
		if (mask)
		{
			for(ump=user->mask;ump;)
			{
				if (matches(mask,ump->p))
				{
					show = FALSE;
					break;
				}
				ump = ump->next;
			}
		}
		if (botsonly && (user->access != BOTLEVEL))
			show = FALSE;
		if (chanonly && *user->chan == '*')
			show = FALSE;
		if (show)
		{
#ifdef BOUNCE
#define BOUNCE_FMT	"%s/"
#else
#define BOUNCE_FMT	/* nothing */
#endif /* BOUNCE */
			to_user(from,"User   : %-11s   [%3i/%s/%s/" BOUNCE_FMT "%s/P%d]   C:%s",
				user->name,(int)user->access,
				(user->aop)    ?  "AO" : "--",
				(user->avoice) ?  "AV" : "--",
#ifdef BOUNCE
				(user->bounce) ? "BNC" : "---",
#endif /* BOUNCE */
				(user->pass)   ?  "PW" : "--",
				user->prot,user->chan);
			if ((ump = user->mask))
			{
				to_user(from,"Mask(s): %s",ump->p);
				while((ump = ump->next))
				{
					to_user(from,"         %s",ump->p);
				}
			}
#ifdef GREET
			if (user->greet)
			{
				to_user(from,"  Greet: %s%s",user->greet,
					(user->greetfile) ? " (greetfile)" : 
					((user->randline) ? " (random line from file)" : ""));
			}
#endif /* GREET */
#ifdef NOTE
			if ((ump = user->note))
			{
				int	n,sz;

				sz = n = 0;
				for(;ump;ump=ump->next)
				{
					if (*ump->p == 1)
						n++;            
					else
						sz += strlen(ump->p);
				}
				to_user(from,"Message: %i message%s (%i bytes)",n,(n == 1) ? "" : "s",sz);
			}
#endif /* NOTE */
			to_user(from," ");
			count++;
		}
	}
	to_user(from,"Total of %d entries",count - 1);
}

void do_host(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Strp	*ump;
	User	*user;
	char	*incmd,*handle,*mask;
	int	ul;

	incmd = chop(&rest);
	handle = chop(&rest);
	mask = chop(&rest);
	if (!mask)
		goto err;
	if ((user = find_handle(handle)) == NULL)
	{
		to_user(from,"Invalid handle");
		return;
	}
	ul = get_userlevel(from,user->chan);
	if ((ul != OWNERLEVEL) && (ul < user->access))
	{
		to_user(from,"Access denied");
		return;
	}
	if (incmd && !Strcasecmp(incmd,"ADD"))
	{
		if ((ump = user->mask))
		{
			while(ump)
			{
				if (!Strcasecmp(ump->p,mask))
				{
					to_user(from,"Mask %s already exists for %s",mask,user->name);
					return;
				}
				ump = ump->next;
			}
		}
#ifdef NEWBIE
		/*
		 *  newbies dont know what they're doing
		 */
		if (!matches(mask,"!@"))
		{
			to_user(from,"Problem adding %s (global mask)",mask);
			return;
		}
		if (matches("*!*@*",mask))
		{
			to_user(from,"Problem adding %s (invalid mask)",mask);
			return;
		}
#endif /* NEWBIE */
		addmasktouser(user,mask);
		to_user(from,"Added mask %s to user %s",mask,user->name);
		current->ul_save++;
		return;
	}
	if (incmd && !Strcasecmp(incmd,"DEL"))
	{
		delmaskfromuser(user,mask);
		to_user(from,"Deleted mask %s from user %s",mask,user->name);
		current->ul_save++;
		return;
	}
err:
	usage(from);	/* usage for CurrentCmd->name */
	return;
}

#define _UNDEF	-1
#define _ADD	1
#define _SUB	2

void do_user(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	User	*user,*doer;
	char	*handle,*pt;
	char	mode;
	int	change,parms;
	int	uaccess,ua;
	int	av,ao,pm,pl,ec;
#ifdef BOUNCE
	int	bc;

	bc =
#endif /* BOUNCE */
	av =
	ao =
	pm =
	uaccess =
	ec = _UNDEF;
	pl = 0;

	/*
	 *  chop wont return NULL (on_msg checks CARGS)
	 */
	handle = chop(&rest);

	if ((user = find_handle(handle)) == NULL)
	{
		to_user(from,TEXT_UNKNOWNUSER,handle);
		return;
	}

	if ((doer = find_user(from,user->chan)) == NULL)
	{
		to_user(from,TEXT_NOACCESSON,user->chan);
		return;
	}

	/*
	 *  lower access cant modif higher access
	 */
	if ((doer->access < user->access) && !superuser(doer))
	{
		to_user(from,TEXT_USEROWNSYOU,user->name);
		return;
	}

	change = FALSE;
	mode   = 0;
	parms  = 0;

	while((pt = chop(&rest)))
	{
		switch(*(pt++))
		{
		case '+':
			mode = _ADD;
			break;
		case '-':
			mode = _SUB;
			break;
		default:
			goto usage;
		}

#ifdef BOUNCE
		if (!Strcasecmp(pt,"BNC"))
			bc = mode;
		else
#endif /* BOUNCE */
		if (!Strcasecmp(pt,"AV"))
			av = mode;
		else
		if (!Strcasecmp(pt,"AO"))
			ao = mode;
		else
		if (!Strcasecmp(pt,"ECHO"))
			ec = mode;
		else
		if (*pt == 'p' || *pt == 'P')
		{
			pt++;
			if ((*pt >= '0') && (*pt <= '4') && (pt[1] == 0))
			{
				pm = mode;
				pl = *pt - '0';
			}
			else
			if ((*pt == 0) && (mode == _SUB))
			{
				pm = mode;
				pl = 0;
			}
			else
			{
				goto usage;
			}
		}
		else
		if ((ua = a2i(pt)) >= 0 && ua <= BOTLEVEL && errno == 0)
		{
			uaccess = ua;
		}
		else
		{
			goto usage;
		}
		parms++;
	}
	if (!parms)
	{
		goto usage;
	}

	/* make the actual changes */

	/* +|- autovoice */
	if ((av == _ADD) && (user->avoice == FALSE))
	{
		user->avoice = TRUE;
		change++;
	}
	if ((av == _SUB) && (user->avoice == TRUE))
	{
		user->avoice = FALSE;
		change++;
	}
	/* +|- autoop */
	if ((ao == _ADD) && (user->aop == FALSE))
	{
		user->aop = TRUE;
		change++;
	}
	if ((ao == _SUB) && (user->aop == TRUE))
	{
		user->aop = FALSE;
		change++;
	}
	/* +|- echo */
	if ((ec == _ADD) && (user->echo == FALSE))
	{
		user->echo = TRUE;
		change++;
	}
	if ((ec == _SUB) && (user->echo == TRUE))
	{
		user->echo = FALSE;
		change++;
	}
	/* +|- protect [0..4] */
	if ((pm == _ADD) && (user->prot != pl))
	{
		user->prot = pl;
		change++;
	}
	if ((pm == _SUB) && (user->prot != 0))
	{
		user->prot = 0;
		change++;
	}
	if (uaccess > _UNDEF && uaccess != user->access)
	{
		user->access = uaccess;
		change++;
	}
#ifdef BOUNCE
	/* +|- bounce */
	if ((bc == _ADD) && (user->bounce == FALSE))
	{
		user->bounce = TRUE;
		change++;
	}
	if ((bc == _SUB) && (user->bounce == TRUE))
	{
		user->bounce = FALSE;
		change++;
	}
#endif /* BOUNCE */

	if (change)
	{
		to_user(from,TEXT_USERCHANGED,user->name);
		current->ul_save++;
	}
	else
	{
		to_user(from,TEXT_USERNOTCHANGED,user->name);
	}
	return;
usage:
	usage(from);	/* usage for CurrentCmd->name */
}

void do_echo(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	char	*tmp;

	tmp = chop(&rest);
	if (!Strcasecmp(tmp,"on"))
	{
		if (CurrentUser->echo == FALSE)
		{
			current->ul_save++;
			CurrentUser->echo = TRUE;
		}
		to_user(from,TEXT_PARTYECHOON);
		return;
	}
	if (!Strcasecmp(tmp,"off"))
	{
		if (CurrentUser->echo == TRUE)
		{
			current->ul_save++;
			CurrentUser->echo = FALSE;
		}
		to_user(from,TEXT_PARTYECHOOFF);
		return;
	}
	usage(from);	/* usage for CurrentCmd->name */
}

/*
 *  Usage: ADD <handle> <channel|*> <nick|mask> <level> [password]
 */
void do_add(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	User	*user;
	char	*name,*chan,*nick,*pass,*anum,*uh,*encpass;
	int	newaccess,uaccess;

	name = chop(&rest);
	chan = chop(&rest);
	nick = chop(&rest);
	anum = chop(&rest);
	pass = chop(&rest);

	newaccess = a2i(anum);
	if (errno)
		goto usage;

	if (*chan == '*')
		chan = NULL;
	else
	if (!ischannel(chan))
		goto usage;

	if ((uaccess = get_userlevel(from,(chan) ? chan : "")) < cmdaccess)
		return;

	/*
	 *  dont create duplicate handles
	 */
	if ((user = find_handle(name)) != NULL)
	{
		to_user(from,"user `%s' already exists",user->name);
		return;
	}

	/*
	 *  check access level
	 */
	if ((newaccess != BOTLEVEL) && ((newaccess < 0) || (newaccess > OWNERLEVEL)))
	{
		to_user(from,"access must be in the range 0 - %i",OWNERLEVEL);
		return;
	}
	if ((uaccess != OWNERLEVEL) && (newaccess > uaccess))
	{
		to_user(from,"access denied on %s",(chan) ? chan : "(global access)");
		return;
	}

	/*
	 *  convert and check nick/mask
	 */
	if ((uh = nick2uh(from,nick)) == NULL)
		return;
	format_uh(uh,FUH_USERHOST);

#ifdef NEWBIE
	if (!matches(uh,"!@"))
	{
		to_user(from,"Problem adding %s (global mask)",uh);
		return;
	}
	if (matches("*!*@*",uh))
	{
		to_user(from,"Problem adding %s (invalid mask)",uh);
		return;
	}
#endif /* NEWBIE */

	/*
	 *  dont duplicate users
	 */
	if (is_user(uh,chan))
	{
		to_user(from,"%s (%s) on %s is already a user",nick,uh,chan);
		return;
	}

	/*
	 *  encrypt password
	 */
	encpass = (pass) ? makepass(pass) : NULL;

	/*
	 *  passwords for bots are never used
	 */
	if (newaccess == BOTLEVEL)
		encpass = NULL;

	if (!chan)
		chan = MATCH_ALL;

	/*
	 *  add_user() touches current->ul_save for us
	 */
	user = add_user(name,chan,encpass,newaccess);
	addmasktouser(user,uh);
	to_user(from,"%s has been added as %s on %s",name,uh,chan);
	to_user(from,"Access level: %i%s%s",newaccess,(pass) ? "  Password: " : "",(pass) ? pass : "");
#ifdef NEWUSER_SPAM
	if ((newaccess != BOTLEVEL) && find_nuh(nick))
	{
		/*
		 *  yay! its a nick! we can spam them!
		 */
		char	cmdchar = current->setting[CHR_CMDCHAR].char_var;

		to_server("NOTICE %s :%s has blessed you with %i levels of immortality\n",
			nick,CurrentNick,uaccess);
		to_server("NOTICE %s :My command character is %c\n",nick,cmdchar);
		to_server("NOTICE %s :Use \026%c%s\026 for command help\n",nick,cmdchar,C_HELP);
		if (encpass)
		{
			to_server("NOTICE %s :Password necessary for doing commands: %s\n",nick,pass);
			to_server("NOTICE %s :If you do not like your password, use \026%c%s\026 to change it\n",
				nick,cmdchar,C_PASSWD);
		}
	}
#endif /* NEWUSER_SPAM */
	return;
usage:
	usage(from);	/* usage for CurrentCmd->name */
}

void do_del(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	User	*user;
	char	*nick;
	int	ulevel;

	nick = chop(&rest);
	if (!nick)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}
	if ((user = find_handle(nick)) == NULL)
	{
		to_user(from,"Unknown handle");
		return;
	}
	ulevel = get_userlevel(from,user->chan);
	if ((user->access == BOTLEVEL) && (ulevel == OWNERLEVEL))
	{
		to_user(from,"Deleting bot %s",nick);
	}
	else
	{
		if (ulevel < user->access)
		{
			to_user(from,"%s has a higher immortality level than you on %s",nick,user->chan);
			return;
		}
	}
	to_user(from,"User %s has been purged",nick);
	/*
	 *  delete all references to the user record
	 */
	reset_userlink(user,NULL);
	/*
	 *  remove_user() touches current->ul_save for us
	 */
	remove_user(user);
}

void change_pass(User *user, char *pass)
{
	User	*new,**uptr;

	/*
	 *  password is stuck in a solid malloc in a linked list
	 *  add_user() touches current->ul_save for us
	 */
	new = add_user(user->name,user->chan,makepass(pass),user->access);

	uptr = &new->next;
	while(*uptr)
	{
		if (*uptr == user)
		{
			*uptr = user->next;
			break;
		}
		uptr = &(*uptr)->next;
	}
	new->echo      = user->echo;
	new->aop       = user->aop;
	new->avoice    = user->avoice;
	new->prot      = user->prot;
#ifdef GREET
	new->greet     = user->greet;
	new->greetfile = user->greetfile;
	new->randline  = user->randline;
#endif /* GREET */
#ifdef BOUNCE
	new->bounce    = user->bounce;
#endif /* BOUNCE */

	new->mask = user->mask;

#ifdef NOTE
	new->note = user->note;
#endif /* NOTE */

	reset_userlink(user,new);
	Free((char**)&user);
}

void do_passwd(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	User	*user;
	char	*pass,*savedpass = NULL;

	pass = chop(&rest);
	if (!pass || !*pass)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}
	if ((user = find_user(from,NULL)) == NULL)
		return;
	if (user->pass)
	{
		savedpass = pass;
		pass = chop(&rest);
		if (!pass || !*pass)
		{
			usage(from);	/* usage for CurrentCmd->name */
			return;
		}
	}
	if (strlen(pass) < MINPASSCHARS)
	{
		to_user(from,"password too short");
		return;
	}
	if (strlen(pass) > MAXPASSCHARS)
	{
		to_user(from,"password too long");
		return;
	}
	if (user->pass && !passmatch(savedpass,user->pass))
	{
		to_user(from,"password incorrect");
		return;
	}
	to_user(from,"new password has been set");
	/*
	 *  all is well
	 *  change_pass() -> add_user() and current->ul_save is touched
	 */
	change_pass(user,pass);
}

void do_setpass(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	User	*user;
	char	*nick,*pass;
	int	ul;

	nick = chop(&rest);
	pass = chop(&rest);

	if (!nick || !*nick || !pass || !*pass)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}
	if (strlen(pass) < MINPASSCHARS)
	{
		to_user(from,"password must be at least %i characters long",MINPASSCHARS);
		return;
	}
	if (strlen(pass) >= MAXPASSCHARS)
		pass[MAXPASSCHARS] = 0;
	if ((user = find_handle(nick)) == NULL)
	{
		to_user(from,"unknown user");
		return;
	}
	ul = get_userlevel(from,user->chan);
	if (ul < user->access)
	{
		to_user(from,"access denied");
		return;
	}
	if (!Strcasecmp(pass,"none"))
	{
		if (ul < ASSTLEVEL)
		{
			to_user(from,"access denied");
			return;
		}
		user->pass = NULL;
		to_user(from,"password for %s has been removed",user->name);
		return;
	}
	to_user(from,"new password for %s has been set",user->name);
	/*
	 *  all is well
	 *  change_pass() -> add_user() and current->ul_save is groped
	 */
	change_pass(user,pass);
}
