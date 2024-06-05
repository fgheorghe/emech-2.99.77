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
#define AUTH_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#ifndef MD5CRYPT

/*
 *  simple password encryption
 */

char	pctab[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuwvxuz0123456789+-";

#define CIPHER(a1,a2,a3,a4,b1,b2,b3,b4) \
{					\
	a2 =  a2 ^  a1;			\
	b2 =  b2 ^  b1;			\
	a3 =  a2 ^  a1;			\
	b3 =  b2 ^  b1;			\
	b3 >>= 2;			\
	b3 |= ((a3 & 3) << 30);		\
	a3 >>= 2;			\
	a2 =  a3 ^  a2;			\
	b2 =  b3 ^  b2;			\
	a4 = ~a4 ^  a2;			\
	b4 = -b4 ^  b2;			\
	a2 =  a4 ^ ~a2;			\
	b2 =  b4 ^ -b2;			\
	b1 >>= 1;			\
	b1 |= ((a1 & 1) << 31);		\
	a1 >>= 1;			\
}

char *cipher(char *arg)
{
	static	char res[40];
	ulong	B1a,B2a,B3a,B4a;
	ulong	B1b,B2b,B3b,B4b;
	uchar	*ptr;
	ulong	R1;
	int	i;

	if (!arg || !*arg)
		return(NULL);
	
	B1a = B2a = B3a = B4a = 0;
	B1b = B2b = B3b = B4b = 0;
	ptr = arg;

	while(*ptr)
	{
		R1 = *ptr;
		for(i=0;i<8;i++)
		{
			if (R1 & 1)
			{
				B1a |= 0x80008000;
				B1b |= 0x80008000;
			}
			R1  >>= 1;
			CIPHER(B1a,B2a,B3a,B4a,B1b,B2b,B3b,B4b);
		}
		ptr++;
	}
	while((B1a) || (B1b))
	{
		CIPHER(B1a,B2a,B3a,B4a,B1b,B2b,B3b,B4b);
	}

	for(i=0;i<10;i++)
	{
		res[i] = pctab[(B4b & 0x3f)];
		B4b >>= 6;
		B4b |= ((B4a & 0x3f) << 26);
		B4a >>= 6;
	}
	res[i] = 0;
	return(res);
}

char *makepass(char *plain)
{
	return(cipher(plain));
}

int passmatch(char *plain, char *encoded)
{
	return(!Strcmp(cipher(plain),encoded));
}

#else /* MD5CRYPT */

/*
 *  use MD5 to hash passwords
 */

char *crypt(char *, char *);

char *makepass(char *plain)
{
	char	salt[8];

	sprintf(salt,"$1$%04x",(rand() >> 16));
	return(crypt(plain,salt));
}

int passmatch(char *plain, char *encoded)
{
	char	*enc;

	if (matches("$1$????$*",encoded))
		return(FALSE);
	enc = crypt(plain,encoded);
	return(!Strcmp(enc,encoded));
}

#endif /* MD5CRYPT */

/*
 *
 */
void delete_auth(char *userhost)
{
	Auth	*auth,**pp;

	pp = &current->authlist;
	while(*pp)
	{
		if (!Strcasecmp(userhost,(*pp)->nuh))
		{
			auth = *pp;
			*pp = auth->next;
			Free((char**)&auth);
			/*
			 *  dont return yet, there might be more auths
			 */
			continue;
		}
		pp = &(*pp)->next;
	}
}

void remove_auth(Auth *auth)
{
	Auth	**pp;

	pp = &current->authlist;
	while(*pp)
	{
		if (*pp == auth)
		{
			*pp = auth->next;
			Free((char**)&auth);
			/*
			 *  when removing a DCC/telnet auth the connection should also be removed
			 */
			return;
		}
		pp = &(*pp)->next;
	}
}

/*
 *  register nick-changes in auths
 */
void change_authnick(char *nuh, char *newnuh)
{
	Auth	*auth,*oldauth,**pp;
	char	*n1,*n2;

	pp = &current->authlist;
	while(*pp)
	{
		auth = *pp;
		if (!nickcmp(nuh,auth->nuh))
		{
			for(n1=nuh;*n1 != '!';n1++);
			for(n2=newnuh;*n2 != '!';n2++);
			if ((n1 - nuh) >= (n2 - newnuh))
			{
#ifdef DEBUG
				debug("(change_authnick) auth->nuh = `%s'; was `%s' (Strcpy) (L1 = %i, L2 = %i)\n",
					newnuh,nuh,(int)(n1 - nuh),(int)(n2 - newnuh));
#endif /* DEBUG */
				Strcpy(auth->nuh,newnuh);
			}
			else
			{
#ifdef DEBUG
				debug("(change_authnick) auth->nuh = `%s'; was `%s' (re-alloc)\n",newnuh,nuh);
#endif /* DEBUG */
				/*
				 *  de-link the old auth record
				 */
				oldauth = auth;
				*pp = auth->next;

				set_mallocdoer(change_authnick);
				auth = (Auth*)Calloc(sizeof(Auth) + strlen(newnuh));
				auth->user = oldauth->user;
				auth->active = now;
				auth->next = current->authlist;
				current->authlist = auth;
				Strcpy(auth->nuh,newnuh);
				Free((char**)&oldauth);
			}
			return;
		}
		pp = &(*pp)->next;
	}
}

LS User *au_user;
LS char *au_userhost;
LS char *au_channel;
LS int au_access;

void aucheck(User *user)
{
	Strp	*ump;

	if (au_channel && (*user->chan != '*') && Strcasecmp(au_channel,user->chan))
		return;

	for(ump=user->mask;ump;ump=ump->next)
	{
		if (!matches(ump->p,au_userhost))
		{
			if (au_access < user->access)
			{
				au_access = user->access;
				au_user = user;
			}
			return;
		}
	}
}

User *get_authuser(char *userhost, char *channel)
{
	Chan	*chan;
	User	*user;
	Auth	*auth;
	int	enfpass = 0;

	au_user = NULL;
	au_userhost = userhost;
	au_channel = channel;
	au_access = 0;

	/*
	 *  people who are authenticated
	 */
	for(auth=current->authlist;auth;auth=auth->next)
	{
		if (au_access < auth->user->access)
		{
			if (!Strcasecmp(userhost,auth->nuh))
			{
				aucheck(auth->user);
			}
		}
	}
	if (channel)
	{
		if ((chan = find_channel_ny(channel)) != NULL)
		{
			enfpass = chan->setting[TOG_ENFPASS].int_var;
		}
	}
	if (enfpass == 0)
	{
		/*
		 *  people who dont need a password
		 */
		for(user=current->userlist;user;user=user->next)
		{
			if (!user->pass && (au_access < user->access))
			{
				aucheck(user);
			}
		}
	}
	return(au_user);
}

int get_authlevel(char *userhost, char *channel)
{
	User	*user;

	if (CurrentDCC && CurrentDCC->user->name == userhost)
	{
		user = CurrentDCC->user;
		if (!channel || *user->chan == '*' || !Strcasecmp(channel,user->chan))
		{
#ifdef NO_DEBUG
			debug("[GAL] %s on %s = %i\n",nullstr(userhost),nullstr(channel),user->access);
#endif /* DEBUG */
			return(user->access);
		}
#ifdef NO_DEBUG
		debug("[GAL] %s on %s = 0\n",nullstr(userhost),nullstr(channel));
#endif /* DEBUG */
		return(0);
	}
	if (is_localbot(userhost))
		return(BOTLEVEL);

	get_authuser(userhost,channel);

#ifdef NO_DEBUG
	debug("[GAL] %s on %s = %i\n",nullstr(userhost),nullstr(channel),au_access);
#endif /* DEBUG */
	return(au_access);
}

/*
 *
 *  authentication commands
 *
 */

/*
 *  Usage: VERIFY <password>
 */
void do_auth(COMMAND_ARGS)
{
	Auth	*auth;
	Chan	*chan;
	ChanUser *cu;
	Strp	*ump;
	User	*user;
	char	*pass;
	int	hostmatch;

	/*
	 *  we dont send a usage line for the VERIFY command
	 */
	if (!rest)
		return;

	pass = chop(&rest);

	/*
	 *  chop chop
	 */
	if (strlen(pass) > MAXPASSCHARS)
		pass[MAXPASSCHARS] = 0;

	hostmatch = FALSE;
	/*
	 *  find a matching password
	 */
	for(user=current->userlist;user;user=user->next)
	{
		for(ump=user->mask;ump;ump=ump->next)
		{
			if (!matches(ump->p,from))
			{
				if (user->pass && passmatch(pass,user->pass))
					goto listcheck;
				hostmatch = TRUE;
			}
		}
	}

	/*
	 *  if there was a matching host, they get a message...
	 */
	if (hostmatch)
		to_user(from,"Incorrect password, not authorized");
	return;

listcheck:
	for(auth=current->authlist;auth;auth=auth->next)
	{
		if ((auth->user == user) && !Strcasecmp(auth->nuh,from))
		{
			to_user(from,"You have already been authorized");
			return;
		}
	}

	/*
	 *  there is no matching auth for this user, we add one
	 */
	set_mallocdoer(do_auth);
	auth = (Auth*)Malloc(sizeof(Auth) + strlen(from));
	auth->user = user;
	auth->active = now;
	Strcpy(auth->nuh,from);

	auth->next = current->authlist;
	current->authlist = auth;

	to_user(from,"You are now officially immortal");

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if (!chan->bot_is_op)
			continue;
		if (!chan->setting[TOG_AOP].int_var)
			continue;
		if ((cu = find_chanuser(chan,from)) == NULL)
			continue;
		if (cu->user && cu->user->aop)
		{
			send_mode(chan,120,QM_CHANUSER,'+','o',(void*)cu);
		}
	}
}
