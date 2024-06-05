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
#define FUNCTION_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"

LS char timebuf[24];		/* max format lentgh == 20+1, round up to nearest longword -> 24 */
LS char idlestr[28];		/* max format lentgh == 24+1, round up to nearest longword -> 28 */

LS const char monlist[12][4] =
{
	"Jan", "Feb", "Mar", "Apr",
	"May", "Jun", "Jul", "Aug",
	"Sep", "Oct", "Nov", "Dec"
};

LS const char daylist[7][4] =
{
	"Sun", "Mon", "Tue", "Wed",
	"Thu", "Fri", "Sat"
};

/*
 *  memory allocation routines
 */

#ifdef DEBUG

void *Calloc(int size)
{
	aME	*mmep;
	aMEA	*mp;
	int	i;

#if 1
	if (mallocdoer == NULL)
	{
		mallocdoer = __builtin_return_address(0);
		debug("(Calloc) mallocdoer = "mx_pfmt"\n",(mx_ptr)mallocdoer);
		mallocdoer = NULL;
	}
#endif

	mmep = NULL;
	mp = mrrec;
	while(!mmep)
	{
		for(i=0;i<MRSIZE;i++)
		{
			if (mp->mme[i].area == NULL)
			{
				mmep = &mp->mme[i];
				break;
			}
		}
		if (!mmep)
		{
			if (mp->next == NULL)
			{
				mp->next = calloc(sizeof(aMEA),1);
				mmep = &mp->next->mme[0];
			}
			else
				mp = mp->next;
		}
	}

	if ((mmep->area = (void*)calloc(size,1)) == NULL)
	{
		run_debug();
		exit(1);
	}
	mmep->size = size;
	mmep->when = now;
	mmep->doer = mallocdoer;
	mallocdoer = NULL;
	return((void*)mmep->area);
}

void *Malloc(int size)
{
	aME	*mmep;
	aMEA	*mp;
	int	i;

#if 1
	if (mallocdoer == NULL)
	{
		mallocdoer = __builtin_return_address(0);
		debug("(Malloc) mallocdoer = "mx_pfmt"\n",(mx_ptr)mallocdoer);
		mallocdoer = NULL;
	}
#endif

	mmep = NULL;
	mp = mrrec;
	while(!mmep)
	{
		for(i=0;i<MRSIZE;i++)
		{
			if (mp->mme[i].area == NULL)
			{
				mmep = &mp->mme[i];
				break;
			}
		}
		if (!mmep)
		{
			if (mp->next == NULL)
			{
				mp->next = calloc(sizeof(aMEA),1);
				mmep = &mp->next->mme[0];
			}
			else
				mp = mp->next;
		}
	}

	if ((mmep->area = (void*)malloc(size)) == NULL)
	{
		run_debug();
		exit(1);
	}
	mmep->size = size;
	mmep->when = now;
	mmep->doer = mallocdoer;
	mallocdoer = NULL;
	return((void*)mmep->area);
}

void Free(char **mem)
{
	aME	*mmep;
	aMEA	*mp;
	int	i;

	if (*mem == NULL)
		return;

	mmep = NULL;
	mp = mrrec;
	while(!mmep)
	{
		for(i=0;i<MRSIZE;i++)
		{
			if (mp->mme[i].area == *mem)
			{
				mmep = &mp->mme[i];
				break;
			}
		}
		if (!mmep)
		{
			if (mp->next == NULL)
			{
				debug("(Free) PANIC: Free(0x"mx_pfmt"); Unregistered memory block\n",(mx_ptr)*mem);
				run_debug();
				exit(1);
			}
			mp = mp->next;
		}
	}

	mmep->area = NULL;
	mmep->size = 0;
	mmep->when = (time_t)0;
	free(*mem);
	*mem = NULL;
}

#else /* DEBUG */

void *Calloc(int size)
{
	void	*tmp;

	if ((tmp = (void*)calloc(size,1)) == NULL)
		exit(1);
	return((void*)tmp);
}

