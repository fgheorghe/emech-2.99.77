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
#define DNS_C
#include "config.h"

#ifdef RAWDNS

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#define unpack_ushort(x)	(((x)[0] << 8) | ((x)[1]))
#define unpack_ulong(x)		(((x)[0] << 24) | ((x)[1] << 16) | ((x)[2] << 8) | ((x)[3]))

typedef struct dnsType
{
	ushort		type;
	ushort		class;

} dnsType;

typedef struct dnsRType
{
	ushort		type;		/* &0 */
	ushort		class;		/* &2 */
	ulong		ttl;		/* &4 */
	ushort		rdlength;	/* &8 */

} dnsRType;

#define DNS_QUERY		1
#define DNS_TYPE_A		1
#define DNS_CLASS_IN		1
#define DNS_TYPE_NS		2
#define DNS_TYPE_CNAME		5

#define	QUERY_FLAGS		128

LS int dnssock = -1;
LS int dnsserver = 0;

void init_rawdns(void)
{
	struct	sockaddr_in sai;

	if ((dnssock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
		return;

	memset(&sai,0,sizeof(sai));
	sai.sin_addr.s_addr = INADDR_ANY;
	sai.sin_family = AF_INET;

	if (bind(dnssock,(struct sockaddr*)&sai,sizeof(sai)) < 0)
	{
		close(dnssock);
		dnssock = -1;
		return;
	}
	SockFlags(dnssock);
#ifdef DEBUG
	debug("(init_rawdns) {%i} dnssock is active\n",dnssock);
#endif /* DEBUG */
}

struct in_addr dnsroot_lookup(const char *hostname)
{
	dnsAuthority *da;
	struct in_addr ip;

	for(da=dnsroot;da;da=da->next)
	{
		if (!Strcasecmp(hostname,da->hostname))
		{
#ifdef DEBUG
			debug("(dnsroot_lookup) %s = %s\n",hostname,inet_ntoa(da->ip));
#endif /* DEBUG */
			return(da->ip);
		}
	}
	ip.s_addr = -1;
	return(ip);
}

const char *get_dns_token(const char *src, const char *packet, char *dst, int sz)
{
	const char *endsrc = NULL;
	ushort	offptr;
	int	tsz;
	int	dot = 0;

	for(;;dot=1)
	{
		tsz = (uchar)*(src++);
		if ((tsz & 0xC0) == 0xC0)
		{
			offptr = (tsz & 0x3f) << 8;
			offptr |= *src;
			if ((packet + offptr) > (packet + sz))
				return(src+1);
			if (!endsrc)
				endsrc = src + 1;
			src = packet + offptr;
			tsz = *(src++);
		}
		if (tsz == 0)
			break;
		if (dot)
			*(dst++) = '.';
		while(tsz)
		{
			tsz--;
			*(dst++) = *(src++);
		}
	}
	*dst = 0;
	return((endsrc) ? endsrc : src);
}

#ifdef DEBUG

char *type2ascii(int type)
{
	switch(type)
	{
	case 1:
		return("A (host address)");
	case 2:
		return("NS (authoritative name server)");
	case 5:
		return("CNAME (canonical name)");
	case 6:
		return("SOA (zone of authority)");
	case 12:
		return("PTR (domain name pointer)");
	case 13:
		return("HINFO (host information)");
	case 15:
		return("MX (mail exchange)");
	case 16:
		return("TXT (text strings)");
	}
	return("unknown");
}

char *class2ascii(int class)
{
	switch(class)
	{
	case 1:
		return("IN (the internet)");
	case 2:
		return("CS (the CSNET class)");
	case 3:
		return("CH (the CHAOS class)");
	case 4:
		return("HS (Hesiod)");
	}
	return("unknown");
}

const char *dump_token(const char *src, const char *packet, int sz)
{
	const char *endsrc;
	char	*dst,token[64];
	ushort	offptr;
	int	tsz;

	endsrc = NULL;
	for(;;)
	{
		tsz = (uchar)*(src++);
		if ((tsz & 0xC0) == 0xC0)
		{
			offptr = ((tsz & 0x3f) << 8) | *src;
			debug(" *");
			if ((packet + offptr) > (packet + sz))
			{
				debug(" err: offset pointer outside packet (offset %u (%04x), size %i)\n",offptr,offptr,sz);
				return(src+1);
			}
			if (!endsrc)
				endsrc = src + 1;
			src = packet + offptr;
			tsz = *(src++);
		}
		debug(" %i ",tsz);
		if (tsz == 0)
			break;
		dst = token;
		while(tsz)
		{
			tsz--;
			*(dst++) = *(src++);
		}
		*dst = 0;
		debug("\"%s\"",token);
	}
	debug("\n");
	return((endsrc) ? endsrc : src);
}

void dump_packet(int sz, char *packet)
{
	struct	in_addr *iap;
	dnsQuery *query;
	dnsRType *rtyp;
	dnsType	*typ;
	ulong	rr_type,rr_class,rr_ttl,rr_len;
	char	token[64],uncode[100];
	const char *src;
	int	tsz,n;

	query = (dnsQuery*)packet;
	debug("() sz = %i\n",sz);

	tsz = ntohs(query->flags);
	*token = 0;

#define MIDLINE		if (*token) strcat(token," | ")

	if (tsz & 1)
	{
		MIDLINE;
		strcat(token,"QR");
	}
	tsz >>= 1;

	MIDLINE;
	switch(tsz & 0xf)
	{
	case 0:
		strcat(token,"OPCODE=QUERY");
		break;
	case 1:
		strcat(token,"OPCODE=IQUERY");
		break;
	case 2:
		strcat(token,"OPCODE=STATUS");
		break;
	case 4:
		strcat(token,"OPCODE=NOTIFY");
		break;
	case 5:
		strcat(token,"OPCODE=UPDATE");
		break;
	default:
		sprintf(uncode,"OPCODE=%i",(tsz & 0xf));
		strcat(token,uncode);
	}
	tsz >>= 4;

	if (tsz & 1)
	{
		MIDLINE;
		strcat(token,"AA");
	}
	tsz >>= 1;

	if (tsz & 1)
	{
		MIDLINE;
		strcat(token,"TC");
	}
	tsz >>= 1;

	if (tsz & 1)
	{
		MIDLINE;
		strcat(token,"RD");
	}
	tsz >>= 1;

	if (tsz & 1)
	{
		MIDLINE;
		strcat(token,"RA");
	}
	tsz >>= 1;

	if (tsz & 7)
	{
		MIDLINE;
		sprintf(uncode,"Z=%i",(tsz & 7));
		strcat(token,uncode);
	}
	tsz >>= 3;

	MIDLINE;
	switch(tsz & 0xf)
	{
	case 0:
		strcat(token,"RCODE=OK");
		break;
	case 1:
		strcat(token,"RCODE=Format Error");
		break;
	case 2:
		strcat(token,"RCODE=Server Failure");
		break;
	case 3:
		strcat(token,"RCODE=Name Error");
		break;
	case 4:
		strcat(token,"RCODE=Not Implemented");
		break;
	case 5:
		strcat(token,"RCODE=Refused");
		break;
	case 6:
		strcat(token,"RCODE=YXDOMAIN");
		break;
	case 7:
		strcat(token,"RCODE=YXRRSET");
		break;
	case 8:
		strcat(token,"RCODE=NXRRSET");
		break;
	case 9:
		strcat(token,"RCODE=NOTAUTH");
		break;
	case 10:
		strcat(token,"RCODE=NOTZONE");
		break;
	default:
		sprintf(uncode,"RCODE=%i",(tsz & 0xf));
		strcat(token,uncode);
	}

	debug(	"() qid         = %i\n"
		"() flags       = %s (%i)\n"
		"() questions   = %i\n"
		"() answers     = %i\n"
		"() authorities = %i\n"
		"() resources   = %i\n\n",
		ntohs(query->qid),
		token,ntohs(query->flags),
		ntohs(query->questions),
		ntohs(query->answers),
		ntohs(query->authorities),
		ntohs(query->resources)
		);

	src = (char*)query;
	src += 12;

	n = ntohs(query->questions);
	while(n)
	{
		debug("() Question\n() offset = %i\n() name  =",(src-packet));
		src = dump_token(src,packet,sz);
		typ = (dnsType*)src;
		rr_type  = unpack_ushort((char*)&typ->type);
		rr_class = unpack_ushort((char*)&typ->class);
		debug("() type  = %lu %s\n",rr_type,type2ascii(rr_type));
		debug("() class = %lu %s\n",rr_class,class2ascii(rr_class));
		debug("\n");
		src += 4;
		n--;
	}

	n = ntohs(query->answers);
	while(n)
	{
		debug("() Answer\n() offset = %i\n() name  =",(src-packet));
		src = dump_token(src,packet,sz);
		rtyp     = (dnsRType*)src;
		rr_type  = unpack_ushort((char*)&rtyp->type);
		rr_class = unpack_ushort((char*)&rtyp->class);
		rr_ttl   = unpack_ulong((char*)&rtyp->ttl);
		rr_len   = unpack_ushort((char*)&rtyp->rdlength);
		src += 10;
		debug("() type  = %lu %s\n",rr_type,type2ascii(rr_type));
		debug("() class = %lu %s\n",rr_class,class2ascii(rr_class));
		debug("() ttl   = %lu\n",rr_ttl);
		debug("() rdlen = %lu\n",rr_len);

		if ((rr_type == DNS_TYPE_A) && (rr_class == DNS_CLASS_IN))
		{
			iap = (struct in_addr*)src;
			debug("() rdata = %s\n",inet_ntoa(*iap));
		}
		if ((rr_type == DNS_TYPE_NS) && (rr_class == DNS_CLASS_IN))
		{
			debug("() rdata =");
			dump_token(src,packet,sz);
		}
		if ((rr_type == DNS_TYPE_CNAME) && (rr_class == DNS_CLASS_IN))
		{
			debug("() rdata =");
			dump_token(src,packet,sz);
		}
		src += rr_len;

		n--;
		debug("\n");
	}

	n = ntohs(query->authorities);
	while(n)
	{
		debug("() Authority\n() offset = %i\n() name  =",(src-packet));
		src = dump_token(src,packet,sz);
		rtyp     = (dnsRType*)src;
		rr_type  = unpack_ushort((char*)&rtyp->type);
		rr_class = unpack_ushort((char*)&rtyp->class);
		rr_ttl   = unpack_ulong((char*)&rtyp->ttl);
		rr_len   = unpack_ushort((char*)&rtyp->rdlength);
		src += 10;

		debug("() type  = %lu %s\n",rr_type,type2ascii(rr_type));
		debug("() class = %lu %s\n",rr_class,class2ascii(rr_class));
		debug("() ttl   = %lu\n",rr_ttl);
		debug("() rdlen = %lu\n() rdata =",rr_len);

		dump_token(src,packet,sz);
		src += rr_len;

		n--;
		debug("\n");
	}

	n = ntohs(query->resources);
	while(n)
	{
		debug("() Resources\n() offset = %i\n() name  =",(src-packet));
		src = dump_token(src,packet,sz);
		rtyp     = (dnsRType*)src;
		rr_type  = unpack_ushort((char*)&rtyp->type);
		rr_class = unpack_ushort((char*)&rtyp->class);
		rr_ttl   = unpack_ulong((char*)&rtyp->ttl);
		rr_len   = unpack_ushort((char*)&rtyp->rdlength);
		src += 10;

		debug("() type  = %lu %s\n",rr_type,type2ascii(rr_type));
		debug("() class = %lu %s\n",rr_class,class2ascii(rr_class));
		debug("() ttl   = %lu\n",rr_ttl);
		debug("() rdlen = %lu\n",rr_len);

		if ((rr_type == DNS_TYPE_A) && (rr_class == DNS_CLASS_IN))
		{
			iap = (struct in_addr*)src;
			debug("() rdata = %s\n",inet_ntoa(*iap));
		}

		src += rr_len;
		n--;
		debug("\n");
	}
}

#endif /* DEBUG */

int make_query(char *packet, const char *hostname)
{
	char	*size,*dst;

	/*
	 *  make a packet
	 */
	packet[0] = rand() >> 24;
	packet[1] = rand() >> 24;
	packet[2] = 0;
	packet[3] = QUERY_FLAGS;
	((ulong*)packet)[1] = htonl(0x10000);
	((ulong*)packet)[2] = 0;
	size = packet + 12;
	dst  = size + 1;
	while(*hostname)
	{
		if ((*dst = *hostname) == '.')
		{
			*size = (dst - size - 1);
			size = dst;
		}
		hostname++;
		dst++;
	}
	*size = (dst - size - 1);

	dst[0] = 0;
	dst[1] = 0;
	dst[2] = 1;
	dst[3] = 0;
	dst[4] = 1;
	return(dst - packet + 5);
}

/*
 *
 */
void parse_query(int psz, dnsQuery *query)
{
	struct	sockaddr_in sai;
	char	packet[512];
	struct in_addr *ip;
	dnsList	*dns;
	const char *src,*rtyp;
	char	token[64],token2[64];
	int	sz,n;

	src = (const char*)query;
	src += 12;

	for(dns=dnslist;dns;dns=dns->next)
	{
		if (dns->id == ntohs(query->qid))
			break;
	}
	if (!dns)
		return;

	n = ntohs(query->questions);
	while(n--)
	{
		/* skip QNAME */
		src = get_dns_token(src,(const char *)query,token,psz);
		/* skip (ushort)QTYPE and (ushort)QCLASS */
		src += 4;
	}

	n = ntohs(query->answers);
	while(n)
	{
		src = get_dns_token(src,(const char*)query,token,psz);
		rtyp = src;
		src += 10;

#ifdef DEBUG
		debug("(parse_query) %i: answer = %s\n",dns->id,token);
#endif /* DEBUG */

		if ((unpack_ushort(&rtyp[0]) == DNS_TYPE_CNAME) &&
		    (unpack_ushort(&rtyp[2]) == DNS_CLASS_IN))
		{
			get_dns_token(src,(const char *)query,token2,psz);
#ifdef DEBUG
			debug("(parse_query) %i: cname: %s = %s\n",dns->id,token,token2);
#endif /* DEBUG */
			if (dns->cname)
				Free((char**)&dns->cname);
			set_mallocdoer(parse_query);
			dns->cname = (char*)Malloc(strlen(token2));
			dns->when = now + 30;
			Strcpy(dns->cname,token2);
		}

		if ((unpack_ushort(&rtyp[0]) == DNS_TYPE_A) &&
		    (unpack_ushort(&rtyp[2]) == DNS_CLASS_IN) &&
		    (unpack_ushort(&rtyp[8]) == 4))
		{
			ip = (struct in_addr *)src;
			if (dns->auth && !Strcasecmp(dns->auth->hostname,token))
			{
				dns->auth->ip.s_addr = ip->s_addr;
				dns->when = now + 60;
#ifdef DEBUG
				debug("(parse_query) a auth: %s = %s\n",token,inet_ntoa(*ip));
#endif /* DEBUG */
			}
			else
			if (!Strcasecmp(dns->host,token) || (dns->cname && !Strcasecmp(dns->cname,token)))
			{
				dns->ip.s_addr = ip->s_addr;
				dns->when = now + 3600;
#ifdef DEBUG
				debug("(parse_query) a: %s = %s\n",token,inet_ntoa(*ip));
#endif /* DEBUG */
				return;
			}
		}
		src += unpack_ushort(&rtyp[8]);
		n--;
	}

	n = ntohs(query->authorities);
	if (n)
		sz = RANDOM(1,n);
#ifdef DEBUG
	if (n)
		debug("auth: select %i count %i\n",sz,n);
#endif /* DEBUG */
	while(n)
	{
		src = get_dns_token(src,(const char*)query,token,psz);
		rtyp = src;
		src += 10;
		if ((unpack_ushort(&rtyp[0]) == DNS_TYPE_NS) &&
		    (unpack_ushort(&rtyp[2]) == DNS_CLASS_IN))
		{
			dnsAuthority *da;

			get_dns_token(src,(const char *)query,token2,psz);
			if (sz == n)
			{
				if (dns->auth == NULL)
				{
					set_mallocdoer(parse_query);
					da = dns->auth = (dnsAuthority*)Malloc(sizeof(dnsAuthority) + strlen(token2));
					da->ip.s_addr = 0;
					Strcpy(da->hostname,token2);
				}
				else
				if (dns->findauth == 1)
				{
					dns->findauth = 2;
					if (dns->auth2)
						Free((char**)&dns->auth2);
					set_mallocdoer(parse_query);
					da = dns->auth2 = (dnsAuthority*)Malloc(sizeof(dnsAuthority) + strlen(token2));
					da->ip.s_addr = 0;
					Strcpy(da->hostname,token2);
#ifdef DEBUG
					debug("(parse_query) 2nd auth set: %s\n",token2);
#endif /* DEBUG */
				}
			}
#ifdef DEBUG
			debug("(parse_query) authorities: %s = %s%s\n",token,token2,(sz==n) ? "*" : "");
#endif /* DEBUG */
		}
		src += unpack_ushort(&rtyp[8]);
		n--;
	}

	if (dns->findauth >= 1)
		dns->findauth = 1;

	n = ntohs(query->resources);
	while(n)
	{
		src = get_dns_token(src,(const char*)query,token,psz);
		rtyp = src;
		src += 10;

		if (	(unpack_ushort(&rtyp[0]) == DNS_TYPE_A) &&
			(unpack_ushort(&rtyp[2]) == DNS_CLASS_IN) &&
			(unpack_ushort(&rtyp[8]) == 4))
		{
			ip = (struct in_addr *)src;
			if (dns->auth && !Strcasecmp(dns->auth->hostname,token))
				dns->auth->ip.s_addr = ip->s_addr;
			if (dns->auth2 && !Strcasecmp(dns->auth2->hostname,token))
				dns->auth2->ip.s_addr = ip->s_addr;
#ifdef DEBUG
			debug("(parse_query) resources: %s = %s\n",token,inet_ntoa(*ip));
#endif /* DEBUG */
		}
		src += unpack_ushort(&rtyp[8]);
		n--;
	}

	if (dns->auth && dns->auth->ip.s_addr == 0)
	{
		sai.sin_addr = dnsroot_lookup(dns->auth->hostname);
		if (sai.sin_addr.s_addr != -1)
			dns->auth->ip.s_addr = sai.sin_addr.s_addr;
	}

	if (dns->auth2 && dns->auth2->ip.s_addr == 0)
	{
		sai.sin_addr = dnsroot_lookup(dns->auth2->hostname);
		if (sai.sin_addr.s_addr != -1)
			dns->auth2->ip.s_addr = sai.sin_addr.s_addr;
	}

	if (dns->auth && dns->auth->ip.s_addr && dns->auth2)
	{
		Free((char**)&dns->auth2);
		dns->findauth = 0;
	}

#ifdef DEBUG
	debug("> dns->when	%lu\n",dns->when);
	debug("> dns->ip	%s\n",inet_ntoa(dns->ip));
	debug("> dns->auth	%s : %s\n",(dns->auth) ? dns->auth->hostname : "NULL",(dns->auth) ? inet_ntoa(dns->auth->ip) : "-");
	debug("> dns->auth2	%s : %s\n",(dns->auth2) ? dns->auth2->hostname : "NULL",(dns->auth2) ? inet_ntoa(dns->auth2->ip) : "-");
	debug("> dns->findauth	%i\n",dns->findauth);
	debug("> dns->id	%i\n",dns->id);
	debug("> dns->cname	%s\n",nullstr(dns->cname));
	debug("> dns->host	%s\n",dns->host);
#endif /* DEBUG */

	src = NULL;
	if (dns->auth2)
	{
		if (dns->auth2->ip.s_addr && dns->auth)
		{
			src = dns->auth->hostname;
			sai.sin_addr.s_addr = dns->auth2->ip.s_addr;
		}
		else
		{
			src = dns->auth2->hostname;
			sai.sin_addr.s_addr = (ia_ns[dnsserver].s_addr == 0) ? ia_default.s_addr : ia_ns[dnsserver].s_addr;
		}
	}
	else
	if (dns->auth)
	{
		if (dns->auth->ip.s_addr)
		{
			/*
			 *  we know the IP of the authorative NS to ask
			 */
			src = (dns->cname) ? dns->cname : dns->host;
			sai.sin_addr.s_addr = dns->auth->ip.s_addr;
		}
		else
		{
			/*
			 *  have to dig up the IP of the NS to ask
			 */
			dns->findauth = 1;
			src = dns->auth->hostname;
			sai.sin_addr.s_addr = (ia_ns[dnsserver].s_addr == 0) ? ia_default.s_addr : ia_ns[dnsserver].s_addr;
		}
	}
	if (src)
	{
		dns->id = rand();
#ifdef DEBUG
		debug("(parse_query) %i: asking %s who is `%s'\n",dns->id,inet_ntoa(sai.sin_addr),src);
#endif /* DEBUG */
		sz = make_query(packet,src);
		dns->when = now + 60;
		sai.sin_family = AF_INET;
		sai.sin_port = htons(53);
		((dnsQuery*)packet)->qid = htons(dns->id);
		sendto(dnssock,packet,sz,0,(struct sockaddr*)&sai,sizeof(sai));
	}
	if (dns->auth && dns->auth->ip.s_addr)
		Free((char**)&dns->auth);
	if (dns->auth2 && dns->auth2->ip.s_addr)
		Free((char**)&dns->auth2);
}

void rawdns(const char *hostname)
{
	struct	sockaddr_in sai;
	dnsQuery *query;
	dnsList *item;
	char	packet[512];
	int	sz;

	if (dnssock == -1)
		init_rawdns();

	if (dnssock == -1)
		return;

	sz = make_query(packet,hostname);
	query = (dnsQuery*)packet;

	set_mallocdoer(rawdns);
	item = (dnsList*)Calloc(sizeof(dnsList) + strlen(hostname));
	Strcpy(item->host,hostname);
	item->id = ntohs(query->qid);
	item->when = now + 30;
	item->next = dnslist;
	dnslist = item;

	/*
	 *  send the packet
	 */
	sai.sin_family = AF_INET;
	sai.sin_port = htons(53);
	sai.sin_addr.s_addr = (ia_ns[dnsserver].s_addr == 0) ? ia_default.s_addr : ia_ns[dnsserver].s_addr;

#ifdef DEBUG
	debug("(rawdns) questions %s: %s\n",inet_ntoa(sai.sin_addr),item->host);
#endif /* DEBUG */

	dnsserver++;
	if (ia_ns[dnsserver].s_addr == 0)
		dnsserver = 0;

	sendto(dnssock,packet,sz,0,(struct sockaddr*)&sai,sizeof(sai));
#ifdef DEBUG
	//dump_packet(sz,packet);
#endif /* DEBUG */
}

void select_rawdns(void)
{
	dnsList *dns,**pdns;

	if (dnssock != -1)
	{
		chkhigh(dnssock);
		FD_SET(dnssock,&read_fds);
	}
restart:
	pdns = &dnslist;
	while(*pdns)
	{
		if ((*pdns)->when < now)
		{
			dns = *pdns;
			if (dns->cname)
				Free((char**)&dns->cname);
			if (dns->auth)
				Free((char**)&dns->auth);
			if (dns->auth2)
				Free((char**)&dns->auth2);
#ifdef DEBUG
			debug("(select_rawdns) removing %s qid %i\n",dns->host,dns->id);
#endif /* DEBUG */
			*pdns = dns->next;
			Free((char**)&dns);
			goto restart;
		}
		pdns = &(*pdns)->next;
	}
}

void process_rawdns(void)
{
	struct	sockaddr_in sai;
	char	packet[512];
	int	sz,n;

	if (dnssock == -1)
		return;
	if (FD_ISSET(dnssock,&read_fds))
	{
		sz = sizeof(sai);
		n = recvfrom(dnssock,packet,512,0,(struct sockaddr*)&sai,&sz);
		if (n < sizeof(dnsQuery))
			return;
#ifdef DEBUG
		debug("(process_rawdns) packet from: %s\n",inet_ntoa(sai.sin_addr));
		//dump_packet(n,packet);
#endif /* DEBUG */
		parse_query(n,(dnsQuery*)packet);
	}
}

char *poll_rawdns(char *hostname)
{
	dnsList *dns;

	for(dns=dnslist;dns;dns=dns->next)
	{
		if (dns->ip.s_addr && !Strcasecmp(dns->host,hostname))
		{
#ifdef DEBUG
			debug("(poll_rawdns) a: %s ==> %s\n",hostname,inet_ntoa(dns->ip));
#endif /* DEBUG */
			return(inet_ntoa(dns->ip));
		}
	}
	return(NULL);
}

int read_dnsroot(char *line)
{
	struct	in_addr ia;
	dnsAuthority *da;
	char	*name,*a,*ip,*src;

	name = chop(&line);
	a    = chop(&line);	/* TTL is optional */
	if (Strcmp(a,"A"))
		a = chop(&line);
	ip   = chop(&line);

	if (a && !Strcmp(a,"A") && ip && inet_aton(ip,&ia) != 0)
	{
		/* remove trailing dot */
		for(src=name;*src;)
		{
			if (*src == '.' && *(src+1) == 0)
				*src = 0;
			else
				src++;
		}
		set_mallocdoer(read_dnsroot);
		da = (dnsAuthority*)Malloc(sizeof(dnsAuthority) + strlen(name));
		Strcpy(da->hostname,name);
		da->ip.s_addr = ia.s_addr;
		da->next = dnsroot;
		dnsroot = da;
#ifdef DEBUG
		debug("(read_dnsroot) stored root IP: %s = %s\n",name,ip);
#endif /* DEBUG */
	}
	return(FALSE);
}

void do_dnsroot(COMMAND_ARGS)
{
	int	in;

	if ((in = open(rest,O_RDONLY)) >= 0)
		readline(in,&read_dnsroot);			/* readline closes in */
}

void do_dnsserver(COMMAND_ARGS)
{
	struct in_addr ia;
	char	*p,c,tempservers[MAX_NAMESERVERS*16+3];
		/* (xxx.yyy.zzz.www + 1 space * MAX_NAMESERVERS) + 1 terminator char + 2 chars bold font */
	int	i;

	if (!rest || !*rest)
	{
#ifdef DEBUG
		debug("(do_dnsserver) no argument, listing servers\n");
#endif /* DEBUG */
		*(p = tempservers) = 0;
		for(i=0;i<MAX_NAMESERVERS;i++)
		{
			if (ia_ns[i].s_addr > 0)
			{
				if (i == dnsserver)
				{
					sprintf(p,"\037%s\037 ",inet_ntoa(ia_ns[i]));
					p = STREND(p);
				}
				else
				{
					p = Strcpy(p,inet_ntoa(ia_ns[i]));
					*(p++) = ' ';
					*p = 0;
				}
			}
		}
		if (*tempservers == 0)
			Strcpy(tempservers,"\037127.0.0.1\037");
		to_user(from,"Current DNS Servers: %s",tempservers);
		return;
	}

	c = *(rest++);
	if ((ia.s_addr = inet_addr(rest)) == -1)
		c = 0;
#ifdef DEBUG
	debug("(do_dnsserver) argument = `%c'\n",c);
#endif /* DEBUG */
	switch(c)
	{
	case '+':
		for(i=0;i<MAX_NAMESERVERS;i++)
		{
			if (ia_ns[i].s_addr == 0)
			{
				ia_ns[i].s_addr = ia.s_addr;
				to_user(from,"DNS Server added: %s",rest);
				return;
			}
		}
		to_user(from,"No free DNS Server slots found, remove one before adding new servers");
		return;
	case '-':
		for(i=0;i<MAX_NAMESERVERS;i++)
		{
			if (ia_ns[i].s_addr == ia.s_addr || ia.s_addr == 0)
			{
				ia_ns[i].s_addr = 0;
			}
		}
		for(i=1;i<MAX_NAMESERVERS;i++)
		{
			if (ia_ns[i-1].s_addr == 0)
			{
				ia_ns[i-1].s_addr = ia_ns[i].s_addr;
				ia_ns[i].s_addr = 0;
			}
		}
		dnsserver = 0;
		if (ia.s_addr > 0)
			to_user(from,"DNS Server removed: %s",rest);
		else
			to_user(from,"All known DNS Servers removed.");
		return;
	default:
		usage(from);
	}
}

#endif /* RAWDNS */
