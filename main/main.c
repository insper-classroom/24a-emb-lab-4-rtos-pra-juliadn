/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "ssd1306.h"
#include "gfx.h"

#include "pico/stdlib.h"
#include <stdio.h>

const uint BTN_1_OLED = 28;
const uint BTN_2_OLED = 26;
const uint BTN_3_OLED = 27;

const uint LED_1_OLED = 20;
const uint LED_2_OLED = 21;
const uint LED_3_OLED = 22;

SemaphoreHandle_t xSemaphore_r;
QueueHandle_t xQueueButId;
QueueHandle_t xQueueButId2;

const int TRIGGER_PIN = 16;
const int ECHO_PIN = 17;


void ECHO_PIN_callback(uint gpio, uint32_t events) {
    static BaseType_t xHigherPriorityTaskWoken;
    static absolute_time_t rise_time;
    static absolute_time_t fall_time;
    
    if (events == 0x4) { // fall edge
        fall_time = get_absolute_time();
        
        uint32_t delta_t = absolute_time_diff_us(rise_time, fall_time);
        xQueueSendFromISR(xQueueButId, &delta_t,0);
        
    }
    if (events == 0x8) { // rise edge
        rise_time = get_absolute_time();
        
    }
}
void oled1_btn_led_init(void) {
    gpio_init(LED_1_OLED);
    gpio_set_dir(LED_1_OLED, GPIO_OUT);

    gpio_init(LED_2_OLED);
    gpio_set_dir(LED_2_OLED, GPIO_OUT);

    gpio_init(LED_3_OLED);
    gpio_set_dir(LED_3_OLED, GPIO_OUT);

    gpio_init(BTN_1_OLED);
    gpio_set_dir(BTN_1_OLED, GPIO_IN);
    gpio_pull_up(BTN_1_OLED);

    gpio_init(BTN_2_OLED);
    gpio_set_dir(BTN_2_OLED, GPIO_IN);
    gpio_pull_up(BTN_2_OLED);

    gpio_init(BTN_3_OLED);
    gpio_set_dir(BTN_3_OLED, GPIO_IN);
    gpio_pull_up(BTN_3_OLED);
}

void echo_task(){
    uint32_t delta_t;
    
    while(true){
        if(xQueueReceive(xQueueButId, &delta_t, 0)){
            float distancia =(delta_t / 10000.0) * 340.0 / 2.0;
            gpio_put(TRIGGER_PIN,1);
            xQueueSend(xQueueButId2, &distancia, 0);
        }
    }
}

void trigger_task(){
    while(true){
        gpio_put(TRIGGER_PIN,1);
        vTaskDelay(30);
        gpio_put(TRIGGER_PIN,0);
        xSemaphoreGive(xSemaphore_r);
        

    }
}
void oled_task(void *p){
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();
    float distancia;
    while (true){
        
        
        
        if(xSemaphoreTake(xSemaphore_r, pdMS_TO_TICKS(500)) == pdTRUE){
            if(xQueueReceive(xQueueButId2, &distancia, 0)){
                printf("Recebeu a distancia %f\n",distancia);
                gfx_clear_buffer(&disp);
                gfx_draw_string(&disp, 0, 0, 1, "Distanciaaaa:");
                char distance_str[16];
                snprintf(distance_str, sizeof(distance_str), "%.2f cm", distancia);
                gfx_draw_string(&disp, 0, 10, 1, distance_str);

                float scale = 0.64; 
                int bar_length = (int)(distancia * scale); 
                if(bar_length > 128) bar_length = 128; 
                gfx_draw_line(&disp, 0, 20, bar_length, 20);

            
                gfx_show(&disp);
            }
            else{
                gfx_clear_buffer(&disp);
                gfx_draw_string(&disp, 0, 0, 1, "Falhou");     
                gfx_show(&disp);       

            }
            
        }
    }

}
int main() {
    stdio_init_all();
    

    gpio_init(TRIGGER_PIN);
    gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
    gpio_put(TRIGGER_PIN, 0);
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(ECHO_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &ECHO_PIN_callback);
    xSemaphore_r = xSemaphoreCreateBinary();
    xQueueButId = xQueueCreate(32, sizeof(int));
    xQueueButId2 = xQueueCreate(32, sizeof(int)); 

    

    xTaskCreate(oled_task, "Oled", 4095, NULL, 1, NULL);
    xTaskCreate(echo_task, "Echo", 4095, NULL, 1, NULL);
    xTaskCreate(trigger_task, "Trigger", 4095, NULL, 1, NULL);
    vTaskStartScheduler();
    

    while (true);
}