void *Malloc(int size)
{
	void	*tmp;

	if ((tmp = (void*)malloc(size)) == NULL)
		exit(1);
	return((void*)tmp);
}

/*
 *  Free() can be called with NULL's
 */
void Free(char **mem)
{
	if (*mem)
	{
		free(*mem);
		*mem = NULL;
	}
}

#endif /* DEBUG */

char *nickcpy(char *dest, const char *nuh)
{
	char	*ret;

	if (!dest)
		dest = nick_buf;
	ret = dest;

	while(*nuh && (*nuh != '!'))
		*(dest++) = *(nuh++);
	*dest = 0;

	return(ret);
}

char *getuh(char *nuh)
{
	char	*s;

	s = nuh;
	while(*s)
	{
		if (*s == '!')
		{
			nuh = s + 1;
			/*
			 *  We have to grab everything from the first '!' since some
			 *  braindamaged ircds allow '!' in the "user" part of the nuh
			 */
			break;
		}
		s++;
	}
	return(nuh);
}

/*
 *  caller is responsible for:
 *
 *    src != NULL
 *   *src != NULL
 *
 */
char *get_token(char **src, const char *token_sep)
{
	const char *s;
	char	*token = NULL;

	/*
	 *  skip past any token_sep chars in the beginning
	 */

	if (0)		/* is this legal C? */
a:	++(*src);
	s = token_sep;
	while(**src && *s)
	{
		if (*(s++) == **src)
			goto a;
	}

	if (token || **src == 0)
		return(token);

	token = *src;

	/*
	 *  find the next token_sep char
	 */
	do {
		s = token_sep;
		do {
			if (*s == **src)
			{
				**src = 0;
				goto a;
			}
			++s;
		}
		while(*s);
		(*src)++;
	}
	while(**src);

	return(token);
}

/*
 *  time to string routines
 */

char *logtime(time_t when)
{
	struct	tm *btime;

	btime = localtime(&when);
	sprintf(timebuf,"%s %i %i %02i:%02i:%02i",	/* max format length: 20+1 */
		monlist[btime->tm_mon],btime->tm_mday,btime->tm_year+1900,
		btime->tm_hour,btime->tm_min,btime->tm_sec);
	return(timebuf);
}

char *time2str(time_t when)
{
	struct	tm *btime;

	if (!when)
		return(NULL);

	btime = localtime(&when);
	sprintf(timebuf,"%02i:%02i:%02i %s %02i %i",	/* max format length: 20+1 */
		btime->tm_hour,btime->tm_min,btime->tm_sec,monlist[btime->tm_mon],
		btime->tm_mday,btime->tm_year+1900);
	return(timebuf);
}

char *time2away(time_t when)
{
	struct	tm *btime;
	char	ampm;

	if (!when)
		return(NULL);

	btime = localtime(&when);
	if (btime->tm_hour < 12)
	{
		if (btime->tm_hour == 0)
			btime->tm_hour = 12;
		ampm = 'a';
	}
	else
	{
		if (btime->tm_hour != 12)
			btime->tm_hour -= 12;
		ampm = 'p';
	}

	sprintf(timebuf,"%i:%02i%cm %s %s %i",		/* max format length: 18+1 */
		btime->tm_hour,btime->tm_min,ampm,daylist[btime->tm_wday],
		monlist[btime->tm_mon],btime->tm_mday);
	return(timebuf);
}

char *time2medium(time_t when)
{
	struct	tm *btime;

	btime = localtime(&when);
	sprintf(timebuf,"%02i:%02i",			/* max format length: 5+1 */
		btime->tm_hour,btime->tm_min);
	return(timebuf);
}

char *time2small(time_t when)
{
	struct	tm *btime;

	btime = localtime(&when);
	sprintf(timebuf,"%s %02i",			/* max format length: 6+1 */
		monlist[btime->tm_mon],btime->tm_mday);
	return(timebuf);
}

