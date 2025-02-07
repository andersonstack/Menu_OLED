#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/binary_info.h"
#include "telas.h"
#include "hardware/clocks.h"
#include <stdbool.h>
#include "hardware/adc.h" // Biblioteca para manipulação do ADC no RP2040
#include "hardware/pwm.h" // Biblioteca para controle de PWM no RP2040
#include "ssd1306_i2c.h"

#define DO 13200
#define RE 14850
#define MI 16500
#define FA 17595
#define SOL 19800
#define LA 22004
#define SI 24750
#define DO2 26400

int melody[] = {
  DO, RE, MI, FA, SOL, LA, SI, DO2, SI, LA, SOL, FA, MI, RE, DO, DO, MI, SOL, DO2, SOL, MI, DO
};

int durations[] = {
  500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

const uint LED = 12;            // Pino do LED conectado
const uint16_t PERIOD_LED = 2000;   // Período do PWM (valor máximo do contador)
const uint16_t LED_STEP = 100;  // Passo de incremento/decremento para o duty cycle do LED
uint16_t led_level = 0; 

// Configuração do pino do buzzer
#define BUZZER_PIN 21
#define BUZZER_FREQUENCY 100

// variaveis globais para estado do botão do joystick, opção do menu selecionada e se esta executando rotina de joystick + led (para não executar mudança de menu)
static volatile bool state_joy_btn = false;
int options_menu = 1;
static volatile bool is_joy_led = false;

// Configuração do display OLED 
#define I2C_SDA_PIN 14 
#define I2C_SCL_PIN 15
#define OLED_WIDTH 128 
#define OLED_HEIGHT 32 

// Configuração do joystick
const int VRX = 26;          // Pino de leitura do eixo X do joystick (conectado ao ADC)
const int VRY = 27;          // Pino de leitura do eixo Y do joystick (conectado ao ADC)
const int ADC_CHANNEL_0 = 0; // Canal ADC para o eixo X do joystick
const int ADC_CHANNEL_1 = 1; // Canal ADC para o eixo Y do joystick
const int SW = 22;           // Pino de leitura do botão do joystick
uint16_t vrx_value, vry_value, sw_value; // valores que guardam posicao do joystick e valor do botão

// configs para o led com o joystick
const int LED_B = 12;                    // Pino para controle do LED azul via PWM
const int LED_R = 13;                    // Pino para controle do LED vermelho via PWM
const float DIVIDER_PWM = 16.0;          // Divisor fracional do clock para o PWM
const uint16_t PERIOD = 4096;            // Período do PWM (valor máximo do contador)
uint16_t led_b_level, led_r_level = 100; // Inicialização dos níveis de PWM para os LEDs
uint slice_led_b, slice_led_r;          // Variáveis para armazenar os slices de PWM correspondentes aos LEDs

 
void oled_setup(ssd1306_t *oled, uint16_t width, uint16_t height, uint8_t address, i2c_inst_t *i2c_instance) {
    // Inicializando o I2C
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Inicializando o display
    ssd1306_init();
}


void joystick_setup() {
  // Inicializa o ADC e os pinos de entrada analógica
  adc_init();         // Inicializa o módulo ADC
  adc_gpio_init(VRX); // Configura o pino VRX (eixo X) para entrada ADC
  adc_gpio_init(VRY); // Configura o pino VRY (eixo Y) para entrada ADC

  // Inicializa o pino do botão do joystick
  gpio_init(SW);             // Inicializa o pino do botão
  gpio_set_dir(SW, GPIO_IN); // Configura o pino do botão como entrada
  gpio_pull_up(SW);          // Ativa o pull-up no pino do botão para evitar flutuações
} 

void display_bitmap(int mode){
  ssd1306_t ssd_bm;
  ssd1306_init_bm(&ssd_bm, 128, 64, false, 0x3C, i2c1);
  ssd1306_config(&ssd_bm);

  switch (mode) {
    case 1:
      ssd1306_draw_bitmap(&ssd_bm, joystick_select);
      break;
    case 2:
      ssd1306_draw_bitmap(&ssd_bm, buzzer_select);
      break;
    case 3:
      ssd1306_draw_bitmap(&ssd_bm, led_select);
      break;
    case 4:
      ssd1306_draw_bitmap(&ssd_bm, exec_buzzer);
      break;
    case 5:
      ssd1306_draw_bitmap(&ssd_bm, exec_joy);
      break;
    case 6:
      ssd1306_draw_bitmap(&ssd_bm, exec_led);
      break;
  }  
}
 
void joystick_read_axis_and_button(uint16_t *vrx_value, uint16_t *vry_value, uint16_t *sw_value) {
  // Leitura do valor do eixo X do joystick
  adc_select_input(ADC_CHANNEL_0); // Seleciona o canal ADC para o eixo X
  sleep_us(2);                     // Pequeno delay para estabilidade
  *vrx_value = adc_read();         // Lê o valor do eixo X (0-4095)

  // Leitura do valor do eixo Y do joystick
  adc_select_input(ADC_CHANNEL_1); // Seleciona o canal ADC para o eixo Y
  sleep_us(2);                     // Pequeno delay para estabilidade
  *vry_value = adc_read();
  *sw_value = gpio_get(SW);
          // Lê o valor do eixo Y (0-4095)
}

// Inicializa o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);

    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o PWM com frequência desejada
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 16.0); // Divisor de clock
    // pwm_set_clkdiv(slice_num, 32.0);
    pwm_init(slice_num, &config, true);

    // Iniciar o PWM no nível baixo
    pwm_set_gpio_level(pin, 0); 
}

