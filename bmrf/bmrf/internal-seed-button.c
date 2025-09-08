/*
 * Copyright (c) 2010, Loughborough University - Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *         This node is part of the RPL multicast example. It basically
 *         represents a node that does not join the multicast group
 *         but still knows how to forward multicast packets and sends a
 *         multicast message when the button is pressed.
 *         The example will work with or without any number of these nodes
 *
 *         Also, performs some sanity checks for the contiki configuration
 *         and generates an error if the conf is bad
 *
 * \author
 *         Guillermo Gastón Lorente
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "dev/button-sensor.h"

#include <string.h>

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"
#include "net/rpl/rpl.h"

#define MAX_PAYLOAD_LEN 120
#define MCAST_SINK_UDP_PORT 3001 /* Host byte order */

/* Start sending messages START_DELAY secs after we start so that routing can
 * converge */
#define START_DELAY 60

static struct uip_udp_conn * mcast_conn;
static char buf[MAX_PAYLOAD_LEN];
static uint32_t seq_id;

#if !UIP_CONF_IPV6 || !UIP_CONF_ROUTER || !UIP_CONF_IPV6_MULTICAST || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current contiki configuration"
#error "Check the values of: UIP_CONF_IPV6, UIP_CONF_ROUTER, UIP_CONF_IPV6_RPL"
#endif
/*---------------------------------------------------------------------------*/
PROCESS(mcast_internal_seed_process, "Internal-seed Process");
AUTOSTART_PROCESSES(&mcast_internal_seed_process);
/*---------------------------------------------------------------------------*/
static void
prepare_mcast(void)
{
  uip_ipaddr_t ipaddr;

  /*
   * IPHC will use stateless multicast compression for this destination
   * (M=1, DAC=0), with 32 inline bits (1E 89 AB CD)
   */
  uip_ip6addr(&ipaddr, 0xFF1E,0,0,0,0,0,0x89,0xABCD);
  mcast_conn = udp_new(&ipaddr, UIP_HTONS(MCAST_SINK_UDP_PORT), NULL);
}
/*---------------------------------------------------------------------------*/
static void
set_own_address(void)
{
  uip_ipaddr_t addr;

  /* First, set our v6 global */
  uip_ip6addr(&addr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&addr, &uip_lladdr);
  uip_ds6_addr_add(&addr, 0, ADDR_AUTOCONF);

}
/*---------------------------------------------------------------------------*/
static void
multicast_send(void)
{
  uint32_t id;

  id = uip_htonl(seq_id);
  memset(buf, 0, MAX_PAYLOAD_LEN);
  memcpy(buf, &id, sizeof(seq_id));

  PRINTF("Send to: ");
  PRINT6ADDR(&mcast_conn->ripaddr);
  PRINTF(" Remote Port %u,", uip_ntohs(mcast_conn->rport));
  PRINTF(" (msg=0x%08lx)", (unsigned long)uip_ntohl(*((uint32_t *)buf)));
  PRINTF(" %lu bytes\n", (unsigned long)sizeof(id));

  seq_id++;
  uip_udp_packet_send(mcast_conn, buf, sizeof(id));
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mcast_internal_seed_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);

  PRINTF("Multicast Engine: '%s'\n", UIP_MCAST6.name);

  NETSTACK_MAC.off(1);

  set_own_address();

  prepare_mcast();

  etimer_set(&et, START_DELAY * CLOCK_SECOND);
  while(1) {
    PROCESS_YIELD();
    if(etimer_expired(&et) && ev == sensors_event && data == &button_sensor) {
        multicast_send();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