char *idle2str(time_t when, int small)
{
	int	d,h,m,s;

	d = when / 86400;
	h = (when -= d * 86400) / 3600;
	m = (when -= h * 3600) / 60;
	s = when % 60;

	if (small)
		sprintf(idlestr,"%i d, %i h, %i m, %i s",d,h,m,s);			/* 24+1 (up to 9999 days) */
	else
		sprintf(idlestr,"%i day%s %02i:%02i:%02i",d,EXTRA_CHAR(d),h,m,s);	/* 18+1 (up to 9999 days) */
	return(idlestr);
}

char *get_channel(char *to, char **rest)
{
	char	*channel;

	if (*rest && ischannel(*rest))
	{
		channel = chop(rest);
	}
	else
	{
		if (!ischannel(to) && current->activechan)
			channel = current->activechan->name;
		else
			channel = to;
	}
	return(channel);
}

char *get_channel2(char *to, char **rest)
{
	char	*channel;

	if (*rest && (**rest == '*' || ischannel(*rest)))
	{
		channel = chop(rest);
	}
	else
	{
		if (!ischannel(to) && current->activechan)
			channel = current->activechan->name;
		else
			channel = to;
	}
	return(channel);
}

char *cluster(char *hostname)
{
	char	mask[NUHLEN];
	char	*p,*host;
	char	num,dot;

	host = p = hostname;
	num = dot = 0;
	while(*p)
	{
		if (*p == '@')
		{
			host = p + 1;
			num = dot = 0;
		}
		else
		if (*p == '.')
			dot++;
		else
		if (*p < '0' || *p > '9')
			num++;
		p++;
	}

	if (!num && (dot == 3))
	{
		/*
		 *  its a numeric IP address
		 *  1.2.3.4 --> 1.2.*.*
		 */
		p = mask;
		while(*host)
		{
			if (*host == '.')
			{
				if (num)
					break;
				num++;
			}
			*(p++) = *(host++);
		}
		Strcpy(p,".*.*");
	}
	else
	{
		/*	
		 *  its not a numeric mask
		 */
		p = mask;
		*(p++) = '*';
		num = (dot >= 4) ? 2 : 1;
		while(*host)
		{
			if (*host == '.')
				dot--;
			if (dot <= num)
				break;
			host++;
		}
		Strcpy(p,host);
	}
	Strcpy(hostname,mask);
	return(hostname);
}

/*
 *  type   output
 *  ~~~~   ~~~~~~
 *  0,1    *!*user@*.host.com
 *  2      *!*@*.host.com
 */
char *format_uh(char *userhost, int type)
{
	char	tmpmask[NUHLEN];
	char	*u,*h;

	if (STRCHR(userhost,'*'))
		return(userhost);

	Strcpy(tmpmask,userhost);

	h = tmpmask;
	    get_token(&h,"!");	/* discard nickname */
	u = get_token(&h,"@");

	if (*h == 0)
		return(userhost);

	if (u && (type < 2))
	{
		if ((type = strlen(u)) > 9)
			u += (type - 9);
		else
		if (*u == '~')
			u++;
	}
	sprintf(userhost,"*!*%s@%s",(u) ? u : "",cluster(h));
	return(userhost);
}

/*
 *  NOTE! beware of conflicts in the use of nuh_buf, its also used by find_nuh()
 */
char *nick2uh(char *from, char *userhost)
{
	if (STRCHR(userhost,'!') && STRCHR(userhost,'@'))
	{
		Strcpy(nuh_buf,userhost);
	}
	else
	if (!STRCHR(userhost,'!') && !STRCHR(userhost,'@'))
	{
		/* find_nuh() stores nickuserhost in nuh_buf */
		if (find_nuh(userhost) == NULL)
		{
			if (from)
				to_user(from,"No information found for %s",userhost);
			return(NULL);
		}
	}
	else
	{
		Strcpy(nuh_buf,"*!");
		if (!STRCHR(userhost,'@'))
			Strcat(nuh_buf,"*@");
		Strcat(nuh_buf,userhost);
	}
	return(nuh_buf);
}

