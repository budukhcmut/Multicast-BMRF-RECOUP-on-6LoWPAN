/* sink.c — Contiki-NG Sink Node (CC2538DK)
 * - Join multicast ALL (ff02::1) để đồng loạt với các sink khác
 * - Xử lý LED (board), RELAY (PA2), TEMP (on-chip manual ADC)
 * - Reply unicast ACK về root
 */

#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uiplib.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/leds.h"
#include "arch/cpu/cc2538/dev/adc.h"  // Manual ADC cho temp (fix garbage)
#include "dev/gpio.h"
#include "sys/etimer.h"
#include "random.h"
#include "project-conf.h"
#include "net/routing/routing.h"
#include "sys/log.h"
#include <stdio.h>
#include <string.h>

#define LOG_MODULE "SINK"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_PORT APP_UDP_PORT
#define GROUP_ALL_ADDR "ff02::1"

/* Relay PA2 */
#define RELAY_PORT_BASE GPIO_PORT_TO_BASE(GPIO_A_NUM)
#define RELAY_PIN_MASK  GPIO_PIN_MASK(2)

static struct simple_udp_connection udp_conn;

/* Relay control */
static void relay_init(void) {
  GPIO_SOFTWARE_CONTROL(RELAY_PORT_BASE, RELAY_PIN_MASK);
  GPIO_SET_OUTPUT(RELAY_PORT_BASE, RELAY_PIN_MASK);
  GPIO_CLR_PIN(RELAY_PORT_BASE, RELAY_PIN_MASK);
}

static void relay_on(void)  { GPIO_SET_PIN(RELAY_PORT_BASE, RELAY_PIN_MASK); }
static void relay_off(void) { GPIO_CLR_PIN(RELAY_PORT_BASE, RELAY_PIN_MASK); }

/* Send unicast ACK */
static void send_unicast_ack(const uip_ipaddr_t *addr, const char *msg) {
  simple_udp_sendto(&udp_conn, msg, strlen(msg), addr);
}

/* Manual temp read (fix raw garbage, integer calc) */
static void get_temp_str(char *out, size_t out_size) {
  adc_init();  // Init nếu chưa

  /* Read ADC channel 14 (temp), ref AVDD/3 (0x20), div 512x */
  int16_t adc_raw = adc_get(SOC_ADC_ADCCON_CH_TEMP, 0x20, SOC_ADC_ADCCON_DIV_512);
  uint16_t raw = (uint16_t)adc_raw >> 4;  // 12-bit

  if (raw > 4095) raw = 2048;  // Clamp nếu garbage

  /* Formula milli-°C: 25000 + (raw - 1422) * 10000 / 42 */
  int32_t temp_mc = 25000 + ((int32_t)(raw - 1422) * 10000 / 42);
  int t_int = temp_mc / 1000;
  int t_milli = temp_mc % 1000;
  if (t_milli < 0) t_milli = -t_milli;

  snprintf(out, out_size, "TEMP:%d.%03d", t_int, t_milli);
}

/* UDP RX callback */
static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender, uint16_t sender_port,
                            const uip_ipaddr_t *receiver, uint16_t receiver_port,
                            const uint8_t *data, uint16_t datalen)
{
  char buf[128];
  if(datalen >= sizeof(buf)) datalen = sizeof(buf) - 1;
  memcpy(buf, data, datalen);
  buf[datalen] = '\0';

  LOG_INFO("FROM %02x%02x : %s\n", sender->u8[14], sender->u8[15], buf);

  char ack[64];
  if(strcmp(buf, "LED_ON") == 0) {
    leds_on(LEDS_RED);
    strcpy(ack, "ACK:LED_ON");
  } else if(strcmp(buf, "LED_OFF") == 0) {
    leds_off(LEDS_RED);
    strcpy(ack, "ACK:LED_OFF");
  } else if(strcmp(buf, "RELAY_ON") == 0) {
    relay_on();
    strcpy(ack, "ACK:RELAY_ON");
  } else if(strcmp(buf, "RELAY_OFF") == 0) {
    relay_off();
    strcpy(ack, "ACK:RELAY_OFF");
  } else if(strcmp(buf, "REQ_TEMP") == 0) {
    get_temp_str(ack, sizeof(ack));
    clock_delay_usec(random_rand() % 500000);  // Random delay anti-collision
  } else {
    return;  // Ignore unknown
  }

  send_unicast_ack(sender, ack);
}

/* Join multicast ALL */
static void join_group(void) {
  uip_ipaddr_t grp;
  uiplib_ip6addrconv(GROUP_ALL_ADDR, &grp);
  uip_ds6_maddr_add(&grp);
  LOG_INFO("Joined ALL group %s\n", GROUP_ALL_ADDR);
}

/* Main process */
PROCESS(sink_process, "Sink node");
AUTOSTART_PROCESSES(&sink_process);

PROCESS_THREAD(sink_process, ev, data)
{
  static struct etimer periodic;
  PROCESS_BEGIN();

  relay_init();
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

  /* Wait RPL DIO */
  PROCESS_WAIT_UNTIL(NETSTACK_ROUTING.node_is_reachable());
  LOG_INFO("Joined RPL DAG\n");
  join_group();

  etimer_set(&periodic, CLOCK_SECOND * 30);
  while(1) {
    PROCESS_WAIT_EVENT();
    if(etimer_expired(&periodic)) {
      uip_ipaddr_t addr;
      uiplib_ip6addrconv(GROUP_ALL_ADDR, &addr);
      simple_udp_sendto(&udp_conn, "STATUS:alive", strlen("STATUS:alive"), &addr);
      etimer_reset(&periodic);
    }
  }

  PROCESS_END();
}
