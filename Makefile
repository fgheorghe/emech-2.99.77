#
#   EnergyMech, IRC Bot software
#   Copyright (c) 1997-2004 proton
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

MISCFILES =	COPYING CREDITS README README.TCL TODO VERSIONS Makefile configure checkmech \
		sample.conf sample.tcl sample.userfile default.bigchars

HELPFILES =	help/ACCESS help/ADD help/ADDSERVER help/ALIAS help/AWAY help/BAN \
		help/BANLIST help/CCHAN help/CHACCESS help/CHANNELS help/CHAT \
		help/CLEARSHIT help/CLVL help/CMD help/CORE help/CSERV help/CTCP \
		help/CYCLE help/DEL help/DELSERVER help/DEOP help/DIE help/DO \
		help/DOWN help/ECHO help/ESAY help/FORGET help/HELP help/HOST \
		help/IDLE help/INSULT help/INVITE help/JOIN help/KB help/KICK \
		help/KS help/KSLIST help/LAST help/LEVELS help/LINK help/LOAD \
		help/LOADLEVELS help/LOADLISTS help/LUSERS help/ME help/MODE \
		help/MSG help/NAMES help/NEXTSERVER help/NICK help/NOTIFY help/ONTIME \
		help/OP help/PART help/PASSWD help/PROTECTION help/QSHIT help/REHASH \
		help/REPORT help/RESET help/RKS help/RSHIT help/RSPY help/RSPYMSG \
		help/RSTATMSG help/RT help/SAVE help/SAVELEVELS help/SAVELISTS \
		help/SAY help/SCREW help/SEEN help/SERVER help/SERVERLIST help/SET \
		help/SETAAWAY help/SETAUB help/SETAVOICE help/SETBANMODES help/SETBT \
		help/SETCKL help/SETENFM help/TOGENFPASS help/SETFL help/SETFPL \
		help/SETIKT help/SETMAL help/SETMBL help/SETMDL help/SETMKL \
		help/SETMPL help/SETNCL help/SETOPMODES help/SETPASS help/SETPROT \
		help/SHIT help/SHITLIST help/SHOWIDLE help/SHUTDOWN help/SITEBAN \
		help/SITEKB help/SPAWN help/SPY help/SPYLIST help/SPYMSG help/STATMSG \
		help/STATS help/TIME help/TOG help/TOGAOP help/TOGAS help/TOGCC \
		help/TOGCK help/TOGDCC help/TOGENFM help/TOGIK help/TOGKS help/TOGPUB \
		help/TOGRK help/TOGSD help/TOGSHIT help/TOGSO help/TOGTOP help/TOPIC \
		help/UNBAN help/UNVOICE help/UP help/UPTIME help/USAGE help/USER \
		help/USERHOST help/USERLIST help/VER help/VERIFY help/VOICE help/WALL \
		help/WHO help/WHOIS help/WHOM

RANDFILES =	messages/8ball.txt messages/away.txt messages/insult.txt \
		messages/kick.txt messages/nick.txt messages/pickup.txt \
		messages/say.txt messages/signoff.txt messages/version.txt

STUBFILES =	src/Makefile.in src/config.h.in src/ld/mech.ldscript

TESTFILES =	config/cc.c config/inet_addr.c config/ldtest.c config/md5.c \
		config/ptr_size.c config/socket.c config/tcl.c config/which

TRIVFILES =	trivia/mkindex.c

SRCFILES =	src/alias.c src/auth.c src/bounce.c src/channel.c src/com-ons.c \
		src/combot.c src/commands.c src/ctcp.c src/debug.c src/dns.c src/dynamode.c \
		src/function.c src/gencmd.c src/greet.c src/help.c src/irc.c src/kicksay.c \
		src/main.c src/mega.c src/net.c src/net_chan.c src/note.c \
		src/notify.c src/parse.c src/perl.c src/prot.c src/redirect.c src/reset.c \
		src/seen.c src/shitlist.c src/socket.c src/spy.c src/stats.c \
		src/tcl.c src/toybox.c src/trivia.c src/uptime.c src/userlist.c \
		src/vars.c src/web.c

HDRFILES =	src/defines.h src/global.h src/h.h src/settings.h src/structs.h src/text.h src/usage.h

CONFFILES =	src/Makefile src/config.h

DISTFILES =	$(MISCFILES) $(RANDFILES) $(SRCFILES) $(HELPFILES) \
		$(STUBFILES) $(HDRFILES) $(TRIVFILES) $(TESTFILES)

#
# simple make rules
#

mech:		$(SRCFILES) $(CONFFILES)
		( cd src ; $(MAKE) mech )

clean:		FORCE
		( cd src ; $(MAKE) clean )

install:	FORCE
		( cd src ; $(MAKE) install )

#
# packing things up for distribution
#

dist:		FORCE
		$(MAKE) dist2 DISTDIR=`sed 's/^.*VERSION.*"\(.*\)".*$$/emech-\1/p; d;' src/global.h`

dist2:		FORCE
		rm -rf /tmp/$(DISTDIR)
		mkdir /tmp/$(DISTDIR)
		tar cf - $(DISTFILES) | ( cd /tmp/$(DISTDIR) ; tar --preserve -xf - )
		cd /tmp ; tar cf - $(DISTDIR) | gzip -9 > $(DISTDIR).tar.gz
		rm -rf /tmp/$(DISTDIR)

FORCE:
