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
#ifndef TEXT_H
#define TEXT_H 1

/*
 *  These are more or less globally used..
 */

#define TEXT_NOTINSERVLIST	"(not in serverlist)"
#define TEXT_NONE		"(none)"

#define TEXT_NAMETOOLONG	"Hostname exceeds maximum length"

#define TEXT_LISTSAVED		"Lists saved to file %s"
#define TEXT_LISTREAD		"Lists read from file %s"
#define ERR_NOSAVE		"Lists could not be saved to file %s"
#define ERR_NOREAD		"Lists could not be read from file %s"

/*
 *  combot.c
 */

#define ERR_NOUSERFILENAME	"No userfile has been set"

/*
 *  main.c
 */

#define TEXT_SAYNICK		"Please enter your nickname.\n"

#define TEXT_SIGINT		"Lurking interrupted by luser ... er, owner. (SIGINT)"
#define TEXT_SIGSEGV		"Mary had a little signal segmentation fault (SIGSEGV)"
#define TEXT_SIGBUS		"Another one drives the bus! (SIGBUS)"
#define TEXT_SIGTERM		"What have I done to deserve this?? aaaaaarrghhh! (SIGTERM)"
#define TEXT_SIGUSR1		"QUIT :Switching servers... (SIGUSR1)\n"

#define TEXT_USAGE		"Usage: %s [switches [args]]\n"
#define TEXT_FSWITCH		" -f <file>   read configuration from <file>\n"
#define TEXT_CSWITCH		" -c          make core file instead of coredebug/reset\n"
#define TEXT_HSWITCH		" -h          show this help\n"
#define TEXT_VSWITCH		" -v          show EnergyMech version\n"

#define TEXT_HDR_VERS		"EnergyMech %s, %s\n"
#define TEXT_HDR_DATE		"Compiled on " __DATE__ " " __TIME__ "\n"
#define TEXT_HDR_FEAT		"Features: %s\n"

#define ERR_MISSINGCONF		"init: No configfile specified\n"
#define ERR_UNKNOWNOPT		"init: Unknown option %s\n"
#define ERR_SAMEUSERLIST	"init: Error: UserList for %s matches the userlist for %s\n"
#define ERR_SAMEUSERLIST2	"             Bots can not share the same userlist, please specify a new one.\n"

#define INFO_USINGCONF		"init: Using config file: %s\n"
#define INFO_RUNNING		"init: EnergyMech running...\n"

/*
 *  xmech.c
 */

#define TEXT_NOLINKS		"No Links"

#define TEXT_CURRNICKWANT	"Current nick        %s (Wanted: %s)"
#define TEXT_CURRNICKHAS	"Current nick        %s"
#define TEXT_CURRGUID		"Guid                %i"
#define TEXT_USERLISTSTATS	"Users in userlist   %i (%i Superuser%s, %i Bot%s)"
#define TEXT_ACTIVECHANS	"Active channels     %s"
#define TEXT_MOREACTIVECHANS	"                    %s"

#define TEXT_VIRTHOST		"Virtual host        %s (IP Alias%s)"
#define TEXT_VIRTHOSTWINGATE	"Virtual host        %s:%i (WinGate%s)"
#define TEXT_VHINACTIVE		" - Inactive"

#define TEXT_CURRSERVER		"Current Server      %s:%i"
#define TEXT_CURRSERVERNOT	"Current Server      " TEXT_NOTINSERVLIST
#define TEXT_SERVERONTIME	"Server Ontime       %s"
#define TEXT_BOTMODES		"Mode                +%s"

#define TEXT_ENTITYNAME		"Entity Name         %s"
#define TEXT_ENTITYANDPASS	"Entity Name         %s (Linkpass: %s)"
#define TEXT_LINKPORT		"Linkport            %i %s"
#define TEXT_LNACTIVE		"(Active)"
#define TEXT_LNINACTIVE		"(Inactive)"

#define TEXT_CURRENTTIME	"Current Time        %s"
#define TEXT_BOTSTARTED		"Started             %s"
#define TEXT_BOTUPTIME		"Uptime              %s"
#define TEXT_BOTVERSION		"Version             %s (%s)"
#define TEXT_BOTFEATURES	"Features            %s"
#define TEXT_BOTCC		"Command Char        %c"

