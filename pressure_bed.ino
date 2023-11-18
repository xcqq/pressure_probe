#include "CS1237.h"
#include <Arduino.h>
#include <EEPROM.h>

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
#define WINDOW_SIZE 600
#define THRESHOLD_MIN 100 // g
#define THRESHOLD_MAX 400
#define THRESHOLD_STEP 50
#define BASE_WEIGHT_UPDATE_THRESHOLD 50

#define CONFIG_MAGIC 0xDEADDEAD
typedef struct {
    uint64_t magic;
    int32_t threshold;
} config_t;

config_t config;

typedef struct {
    int32_t *data;
    int32_t *sort_data;
    uint32_t size;
    int64_t sum;
    int64_t mean;
    int64_t mid;
    int64_t max;
    int64_t min;
    uint32_t index;
} window_data_t;

window_data_t window_data;
window_data_t filter_window_data;

int32_t base_weight = 0;

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
    wd->data = (int32_t *)calloc(size, sizeof(*wd->data));
    wd->sort_data = (int32_t *)calloc(size, sizeof(*wd->sort_data));
}

void window_data_add(window_data_t *wd, int32_t data)
{
    wd->sum -= wd->data[wd->index];
    wd->data[wd->index] = data;
    wd->sum += data;
    wd->mean = wd->sum / wd->size;
    wd->index = (wd->index + 1) % wd->size;
}

void window_data_max_min(window_data_t *wd)
{
    int32_t max, min;
    int i;

    max = wd->data[0];
    min = wd->data[0];
    for (i = 1; i < wd->size; i++) {
        if (wd->data[i] > max)
            max = wd->data[i];
        if (wd->data[i] < min)
            min = wd->data[i];
    }
    wd->max = max;
    wd->min = min;
}

void window_data_mid(window_data_t *wd)
{
    memcpy(wd->sort_data, wd->data, wd->size * sizeof(*wd->sort_data));
    qsort(wd->sort_data, wd->size, sizeof(*wd->sort_data), compare);
    wd->mid = wd->sort_data[wd->size / 2];
}

void led_blink(uint32_t pin, uint32_t times, uint32_t delay_ms)
{
    uint32_t i;
    for (i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(delay_ms);
        digitalWrite(pin, LOW);
        delay(delay_ms);
    }
}

void config_set(config_t *config)
{
    int i;

    for (i = 0; i < sizeof(config_t); i++) {
        eeprom_buffered_write_byte(i, *((uint8_t *)config + i));
    }
    eeprom_buffer_flush();
    // once is not enough? Write twice as a workaround
    eeprom_buffer_flush();
}

void config_init(config_t *config)
{
    int i;
    eeprom_buffer_fill();
    for (i = 0; i < sizeof(config_t); i++) {
        *((uint8_t *)config + i) = eeprom_buffered_read_byte(i);
    }
    if (config->magic != CONFIG_MAGIC) {
        config->magic = CONFIG_MAGIC;
        config->threshold = THRESHOLD_MIN;
        config_set(config);
        led_blink(LED_DEBUG, 1, 1000);
    }
}

void config_update_threshold(config_t *config)
{
    int i;

    config->threshold += THRESHOLD_STEP;
    if (config->threshold > THRESHOLD_MAX) {
        config->threshold = THRESHOLD_MIN;
        led_blink(LED_DEBUG, 3, 250);
    } else {
        led_blink(LED_DEBUG, 1, 1000);
    }
    config_set(config);
}

void setup()
{
    //need high baudrate so it won't delay ADC read
    Serial.begin(2000000);
    Serial.println("Pressure bed starting...");

    pinMode(KEY_USER, INPUT_PULLUP);
    pinMode(LED_DEBUG, OUTPUT);
    pinMode(LED_PROBE, OUTPUT);
    digitalWrite(LED_DEBUG, HIGH);
    digitalWrite(LED_PROBE, LOW);

    window_data_init(&window_data, WINDOW_SIZE);
    window_data_init(&filter_window_data, FILTER_WINDOW_SIZE);

    config_init(&config);
    CS1237_init(ADC_SCK, ADC_DOUT);
    if (!CS1237_configure(PGA_128, SPEED_1280, CHANNEL_A))
        Serial.println("ADC configured successfully");
    else
        Serial.println("ADC configuration failed");
}

int32_t adc_to_weight(int32_t value)
{
    int32_t weight;
    int64_t con = SENSOR_MAX_VOLT * ADC_MAX_VALUE * ADC_PGA / (ADC_REF_VOLT * SENSOR_MAX_WEIGHT);
    weight = value / con;

#ifdef SENSOR_REVERSE
        return -weight;
#else
        return weight;
#endif
}

void loop()
{
    int32_t ret = 0;
    int32_t weight = 0;

    ret = CS1237_read();
    weight = adc_to_weight(ret);
    window_data_add(&filter_window_data, weight);
    window_data_add(&window_data, filter_window_data.mean);
    window_data_max_min(&window_data);

    if (window_data.max - window_data.min < BASE_WEIGHT_UPDATE_THRESHOLD) {
        base_weight = window_data.mean;
        digitalWrite(LED_DEBUG, LOW);
    }
    if (base_weight != 0) {
        if (weight - base_weight >= config.threshold) {
            digitalWrite(LED_PROBE, HIGH);
        } else {
            digitalWrite(LED_PROBE, LOW);
        }
    } else {
        digitalWrite(LED_PROBE, HIGH);
    }
    //debug log
    Serial.print("Raw:");
    Serial.print(ret);
    Serial.print(",");
    Serial.print("Weight:");
    Serial.print(filter_window_data.mean);
    Serial.print(",");
    Serial.print("Ref:");
    Serial.print(base_weight);
    Serial.print(",");
    Serial.print("Th:");
    Serial.print(config.threshold);
    Serial.println();
    // check if btn pressed
    if (digitalRead(KEY_USER) == LOW) {
        while (digitalRead(KEY_USER) == LOW);
        config_update_threshold(&config);
    }
}