void setup_pwm_led() {
    uint slice;
    gpio_set_function(LED, GPIO_FUNC_PWM); // Configura o pino do LED para função PWM
    slice = pwm_gpio_to_slice_num(LED);    // Obtém o slice do PWM associado ao pino do LED
    pwm_set_clkdiv(slice, DIVIDER_PWM);    // Define o divisor de clock do PWM
    pwm_set_wrap(slice, PERIOD_LED);       // Configura o valor máximo do contador (período do PWM)
    pwm_set_gpio_level(LED, led_level);    // Define o nível inicial do PWM para o pino do LED
    pwm_set_enabled(slice, true);          // Habilita o PWM no slice correspondente
}

void setup_pwm_leds_joy(uint led, uint *slice, uint16_t level)
{
  gpio_set_function(led, GPIO_FUNC_PWM); // Configura o pino do LED como saída PWM
  *slice = pwm_gpio_to_slice_num(led);   // Obtém o slice do PWM associado ao pino do LED
  pwm_set_clkdiv(*slice, DIVIDER_PWM);   // Define o divisor de clock do PWM
  pwm_set_wrap(*slice, PERIOD);          // Configura o valor máximo do contador (período do PWM)
  pwm_set_gpio_level(led, level);        // Define o nível inicial do PWM para o LED
  pwm_set_enabled(*slice, true);         // Habilita o PWM no slice correspondente ao LED
}

// Toca uma nota com a frequência e duração especificadas
void play_tone(uint pin, uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;

    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top / 2); // 50% de duty cycle

    sleep_ms(duration_ms);

    pwm_set_gpio_level(pin, 0); // Desliga o som após a duração
    sleep_ms(50); // Pausa entre notas
}


// Função principal para tocar a música
void play_music() {
    // play_up_sound();
    printf("playing music...");
    const int pin = 21;
    for (int i = 0; i < sizeof(melody) / sizeof(melody[0]); i++) {
      if (state_joy_btn == true) {
        break;
      };
      if (melody[i] == 0) {
        sleep_ms(durations[i]);
      } else {
        play_tone(pin, melody[i], durations[i]);
      }
    }
    state_joy_btn = false;
}



void reading_joystick() {
  // executo tanto as funções de ler os dados do joystick quando  de variar as opções de menu e mudar a tela se necessarios
    bool changed_menu = false;
    joystick_read_axis_and_button(&vrx_value, &vry_value, &sw_value);
    if (0 == sw_value) {
        state_joy_btn = true;
        printf("botão do joystick pressionado: %d \n", state_joy_btn);
        sleep_ms(250);
    }
    // printf("x axys joystick: %d - state_btn: %d - options menu: %d - is_joy_led: %d \n", vrx_value, state_joy_btn, options_menu, is_joy_led);
    if (is_joy_led == true) {
      return;
    }
    if (100 > vrx_value) {
      if (options_menu < 3) {
        options_menu += 1;
        changed_menu = true;
        sleep_ms(500);
      }
    }
    if (4000 < vrx_value) {
      if (options_menu > 1) {
        options_menu -= 1;
        changed_menu = true;
        sleep_ms(500);
      }
    }

    if (changed_menu == true) {
      switch (options_menu) {
        case 1:
          display_bitmap(1);
          break;
        case 2:
          display_bitmap(2);
          break;
        case 3:
          display_bitmap(3);
          break;
      }
    }
    
}

