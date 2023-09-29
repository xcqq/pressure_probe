#include "CS1237.h"
#include <Arduino.h>

// IO definision
#define KEY_USER PA8
#define LED_DEBUG PA4
#define LED_PROBE PA5
#define ADC_SCK PA6
#define ADC_DOUT PA7

// pressure sensor configure
#define SENSOR_MAX_WEIGHT 5000
#define SENSOR_MAX_VOLT 1 // mv
#define SENSOR_REVERSE 1

// ADC configure
#define ADC_REF_VOLT 2500 // mv
#define ADC_MAX_VALUE (1 << 24)
#define ADC_PGA 128 // just for reference

// edge detection configure
#define WINDOW_SIZE 500
#define THRESHOLD_STATIC 250 // g
#define EDGE_CONTINUOUS 1 // could make it more stable

typedef struct {
    int32_t data[WINDOW_SIZE];
    int32_t sum;
    int32_t mean;
    int32_t index;
} window_data_t;

window_data_t window_data;

void window_data_init(window_data_t *wd)
{
    wd->sum = 0;
    wd->mean = 0;
    wd->index = 0;
    memset(wd->data, 0, sizeof(wd->data));
}

void window_data_add(window_data_t *wd, int32_t data)
{
    wd->sum -= wd->data[wd->index];
    wd->data[wd->index] = data;
    wd->sum += data;
    wd->mean = wd->sum / WINDOW_SIZE;
    wd->index = (wd->index + 1) % WINDOW_SIZE;
}

void setup()
{
    int32_t values;

    //need high baudrate so it won't delay ADC read
    Serial.begin(2000000);
    Serial.println("Pressure bed starting...");

    pinMode(LED_DEBUG, OUTPUT);
    pinMode(LED_PROBE, OUTPUT);
    digitalWrite(LED_DEBUG, LOW);
    digitalWrite(LED_PROBE, LOW);

    window_data_init(&window_data);

    CS1237_init(ADC_SCK, ADC_DOUT);
    if (!CS1237_configure(PGA_128, SPEED_1280, CHANNEL_A))
        Serial.println("ADC configured successfully");
    else
        Serial.println("ADC configuration failed");
}

int32_t adc_to_weight(int64_t value)
{
    int64_t weight;
    int64_t con = SENSOR_MAX_VOLT * ADC_MAX_VALUE * ADC_PGA / (ADC_REF_VOLT * SENSOR_MAX_WEIGHT);
    weight = value / con;

#if SENSOR_REVERSE == 1
    weight = -weight;
#endif
    return weight;
}

    int32_t edge_cnt = 0;
void loop()
{
    int32_t ret = 0;
    int32_t weight = 0;

    ret = CS1237_read();
    weight = adc_to_weight(ret);
    window_data_add(&window_data, weight);
    if(weight - window_data.mean >= THRESHOLD_STATIC) {
        edge_cnt++;
    } else {
        edge_cnt = 0;
    }
    if(edge_cnt >= EDGE_CONTINUOUS) {
        digitalWrite(LED_PROBE, HIGH);
    } else {
        digitalWrite(LED_PROBE, LOW);
    }
    //debug log
    Serial.printf("%d,%d,%d,%d\n", ret, weight, window_data.mean, edge_cnt ? weight : 0);
    digitalToggle(LED_DEBUG);
}
