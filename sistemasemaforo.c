#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

// Definição dos pinos
#define LEDG_PIN 11  // LED Verde (veículos)
#define LEDY_PIN 12  // LED Amarelo (transição)
#define LEDR_PIN 13  // LED Vermelho (pedestres)
#define BUZ_A_PIN 10 // BUZER A
#define BUZ_B_PIN 21 // BUZZER B
#define TRIG_PIN 19  // Pino de trigger do HC-SR04
#define ECHO_PIN 20  // Pino de echo do HC-SR04

// Tempos de cada estado (em milissegundos)
#define TEMPO_VERDE 5000       // Tempo padrão para verde (veículos)
#define TEMPO_VERMELHO 30000    // Tempo para vermelho (pedestres)
#define TEMPO_AMARELO 10000     // Tempo para amarelo (transição)
#define DISTANCIA_MAXIMA 150   // Distância máxima para detecção de pedestres (em cm)
#define DISTANCIA_MINIMA 30   // Distância mínima para detecção de pedestres (em cm)

// Definição das frequências das notas musicais (em Hz)
#define NOTA_DING  784  // Frequência para o "ding" (G5)
#define NOTA_DONG  784 // Frequência para o "dong" (D5)

// Função para inicializar os pinos
void inicializar_pinos() {
    gpio_init(LEDG_PIN);
    gpio_init(LEDY_PIN);
    gpio_init(LEDR_PIN);
    gpio_init(BUZ_A_PIN);
    gpio_init(BUZ_B_PIN);
    gpio_init(TRIG_PIN);
    gpio_init(ECHO_PIN);

    gpio_set_dir(LEDG_PIN, GPIO_OUT);
    gpio_set_dir(LEDY_PIN, GPIO_OUT);
    gpio_set_dir(LEDR_PIN, GPIO_OUT);
    gpio_set_dir(BUZ_A_PIN, GPIO_OUT);
    gpio_set_dir(BUZ_B_PIN, GPIO_OUT);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_set_dir(ECHO_PIN, GPIO_IN);

    gpio_put(LEDG_PIN, 1); // Inicia com verde para veículos
    gpio_put(LEDY_PIN, 0);
    gpio_put(LEDR_PIN, 0);
    gpio_put(BUZ_A_PIN, 0); // Buzzer desligado
    gpio_put(BUZ_B_PIN, 0); // Buzzer desligado
    gpio_put(TRIG_PIN, 0);  // Inicia com o trigger desligado
}

// Função para medir a distância com o HC-SR04
float medir_distancia() {
    float distancia = 0;
    int leituras_validas = 0;
    const int num_leituras = 5; // Número de leituras para a média

    for (int i = 0; i < num_leituras; i++) {
        // Envia um pulso de 10us no pino TRIG
        gpio_put(TRIG_PIN, 1);
        sleep_us(10);
        gpio_put(TRIG_PIN, 0);

        // Espera pelo pulso de retorno no pino ECHO
        while (gpio_get(ECHO_PIN) == 0);
        absolute_time_t start_time = get_absolute_time();
        while (gpio_get(ECHO_PIN) == 1);
        absolute_time_t end_time = get_absolute_time();

        // Calcula a duração do pulso de retorno
        int64_t pulse_duration = absolute_time_diff_us(start_time, end_time);

        // Calcula a distância em cm
        float distancia_atual = (pulse_duration * 0.0343) / 2;

        // Filtra leituras inválidas (muito curtas ou muito longas)
        if (distancia_atual >= 2 && distancia_atual <= 400) {
            distancia += distancia_atual;
            leituras_validas++;
        }

        sleep_ms(10); // Intervalo entre leituras
    }

    if (leituras_validas > 0) {
        distancia /= leituras_validas; // Calcula a média das leituras válidas
    } else {
        distancia = 0; // Retorna 0 se nenhuma leitura for válida
    }

    return distancia;
}

// Função para tocar uma nota no buzzer
void tocar_nota(int pino_buzzer, int frequencia, int duracao) {
  if (frequencia == 0) {
      // Se a frequência for 0, é uma pausa (silêncio)
      pwm_set_gpio_level(pino_buzzer, 0);
      sleep_ms(duracao);
  } else {
      // Configura o PWM para a frequência da nota
      uint slice_num = pwm_gpio_to_slice_num(pino_buzzer);
      uint channel_num = pwm_gpio_to_channel(pino_buzzer);
      pwm_set_clkdiv(slice_num, 125000000 / (frequencia * 256)); // Configura o divisor de clock
      pwm_set_wrap(slice_num, 256);
      pwm_set_chan_level(slice_num, channel_num, 128); // Duty cycle de 50%
      pwm_set_enabled(slice_num, true);
      sleep_ms(duracao); // Mantém a nota pelo tempo especificado
      pwm_set_enabled(slice_num, false); // Desliga o PWM
  }
}

