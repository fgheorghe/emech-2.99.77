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
#define VARS_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"
#include "settings.h"

void set_str_varc(Chan *channel, int which, char *value)
{
	char	*temp,**dst;

	if (value && *value)
	{
		set_mallocdoer(set_str_varc);
		temp = Strdup(value);
	}
	else
		temp = NULL;
	dst = (channel) ? (char**)&channel->setting[which] : (char**)&current->setting[which];
	if (*dst)
		Free(dst);
	*dst = temp;
}

/*
 *  The rest
 */
int find_setting(char *name)
{
	int	i;

	for(i=0;VarName[i].name;i++)
	{
		if (!Strcasecmp(name,VarName[i].name))
			return(i);
	}
	return(-1);
}

void copy_vars(UniVar *dst, UniVar *src)
{
	int	i;

	for(i=0;i<CHANSET_SIZE;i++)
	{
		if (IsStr(i))
		{
			if (src[i].str_var)
			{
				set_mallocdoer(copy_vars);
				dst[i].str_var = Strdup(src[i].str_var);
			}
		}
		else
		{
			dst[i].int_var = src[i].int_var;
		}
	}	
}

void set_binarydefault(UniVar *dst)
{
	int	i;

	for(i=0;VarName[i].name;i++)
		dst[i].str_var = VarName[i].setto;
}

void delete_vars(UniVar *vars, int which)
{
	while(--which)
	{
		if (IsStr(which) && !IsProc(which))
		{
#ifdef DEBUG
			debug("(delete_vars) deleting string var `%s'\n",VarName[which].name);
#endif /* DEBUG */
			Free((char**)&vars[which].str_var);
		}
	}
#ifdef DEBUG
	debug("(delete_vars) all done\n");
#endif /* DEBUG */
}

char *tolowercat(char *dest, char *src)
{
	dest = STREND(dest);
	while(*src)
	{
		*dest = (char)tolowertab[(uchar)*src];
		dest++;
		src++;
	}
	return(dest);
}


void do_set_report(COMMAND_ARGS)
{
	/*
	 *  on_msg checks:
	 */
	Chan	*chan;
	UniVar	*univar,*varval;
	char	tmp[MSGLEN];
	char	*pp,*channel,*name;
	int	n,which,i,sz,limit;

	channel = get_channel2(to,&rest);
	if (!rest || !*rest)
	{
		if ((chan = find_channel_ny(channel)) == NULL)
		{
			to_user(from,ERR_CHAN,to);
			return;
		}

		if (!CurrentDCC)
			return;
		to_user(from,"\037Global settings\037");

		i = CHANSET_SIZE;
		limit = SIZE_VARS - 1;
		univar = current->setting;
		*tmp = 0;

second_pass:
		for(;i<limit;i++)
		{
			varval = (IsProc(i)) ? current->setting[i].proc_var : &univar[i];

			sz = strlen(tmp) + strlen(VarName[i].name);

			if (IsStr(i))
			{
				sz += (varval->str_var) ? strlen(varval->str_var) : 7;
			}

			if (sz > 58)
			{
				to_user(from,"%s",tmp);
				*tmp = 0;
			}

			if (IsInt(i))
			{
				pp = tolowercat(tmp,VarName[i].name);
				sprintf(pp,(IsChar(i)) ? "=`%c' " : "=%i ",varval->int_var);
			}
			else
			if (IsStr(i))
			{
				pp = tolowercat(tmp,VarName[i].name);
				sprintf(pp,(varval->str_var) ? "=\"%s\" " : "=(unset) ",varval->str_var);
			}
			else
			if (IsTog(i))
			{
				pp = Strcat(tmp,(varval->int_var) ? "+" : "-");
				pp = tolowercat(pp,VarName[i].name);
				pp[0] = ' ';
				pp[1] = 0;
			}
		}
		if (*tmp && tmp[1])
			to_user(from,"%s",tmp);

		if (limit != CHANSET_SIZE)
		{
			to_user(from,"\037Channel settings: %s\037",(chan) ? chan->name : rest);

			i = 0;
			limit = CHANSET_SIZE;
			univar = chan->setting;
			*tmp = 0;

			goto second_pass;
		}
		return;
	}

	/*
	 *  the CARGS that was checked for in on_msg() might have been
	 *  chopped off by get_channel2()
	 */
	name = chop(&rest);
	if (!name || ((which = find_setting(name)) == -1))
	{
set_usage:
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	if ((which < CHANSET_SIZE) && *channel != '*')
	{
		if ((chan = find_channel_ny(channel)) == NULL)
		{
			to_user(from,ERR_CHAN,channel);
			return;
		}
		channel = chan->name;
		varval = &chan->setting[which];
	}
	else
	{
		channel = MATCH_ALL;
		varval = &current->setting[which];
	}

	if (get_authlevel(from,channel) < cmdaccess)
		return;

	/*
	 *  Check each type and process `rest' if needed.
	 */
	n = 0;
	if (IsChar(which))
	{
		if (rest[1])
			goto set_usage;
	}
	else
	if (IsNum(which))
	{
		if (IsTog(which))
		{
			if (!Strcasecmp(rest,"ON"))
			{
				n = 1;
				goto num_data_ok;
			}
			else
			if (!Strcasecmp(rest,"OFF"))
			{
				/* n is 0 by default */
				goto num_data_ok;
			}
		}
		n = a2i((rest = chop(&rest)));
		if (errno || n < VarName[which].min || n > VarName[which].max)
		{
			to_user(from,"Possible values are %i through %i",VarName[which].min,VarName[which].max);
			return;
		}
	}
num_data_ok:
	/*
	 *
	 */
	if ((which < CHANSET_SIZE) && *channel == '*')
	{
		for(chan=current->chanlist;chan;chan=chan->next)
		{
			if (IsNum(which))
			{
				chan->setting[which].int_var = n;
			}
			else
			if (IsStr(which))
			{
				Free((char**)&chan->setting[which].str_var);
				if (*rest)
				{
					set_mallocdoer(do_set_report);
					chan->setting[which].str_var = Strdup(rest);
				}
			}
		}
		channel = "(all channels)";
	}
	else
	{
		if (IsProc(which))
			varval = varval->proc_var;

		if (IsChar(which))
			varval->char_var = *rest;
		else
		if (IsNum(which))
			varval->int_var = n;
		else
		{
			if (varval->str_var)
				Free((char**)&varval->str_var);
			if (*rest)
			{
				set_mallocdoer(do_set_report);
				varval->str_var = Strdup(rest);
			}
		}
	}
	to_user(from,"Var: %s   On: %s   Set to: %s",VarName[which].name,
		(which >= CHANSET_SIZE) ? "(global)" : channel,(*rest) ? rest : NULLSTR);
}