#define TEXT_CSERV		"Current Server: %s:%i"
#define TEXT_CSERVNOT		"Current Server: " TEXT_NOTINSERVLIST

#define TEXT_ALREADYSHITTED	"%s is in my shitlist already for this channel"
#define TEXT_SHITLOWACCESS	"Unable to shit %s, insufficient access"
#define TEXT_DEFAULTSHIT	"Leave Lamer!"
#define TEXT_HASSHITTED		"The user has been shitted as %s on %s"
#define TEXT_SHITEXPIRES	"The shitlist will expire: %s"

#define TEXT_SENTWALLOP		"Sent wallop to %s"

#define TEXT_LEVELSNOSAVE	"Levels could not be saved to %s"
#define TEXT_SEENNOSAVE		"SeenList could not be saved to file %s"
#define TEXT_SEENNOLOAD		"SeenList could not be loaded from file %s"

#define TEXT_CLEAREDSHITLIST	"Shitlist has been cleared"

#define TEXT_NOTCONNECTED	"(not connected)"
#define TEXT_WHOMUSERLINE	"* %-16s %-4s   %s (idle %i min, %i sec)"
#define TEXT_WHOMSELFLINE	"%-20s %-4s   %s"
#define TEXT_WHOMBOTLINE	"%-18s %-4s   %s [%i]"

#define TEXT_PARTYECHOON	"Partyline echo is now On"
#define TEXT_PARTYECHOOFF	"Partyline echo is now Off"

#define TEXT_LASTHDR		"\037Last %i Commands\037"

#define TEXT_TOPICCHANGED	"Topic changed on %s"

#define TEXT_DEFAULTSCREWBAN	"Out you go!"

#define TEXT_ADDTOSERVLIST	"Attempting to add server %s to server list"

#define TEXT_EMPTYSERVLIST	"No servers in serverlist!"
#define TEXT_NOSERVMATCHP	"No matching entries was found for %s:%i"
#define TEXT_NOSERVMATCH	"No matching entries was found for %s:*"
#define TEXT_SERVERDELETED	"Server has been deleted: %s:%i"
#define TEXT_MANYSERVMATCH	"Several entries for %s exists, please specify port also"

#define TEXT_AGO		" ago"
#define TEXT_CURRENT		" (current)"
#define TEXT_NEVER		"(never)"

#define TEXT_SP_NOAUTH		"(no authorization)"
#define TEXT_SP_KLINED		"(K-lined)"
#define TEXT_SP_FULLCLASS	"(connection class full)"
#define TEXT_SP_TIMEOUT		"(connection timed out)"
#define TEXT_SP_ERRCONN		"(unable to connect)"
#define TEXT_SP_DIFFPORT	"(use a different port)"

#define TEXT_NOLONGERAWAY	"No longer set /away"
#define TEXT_NOWSETAWAY		"Now set /away"

#define TEXT_DCC_GOODBYE	"Hasta la vista!"

#define TEXT_UNKNOWNUSER	"Unknown user %s"
#define TEXT_NOACCESSON		"Access denied (you have no access on %s)"
#define TEXT_USEROWNSYOU	"Access denied (%s has higher access than you)"
#define TEXT_REMOTEREADONLY	"%s is remote and cannot be modified on this bot"
#define TEXT_USERUNLOCKED	"%s is not locked and cannot be modified"

#define TEXT_USERCHANGED	"User %s has been modified"
#define TEXT_USERNOTCHANGED	"User %s is unmodified"

#define TEXT_CC_IS		"Current commandchar is '%c'"
#define TEXT_CC_SETTO		"Commandchar set to '%c'"
#define TEXT_SINGLECHARFORCC	"Please specify a single character to be used as commandchar"

#define TEXT_SHUTDOWNBY		"Shutdown initiated by %s[100], flatlining ..."

#define TEXT_NOALIASES		"No aliases has been set"

#endif /* TEXT_H */