// Função para emitir sinal sonoro contínuo
void emitir_sinal_continuo() {
    // Inicializa os GPIOs dos buzzers
    gpio_set_function(BUZ_A_PIN, GPIO_FUNC_PWM);
    gpio_set_function(BUZ_B_PIN, GPIO_FUNC_PWM);

    // Melodia: "Ding Dong" (frequências e durações)
    int melodia[] = {NOTA_DING, NOTA_DONG};
    int duracoes[] = {10, 10};  // Durações para "ding" e "dong"

    // Tempo total de execução: 10 segundos
    int tempo_total = TEMPO_VERMELHO; // 10 segundos em milissegundos
    int tempo_decorrido = 0;

    do {
        // Toca "ding" no buzzer A
        tocar_nota(BUZ_A_PIN, melodia[0], duracoes[0]);
        sleep_ms(100); // Pequena pausa entre as notas

        // Delay de 1 segundo
        sleep_ms(1000);
        tempo_decorrido += 1000 + duracoes[0] + 100;

        // Toca "dong" no buzzer B
        tocar_nota(BUZ_B_PIN, melodia[1], duracoes[1]);
        sleep_ms(100); // Pequena pausa entre as notas

        // Delay de 1 segundo
        sleep_ms(1000);
        tempo_decorrido += 1000 + duracoes[1] + 100;
    } while (tempo_decorrido < tempo_total);
}

// Função para emitir sinal sonoro intermitente
void emitir_sinal_intermitente() {
    // Inicializa os GPIOs dos buzzers
    gpio_set_function(BUZ_A_PIN, GPIO_FUNC_PWM);
    gpio_set_function(BUZ_B_PIN, GPIO_FUNC_PWM);

    // Melodia: "Ding Dong" (frequências e durações)
    int melodia[] = {NOTA_DING, NOTA_DONG};
    int duracoes[] = {300, 500};  // Durações para "ding" e "dong"

    // Tempo total de execução: 10 segundos
    int tempo_total = TEMPO_AMARELO; // 10 segundos em milissegundos
    int tempo_decorrido = 0;

    do {
        // Toca "ding" no buzzer A
        tocar_nota(BUZ_A_PIN, melodia[0], duracoes[0]);
        sleep_ms(100); // Pequena pausa entre as notas

        // Delay de 1 segundo
        sleep_ms(1000);
        tempo_decorrido += 1000 + duracoes[0] + 100;

        // Toca "dong" no buzzer B
        tocar_nota(BUZ_B_PIN, melodia[1], duracoes[1]);
        sleep_ms(100); // Pequena pausa entre as notas

        // Delay de 1 segundo
        sleep_ms(1000);
        tempo_decorrido += 1000 + duracoes[1] + 100;
    } while (tempo_decorrido < tempo_total);
}

// Função principal
int main() {
    stdio_init_all();
    inicializar_pinos();

    while (true) {
        
      do{
        // Mede a distância do sensor
        float distancia = medir_distancia();
        printf("\nDistancia detectada: %.2f", distancia);
        // Verifica se há um pedestre a uma distância de 10 cm até 50 cm
        if (distancia >= DISTANCIA_MINIMA && distancia <= DISTANCIA_MAXIMA) {
          // Aguarda 10 segundos com o LED verde ligado
          sleep_ms(10000);
          

          // Desliga o LED verde e liga o LED amarelo
          gpio_put(LEDG_PIN, 1);
          gpio_put(LEDR_PIN, 1);
          emitir_sinal_intermitente(); // Emite sinal sonoro intermitente

          // Desliga o LED amarelo e liga o LED vermelho
          gpio_put(LEDG_PIN, 0);
          gpio_put(LEDR_PIN, 1);
          emitir_sinal_continuo(); // Emite sinal sonoro contínuo

          // Volta para o estado inicial (verde para veículos)
          inicializar_pinos();
        } else {
            // Mantém o estado verde para veículos
            gpio_put(LEDG_PIN, 1);
            gpio_put(LEDY_PIN, 0);
            gpio_put(LEDR_PIN, 0);
        }

        // Intervalo entre leituras do sensor
        sleep_ms(100);

      }while(medir_distancia());
      
    }
}