/* root-ng.c — Contiki-NG Border Router (Gateway đơn giản)
 * - RPL root, bridge UART <-> UDP multicast ALL (ff02::1)
 * - Commands: LED_ON/OFF, RELAY_ON/OFF, REQ_TEMP → cả sinks đồng loạt
 * - Reply từ sink forward serial cho Pi
 */

#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uiplib.h"
#include "net/routing/routing.h"
#include "dev/serial-line.h"
#include "project-conf.h"
#include <stdio.h>
#include <string.h>

#define UDP_PORT APP_UDP_PORT
#define GROUP_ALL_ADDR "ff02::1"  // Multicast ALL cho đồng loạt
static struct simple_udp_connection udp_conn;

static void send_multicast(const char *payload)
{
  uip_ipaddr_t addr;
  uiplib_ip6addrconv(GROUP_ALL_ADDR, &addr);
  simple_udp_sendto(&udp_conn, payload, strlen(payload), &addr);
  printf("ROOT: multicast ALL -> %s\n", payload);
}

static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender, uint16_t sender_port,
                            const uip_ipaddr_t *receiver, uint16_t receiver_port,
                            const uint8_t *data, uint16_t datalen)
{
  char buf[128];
  if(datalen >= sizeof(buf)) datalen = sizeof(buf) - 1;
  memcpy(buf, data, datalen);
  buf[datalen] = '\0';

  /* Forward reply từ sink đến serial (cho Pi parser) */
  printf("ROOT: FROM %02x%02x : %s\n", sender->u8[14], sender->u8[15], buf);
  printf("%s\n", buf);  // Line-only cho Pi
}

PROCESS(root_process, "Contiki-NG Root / Gateway");
AUTOSTART_PROCESSES(&root_process);

PROCESS_THREAD(root_process, ev, data)
{
  PROCESS_BEGIN();

  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

  /* Start RPL root */
  NETSTACK_ROUTING.root_start();
  printf("[INFO] RPL DAG created (root mode)\n");

  serial_line_init();
  printf("ROOT started. Commands: LED_ON/OFF, RELAY_ON/OFF, REQ_TEMP\n");

  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == serial_line_event_message) {
      char *cmd = (char *)data;
      if(!cmd || !*cmd) continue;

      /* Trim newline */
      size_t L = strlen(cmd);
      while(L && (cmd[L-1]=='\n' || cmd[L-1]=='\r')) cmd[--L] = 0;

      char payload[128];
      strncpy(payload, cmd, sizeof(payload)-1);
      payload[sizeof(payload)-1] = 0;

      send_multicast(payload);  // Gửi multicast ALL
    }
  }

  PROCESS_END();
}
