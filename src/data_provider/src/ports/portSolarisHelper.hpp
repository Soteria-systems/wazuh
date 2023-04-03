/*
 * Wazuh SysInfo
 * Copyright (C) 2015, Wazuh Inc.
 * March 17, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <sys/types.h>
#include <sys/tihdr.h>
#include <inet/mib2.h>
#include <inet/ip.h>

#include <vector>

#ifndef _PORT_SOLARIS_HELPER_H
#define _PORT_SOLARIS_HELPER_H

typedef struct mib_item_s {
	int			group;
	int			mib_id;
	int			length;
	std::shared_ptr<char> val;
} mib_item_t;

class PortSolarisHelper final
{
    public:
        static int tcpConnEntrySize;
        static int tcp6ConnEntrySize;
        static int udpEntrySize;
        static int udp6EntrySize;

        static void mibGetItems(int sd, std::vector<mib_item_t> &items);
        static int mibOpen(void);
        static void mibGetConstants(std::vector<mib_item_t>& items);
};

#endif //_PORT_SOLARIS_HELPER_H