void led_rgb_routine() {
    uint up_down = 1;
    led_level = 100;
    state_joy_btn = false;
    
    while (state_joy_btn == false) {
        pwm_set_gpio_level(LED, led_level);
        // reading_joystick();
        sleep_ms(250);
        if (up_down) {
            led_level += LED_STEP; // Incrementa o nível do LED
            if (led_level >= PERIOD_LED)
                up_down = 0; // Muda direção para diminuir quando atingir o período máximo
            }
            else {
                led_level -= LED_STEP; // Decrementa o nível do LED
                if (led_level <= LED_STEP)
                    up_down = 1; // Muda direção para aumentar quando atingir o mínimo
            }
    }

    // resetando valores e desligando led
    state_joy_btn = false;
    led_level = 0;
    pwm_set_gpio_level(LED, led_level);        
}

void led_plus_joystick_routine() {
    is_joy_led = true;
    setup_pwm_leds_joy(LED_B, &slice_led_b, led_b_level); // Configura o PWM para o LED azul
    setup_pwm_leds_joy(LED_R, &slice_led_r, led_r_level); // Configura o PWM para o LED vermelho

    while (state_joy_btn == false) {
        reading_joystick();
        pwm_set_gpio_level(LED, vrx_value);
        // pwm_set_gpio_level(LED_B, vrx_value); // Ajusta o brilho do LED azul com o valor do eixo X
        pwm_set_gpio_level(LED_R, vry_value); // Ajusta o brilho do LED vermelho com o valor do eixo Y
        // Pequeno delay antes da próxima leitura
        sleep_ms(100); // Espera 100 ms antes de repetir o ciclo
    }
    // desligando led e resetando flags ao sair da rotina
    state_joy_btn = false;
    pwm_set_gpio_level(LED_B, 0); 
    pwm_set_gpio_level(LED_R, 0);
    is_joy_led = false;
}


void gpio_joystick_irq_handler(uint gpio_pin, uint32_t events) {
   if (state_joy_btn == false) {
      state_joy_btn = true;
   }
}

int main() { 
    stdio_init_all();
    //setup joystick
    joystick_setup(); 
    // Inicialização do OLED
    ssd1306_t oled;
    oled_setup(&oled, 128, 64, 0x3C, i2c1);
    // inicialização buzzer
    pwm_init_buzzer(BUZZER_PIN);
    // inicialização led pwm
    setup_pwm_led();
    // evento de interrupção para avisar que o botão foi clicado no meio da execução de alguma rotina
    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_FALL, true, &gpio_joystick_irq_handler);

    printf("Programa principal iniciado. Tudo configurado\n");
    // tela inicial
    display_bitmap(1);    
 
    while (true) {
      reading_joystick();
      if (true == state_joy_btn) {
          switch (options_menu) {
            case 1:
                printf("Executando rotina do joystick rgb\n");
                state_joy_btn = false;                
                display_bitmap(5);
                led_plus_joystick_routine();
                printf("Saindo da rotina de joystick rgb\n");
                display_bitmap(1);
                sleep_ms(250);                        
                break;
            case 2:
                printf("Entrando rotina do buzzer\n");
                state_joy_btn = false;
                display_bitmap(4);
                play_music();
                printf("Saindo da rotina de buzzer\n");
                display_bitmap(2);
                sleep_ms(250);
                break;
            case 3:
                printf("Entrando em rotina de led rgb\n");
                display_bitmap(6);
                led_rgb_routine();
                printf("Saindo da rotina de led rgb\n");
                display_bitmap(3);
                sleep_ms(250);                
                break;
            default:
                printf("ERRO AO SELECIONAR OPÇÕES!!\n");
                break;
          }
      }      
      sleep_ms(10);          
    } 
    return 0; 
}