void deop_ban(Chan *chan, ChanUser *victim, char *mask)
{
	if (!mask)
		mask = format_uh(get_nuh(victim),FUH_USERHOST);

	send_mode(chan,85,QM_CHANUSER,'-','o',victim);
	send_mode(chan,90,QM_RAWMODE,'+','b',mask);
}

void deop_siteban(Chan *chan, ChanUser *victim)
{
	char	*mask;

	mask = format_uh(get_nuh(victim),FUH_HOST);
	deop_ban(chan,victim,mask);
}

void screwban_format(char *userhost)
{
	int	sz,n,pos;

#ifdef DEBUG
	debug("(screwban_format) %s\n",userhost);
#endif /* DEBUG */

	if ((sz = strlen(userhost)) < 8)
		return;

	n = RANDOM(4,sz);
	while(--n)
	{
		pos = RANDOM(0,(sz - 1));
		if (!STRCHR("?!@*",userhost[pos]))
		{
			userhost[pos] = (RANDOM(0,3) == 0) ? '*' : '?';
		}
	}
}

void deop_screwban(Chan *chan, ChanUser *victim)
{
	char	*mask;
	int	i;

	for(i=2;--i;)
	{
		mask = format_uh(get_nuh(victim),FUH_USERHOST);
		screwban_format(mask);
		deop_ban(chan,victim,mask);
	}
}

int isnick(char *nick)
{
	uchar	*p;

	p = (uchar*)nick;
	if ((attrtab[*p] & FNICK) != FNICK)
		return(FALSE);
	while(*p)
	{
		if ((attrtab[*p] & NICK) != NICK)
			return(FALSE);
		p++;
	}
	return(TRUE);
}

int capslevel(char *text)
{
	int	sz,upper;

	if (!*text)
		return(0);

	sz = upper = 0;
	while(*text)
	{
		if ((*text >= 'A' && *text <= 'Z') || (*text == '!'))
			upper++;
		sz++;
		text++;
	}
	return((100*upper)/sz);
}

int a2i(char *anum)
{
	int	res,neg;

	errno = EINVAL;

	if (!anum || !*anum)
		return(-1);

	neg = (*anum == '-') ? 1 : 0;
	anum += neg;

	res = 0;
	while(*anum)
	{
		res *= 10;
		if (attrtab[(uchar)*anum] & NUM)
			res += *(anum++) - '0';
		else
			return(-1);
	}
	errno = 0;
	return((neg) ? -res : res);
}

int get_number(char *rest)
{
	int	n = -1;

	while(*rest)
	{
		if (*rest >= '0' && *rest <= '9')
		{
			if (n == -1)
				n = 0;
			n = (n * 10) + (*rest - '0');
		}
		else
		if (n != -1)
			break;
		rest++;
	}
	return(n);
}

void fix_config_line(char *text)
{
	char	*s,*space;

	space = NULL;
	for(s=text;*s;s++)
	{
		if (*s == '\t')
			*s = ' ';
		if (!space && *s == ' ')
			space = s;
		if (space && *s != ' ')
			space = NULL;
	}
	if (space)
		*space = 0;
}

#ifdef SAFEPATH
/*
 *  try to determine if a path is safe or not.
 *  routine written by zip, optimized by proton.
 */
int safepath(char *path)
{
	char	copypath[strlen(path)+1];
	char	*tok;
	int	level = 0;

	if (*path == '/')
		return(0);

	Strcpy(copypath,path);
	path = copypath;

	while(1)
	{
		if ((tok = get_token(&path,"/")) == NULL)
			break;

		if (tok[0] == '.')
		{
			if (tok[1] == '.')
			{
				if (tok[2] == 0)
				{
					if (level)
					{
						level--;
						continue;
					}
					return(0);
				}
			}
			if (tok[1] == 0)
				continue;
		}
		level++;
	}
	return(level);
}
#endif /* SAFEPATH */

/*
 *  returns NULL or non-zero length string
 */
