#include "contiki.h"
#include "arch/cpu/cc2538/dev/adc.h"  // ADC API đúng (include soc-adc.h gián tiếp)
#include "dev/leds.h"
#include "sys/etimer.h"
#include "sys/log.h"
#include <stdio.h>

#define LOG_MODULE "TEMP-TEST"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(temp_test_process, "CC2538 Manual Temp Test");
AUTOSTART_PROCESSES(&temp_test_process);

PROCESS_THREAD(temp_test_process, ev, data)
{
  static struct etimer timer;
  PROCESS_BEGIN();

  printf("Starting CC2538 Manual Temperature Sensor Test\n");

  // Init ADC
  adc_init();

  while(1) {
    etimer_set(&timer, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    // Read ADC: channel temp (0xE), ref AVDD/3 (0x20), div 512x
    int16_t adc_raw = adc_get(SOC_ADC_ADCCON_CH_TEMP, 0x20, SOC_ADC_ADCCON_DIV_512);
    
    // Extract 12-bit raw (adc_get returns 16-bit with result in [15:4])
    uint16_t raw = (uint16_t)adc_raw >> 4;
    
    // Clamp raw (0-4095)
    if (raw > 4095) {
      printf("WARNING: Invalid RAW=%u (clamp to 2048)\n", raw);
      raw = 2048;  // Midpoint default
    }

    // Calc temp theo driver formula cho AVDD/3 ref: 0.01°C (25000 = 25.00°C base, calibrate cho raw ~1422 = 0V offset)
    int32_t temp_mc = 25000 + ((int32_t)(raw - 1422) * 10000 / 42);

    // In T: XX.XXX °C
    int t_int = temp_mc / 1000;
    int t_milli = temp_mc % 1000;
    if (t_milli < 0) t_milli = -t_milli;  // Handle negative frac

    // V approx: (raw / 4096) * 1.1V (AVDD/3 ref ~1.1V, assume AVDD=3.3V)
    int v_mv = (raw * 1100) / 4096;
    int v_int = v_mv / 1000;
    int v_milli = v_mv % 1000;

    printf("RAW=%u | V=%d.%03d V | T=%d.%03d °C\n", raw, v_int, v_milli, t_int, t_milli);

    // LED >35°C
    if (t_int > 35)
      leds_on(LEDS_RED);
    else
      leds_off(LEDS_RED);
  }

  PROCESS_END();
}
