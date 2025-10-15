#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define APP_UDP_PORT 3001

/* Routing & multicast engine (giữ ESMRF cho lý thuyết multicast efficient) */
#define UIP_CONF_ROUTER            1
#define RPL_CONF_MULTICAST         1

/* Multicast forwarding engine */
#define UIP_MCAST6_CONF_ENGINE     UIP_MCAST6_ENGINE_ESMRF

/* Table sizes (nhỏ cho test 2 sinks) */
#define NBR_TABLE_CONF_MAX_NEIGHBORS  10
#define UIP_CONF_MAX_ROUTES           10

/* Network stack (stable test, no duty cycle) */
#define NETSTACK_RDC   nullrdc_driver
#define NETSTACK_MAC   csma_driver
#define RF_CHANNEL     26

/* Multicast group (chỉ ALL cho đồng loạt đơn giản) */
#define GROUP_ALL_ADDR   "ff02::1"  // Link-local ALL: sinks nhận đồng loạt LED/RELAY/TEMP

/* Sensor & printf cho temp on-chip (fix raw garbage) */
#define CC2538_SENSOR 1
#define PRINTF_FLOATH_ENABLED 1  // Cho snprintf nếu cần (code sink dùng integer %d)

/* IPv6 tweaks (simple) */
#define UIP_CONF_DS6_ADDRCONF 0  // Disable auto SLAAC
#define UIP_CONF_ND6_SEND_RA 0   // Root không advertise RA

/* Logging (balanced cho debug RPL/multicast) */
#define LOG_CONF_LEVEL_RPL     LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_IPV6    LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_6LOWPAN LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC     LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAIN    LOG_LEVEL_INFO

#endif /* PROJECT_CONF_H_ */
