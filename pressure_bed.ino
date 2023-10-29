#include "CS1237.h"
#include <Arduino.h>

// IO definision
#define KEY_USER PA8
#define LED_DEBUG PA4
#define LED_PROBE PA5
#define ADC_SCK PA6
#define ADC_DOUT PA7

// pressure sensor configure
#define SENSOR_MAX_WEIGHT 50000
#define SENSOR_MAX_VOLT 1 // mv
#define SENSOR_REVERSE 1

// ADC configure
#define ADC_REF_VOLT 2500 // mv
#define ADC_MAX_VALUE (1 << 24)
#define ADC_PGA 128 // just for reference

// filter configure
#define FILTER_WINDOW_SIZE 20

// edge detection configure
#define WINDOW_SIZE 500
#define THRESHOLD_STATIC 250 // g

typedef struct {
    int64_t *data;
    int64_t *sort_data;
    uint32_t size;
    int64_t sum;
    int64_t mean;
    int64_t mid;
    int64_t index;
} window_data_t;

window_data_t window_data;
window_data_t filter_window_data;


int compare(const void *a, const void *b) {
    return (*(int64_t*)a - *(int64_t*)b);
}

void window_data_init(window_data_t *wd, uint32_t size)
{
    wd->sum = 0;
    wd->mean = 0;
    wd->index = 0;
    wd->mid = 0;
    wd->size = size;
    wd->data = (int64_t *)calloc(size, sizeof(*wd->data));
    wd->sort_data = (int64_t *)calloc(size, sizeof(*wd->sort_data));
}

void window_data_add(window_data_t *wd, int64_t data)
{
    wd->sum -= wd->data[wd->index];
    wd->data[wd->index] = data;
    wd->sum += data;
    wd->mean = wd->sum / (int64_t)wd->size;
    wd->index = (wd->index + 1) % wd->size;
}

void window_data_mid(window_data_t *wd)
{
    memcpy(wd->sort_data, wd->data, wd->size * sizeof(*wd->sort_data));
    qsort(wd->sort_data, wd->size, sizeof(*wd->sort_data), compare);
    wd->mid = wd->sort_data[wd->size / 2];
}

void setup()
{
    //need high baudrate so it won't delay ADC read
    Serial.begin(2000000);
    Serial.println("Pressure bed starting...");

    pinMode(LED_DEBUG, OUTPUT);
    pinMode(LED_PROBE, OUTPUT);
    digitalWrite(LED_DEBUG, LOW);
    digitalWrite(LED_PROBE, LOW);

    window_data_init(&window_data, WINDOW_SIZE);
    window_data_init(&filter_window_data, FILTER_WINDOW_SIZE);

    CS1237_init(ADC_SCK, ADC_DOUT);
    if (!CS1237_configure(PGA_128, SPEED_1280, CHANNEL_A))
        Serial.println("ADC configured successfully");
    else
        Serial.println("ADC configuration failed");
}

int64_t adc_to_weight(int64_t value)
{
    int64_t weight;
    int64_t con = SENSOR_MAX_VOLT * ADC_MAX_VALUE * ADC_PGA / (ADC_REF_VOLT * SENSOR_MAX_WEIGHT);
    weight = value / con;

#if SENSOR_REVERSE == 1
    weight = -weight;
#endif
    return weight;
}

void loop()
{
    int64_t ret = 0;
    int64_t weight = 0;

    ret = CS1237_read();
    weight = adc_to_weight(ret);
    window_data_add(&filter_window_data, weight);
    window_data_mid(&filter_window_data);
    window_data_add(&window_data, filter_window_data.mid);
    if(weight - window_data.mean >= THRESHOLD_STATIC) {
        digitalWrite(LED_PROBE, HIGH);
    } else {
        digitalWrite(LED_PROBE, LOW);
    }
    //debug log
    Serial.printf("%ld,%ld,%ld,%ld\n", (int32_t)ret,(int32_t) weight, (int32_t)filter_window_data.mid, (int32_t)window_data.mean);
    digitalToggle(LED_DEBUG);
}