char *chop(char **src)
{
	char	*tok,*cut = *src;

	while(*cut && *cut == ' ')
		cut++;

	if (*cut)
	{
		tok = cut;
		while(*cut && *cut != ' ')
			cut++;
		*src = cut;
		while(*cut && *cut == ' ')
			cut++;
		**src = 0;
		*src = cut;
	}
	else
	{
		tok = NULL;
	}
	return(tok);
}

/*
 *  remove all '\0' in an array bounded by two pointers
 */
void unchop(char *orig, char *rest)
{
	for(;orig<rest;orig++)
	{
		if (*orig == 0)
			*orig = ' ';
	}
}

int Strcasecmp(const uchar *p1, const uchar *p2)
{
	int	ret;

	if (p1 != p2)
	{
		while(!(ret = tolowertab[*(p1++)] - tolowertab[*p2]) && *(p2++))
			;
		return(ret);
	}
	return(0);
}

int Strcmp(const uchar *p1, const uchar *p2)
{
	int	ret;

	if (p1 != p2)
	{
		while(!(ret = *(p1++) - *p2) && *(p2++))
			;
		return(ret);
	}
	return(0);
}

int nickcmp(const uchar *p1, const uchar *p2)
{
	int	ret;
	int	c;

	if (p1 != p2)
	{
		while(!(ret = nickcmptab[*(p1++)] - (c = nickcmptab[*(p2++)])) && c)
			;
		return(ret);
	}
	return(0);
}

void Strncpy(char *dst, const char *src, int sz)
{
	char	*stop = dst + sz - 1;

	while(*src)
	{
		*(dst++) = *(src++);
		if (dst == stop)
			break;
	}
	*dst = 0;
}

char *Strcpy(char *dst, const char *src)
{
	while(*src)
		*(dst++) = *(src++);
	*dst = 0;
	return(dst);
}

char *Strchr(const char *t, int c)
{
	char	ch = c;

	while(*t != ch && *t)
		t++;
	return((*t == ch) ? (char*)t : NULL);
}

char *Strdup(char *src)
{
	char	*dest;

	dest = (char*)Malloc(strlen(src)+1);
	Strcpy(dest,src);
	return(dest);
}

/*
 *  This code might look odd but its optimized for size,
 *  so dont change it!
 */
char *Strcat(char *dst, const char *src)
{
	while(*(dst++))
		;
	--dst;
	while((*(dst++) = *(src++)) != 0)
		;
	return(dst-1);
}

/*
 *  mask matching
 */
int matches(char *mask, char *text)
{
	uchar	*m = (uchar*)mask;
	uchar	*n = (uchar*)text;
	uchar	*orig = mask;
	int	wild,q;

	q = wild = 0;

	if (!m || !n)
		return(TRUE);

loop:
	if (!*m)
	{
		if (!*n)
			return(FALSE);
		for (m--;((*m == '?') && (m > orig));m--)
			;
		if ((*m == '*') && (m > orig) && (m[-1] != '\\'))
			return(FALSE);
		if (wild)
		{
			m = (uchar *)mask;
			n = (uchar *)++text;
		}
		else
			return(TRUE);
	}
	else
	if (!*n)
	{
		while(*m == '*')
			m++;
		return(*m != 0);
	}

	if (*m == '*')
	{
		while (*m == '*')
			m++;
		wild = 1;
		mask = (char *)m;
		text = (char *)n;
	}

	if ((*m == '\\') && ((m[1] == '*') || (m[1] == '?')))
	{
		m++;
		q = 1;
	}
	else
		q = 0;

	if ((tolowertab[(uchar)*m] != tolowertab[(uchar)*n]) && ((*m != '?') || q))
	{
		if (wild)
		{
			m = (uchar *)mask;
			n = (uchar *)++text;
		}
		else
			return(1);
	}
	else
	{
		if (*m)
			m++;
		if (*n)
			n++;
	}
	goto loop;
}

int num_matches(char *mask, char *text)
{
	char	*p = mask;
	int	n;

	n = !matches(mask,text);

	if (n)
	{
		do {
			if (*p != '*')
				n++;
		}
		while(*++p);
	}
	return(n);
}
