// Simon.c
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEBOUNCE_TIME 50 // Tempo mínimo entre leituras (ms)
#define MAX_SEQUENCE 15  // Tamanho máximo da sequência
#define ERROR_FREQ 200   // Frequência do som de erro (Hz)
#define TIMEOUT_MS_MODO1 1500
#define TIMEOUT_MS_MODO3 800  // Tempo máximo para pressionar o botão (ms)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
uint32_t freq_atual = 0;

uint32_t last_time_b1 = 0;
uint32_t last_time_b2 = 0;
uint32_t last_time_b3 = 0;
uint32_t last_time_b4 = 0;

uint8_t last_state_b1 = 1;
uint8_t last_state_b2 = 1;
uint8_t last_state_b3 = 1;
uint8_t last_state_b4 = 1;

uint8_t selected_mode = 0;

uint8_t sequence[MAX_SEQUENCE]; // Armazena a sequência do jogo
uint8_t sequence_length = 0;   // Comprimento atual da sequência
uint8_t player_index = 0;      // Índice da entrada do jogador

/* Modo 3 specific variables */
uint8_t active_colors[4] = {1, 2, 3, 4}; // Cores ativas (1=vermelho, 2=verde, 3=azul, 4=amarelo)
uint8_t num_active_colors = 4;           // Número de cores ativas
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void set_pwm_freq_safe(uint32_t freq_hz) {
        uint32_t period = (60000000 / (1000*freq_hz));
        __HAL_TIM_SET_PRESCALER(&htim3, period);
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

int debounce_falling_edge(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint32_t* last_time, uint8_t* last_state) {
	uint32_t current_time = HAL_GetTick();

	if (current_time - *last_time < DEBOUNCE_TIME) {
		return 0; // Ignora se o tempo de debounce não foi atingido
	}
	int estado = HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET;
	HAL_Delay(5); // Espera 5 ms
	int estado2 = HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET;
	if (estado && estado2 && !*last_state) { // Borda de descida detectada
		*last_time = current_time; // Atualiza o tempo da última leitura válida
		*last_state = 1; // Atualiza o estado atual
		return 1; // Botão pressionado
	}
	if (!estado && !estado2) { // Botão solto, reseta o estado
		*last_state = 0;
	}
	return 0; // Sem borda de descida
}

uint8_t random_number() {
    // Usa o contador de ticks como semente simples
	if (num_active_colors == 0) return 1; // Evita divisão por zero

	return active_colors[abs(HAL_GetTick()) % num_active_colors];
}

void play_final_victory() {
    // Notas simples do tema Mario (adaptadas)
    const uint32_t notes[] = {660, 660, 0, 660, 0, 520, 660, 0, 780};
    const uint32_t durations[] = {150, 150, 150, 150, 150, 150, 150, 150, 300};
    int n = sizeof(notes) / sizeof(notes[0]);

    for (int i = 0; i < n; i++) {
        if (notes[i] != 0) {
            set_pwm_freq_safe(notes[i]);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_SET);
        }
        HAL_Delay(durations[i]);
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);
        HAL_Delay(40);
    }
}

// Reproduz a sequência para o jogador
void play_sequence() {

	uint32_t base_delay = 600; // Delay inicial (ms)
	uint32_t min_delay = 200;  // Delay mínimo (ms)
	uint32_t delay = base_delay - (sequence_length * 20); // Reduz 20ms por nível
	if (delay < min_delay) delay = min_delay; // Limita ao mínimo

    for (uint8_t i = 0; i < sequence_length; i++) {
        // Apaga todos os LEDs
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);

        // Acende o LED correspondente e toca o som
        switch (sequence[i]) {
            case 1:
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
                set_pwm_freq_safe(540);
                break;
            case 2:
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
                set_pwm_freq_safe(800);
                break;
            case 3:
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
                set_pwm_freq_safe(1100);
                break;
            case 4:
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
                set_pwm_freq_safe(1500);
                break;
        }
        HAL_Delay(delay); // LED e som ligados por 500 ms
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);
        HAL_Delay(delay / 6); // Pausa entre notas
    }
}

// Toca o som de erro
void play_error() {
    int pisca;

    for (pisca = 0; pisca <= 2; pisca++){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_SET);
		set_pwm_freq_safe(ERROR_FREQ);
		HAL_Delay(500); // Som de erro por 1 segundo
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);
		HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
		HAL_Delay(500);
    }
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_SET);
    set_pwm_freq_safe(ERROR_FREQ);
    HAL_Delay(500);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_Delay(1000);
}

void play_error_modo_3(uint8_t color) {
    uint16_t error_pin;
    uint32_t error_freq;
    switch (color) {
        case 1:
            error_pin = GPIO_PIN_3;  // Vermelho
            error_freq = 200;        // Grave
            break;
        case 2:
            error_pin = GPIO_PIN_2;  // Verde
            error_freq = 250;        // Um pouco mais agudo
            break;
        case 3:
            error_pin = GPIO_PIN_1;  // Azul
            error_freq = 300;        // Médio
            break;
        case 4:
            error_pin = GPIO_PIN_0;  // Amarelo
            error_freq = 350;        // Mais agudo
            break;
        default:
            return; // Cor inválida
    }
    for (int pisca = 0; pisca < 3; pisca++) {
        HAL_GPIO_WritePin(GPIOA, error_pin, GPIO_PIN_SET);
        set_pwm_freq_safe(error_freq);
        HAL_Delay(300);
        HAL_GPIO_WritePin(GPIOA, error_pin, GPIO_PIN_RESET);
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        HAL_Delay(300);
    }
}

void play_victory(uint8_t winner_color) {
    // Som de vitória: sequência ascendente, pisca apenas o LED do vencedor
    uint16_t winner_pin;
    uint32_t winner_freq;
    switch (winner_color) {
        case 1:
            winner_pin = GPIO_PIN_3;  // Vermelho
            winner_freq = 540;        // Lá
            break;
        case 2:
            winner_pin = GPIO_PIN_2;  // Verde
            winner_freq = 800;        // Dó
            break;
        case 3:
            winner_pin = GPIO_PIN_1;  // Azul
            winner_freq = 1100;       // Mi
            break;
        case 4:
            winner_pin = GPIO_PIN_0;  // Amarelo
            winner_freq = 1500;       // Sol
            break;
        default:
            return; // Cor inválida, não faz nada
    }
    for (int i = 0; i < 3; i++) { // Pisca 3 vezes
        HAL_GPIO_WritePin(GPIOA, winner_pin, GPIO_PIN_SET);
        set_pwm_freq_safe(winner_freq);
        HAL_Delay(200);
        HAL_GPIO_WritePin(GPIOA, winner_pin, GPIO_PIN_RESET);
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        HAL_Delay(200);
    }
}

// Adiciona um novo número à sequência
void add_to_sequence() {
    if (sequence_length < MAX_SEQUENCE) {
        sequence[sequence_length] = random_number();
        sequence_length++;
    }
}

// Reinicia o jogo
void reset_game() {
    sequence_length = 0;
    player_index = 0;
    num_active_colors = 4;
    active_colors[0] = 1;
    active_colors[1] = 2;
    active_colors[2] = 3;
    active_colors[3] = 4;
    add_to_sequence();
}

void eliminate_color(uint8_t color) {
    uint8_t new_active_colors[4];
    uint8_t new_index = 0;
    for (uint8_t i = 0; i < num_active_colors; i++) {
        if (active_colors[i] != color) {
            new_active_colors[new_index++] = active_colors[i];
        }
    }
    num_active_colors = new_index;
    for (uint8_t i = 0; i < num_active_colors; i++) {
        active_colors[i] = new_active_colors[i];
    }
    sequence_length = 0; // Reinicia sequência
    add_to_sequence();
}

int check_mode_change() {
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_RESET) {
        selected_mode = 1;
        return 1;
    }
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_RESET) {
        selected_mode = 2;
        return 1;
    }
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET) {
        selected_mode = 3;
        return 1;
    }
    return 0;
}

void modo_1(){
    reset_game();

    while(1){
        play_sequence();

        // Captura a entrada do jogador
        player_index = 0;
        while (player_index < sequence_length) {
            uint32_t start_time = HAL_GetTick();
            uint8_t button_pressed = 0;
            uint8_t button_value = 0;
            while (!button_pressed && (HAL_GetTick() - start_time < TIMEOUT_MS_MODO1)) {
                // Verifica botões com debounce
                int b1 = debounce_falling_edge(GPIOB, GPIO_PIN_13, &last_time_b1, &last_state_b1);
                int b2 = debounce_falling_edge(GPIOB, GPIO_PIN_12, &last_time_b2, &last_state_b2);
                int b3 = debounce_falling_edge(GPIOB, GPIO_PIN_15, &last_time_b3, &last_state_b3);
                int b4 = debounce_falling_edge(GPIOB, GPIO_PIN_14, &last_time_b4, &last_state_b4);
                int b5 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_RESET ||
                         HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_RESET ||
                         HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET;

                // Determina qual botão foi pressionado
                if (b1) {
                    button_pressed = 1;
                    button_value = 1;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
                    set_pwm_freq_safe(540);
                    HAL_Delay(400);
                    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
                } else if (b2) {
                    button_pressed = 1;
                    button_value = 2;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
                    set_pwm_freq_safe(800);
                    HAL_Delay(400);
                    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
                } else if (b3) {
                    button_pressed = 1;
                    button_value = 3;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
                    set_pwm_freq_safe(1100);
                    HAL_Delay(400);
                    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
                } else if (b4) {
                    button_pressed = 1;
                    button_value = 4;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
                    set_pwm_freq_safe(1500);
                    HAL_Delay(400);
                    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
                } else if (b5) {
                    return;
                }
                HAL_Delay(10); // Delay para estabilidade
            }
            if (!button_pressed) {
                // Timeout: toca som de erro e reinicia
                play_error();
                reset_game();
                break;
            }
            // Verifica se o botão pressionado está correto
            if (button_value == sequence[player_index]) {
                player_index++; // Avança para o próximo na sequência
            } else {
                // Erro: toca som grave e reinicia o modo atual
                play_error();
                reset_game();
                break; // Volta ao início de modo_1
            }
        }
        // Se o jogador completou a sequência, adiciona um novo número
        if (player_index == sequence_length) {
            if (sequence_length >= MAX_SEQUENCE) {
                play_final_victory();
                HAL_Delay(1500);
                reset_game();
                continue; // volta ao início do modo
            }
            HAL_Delay(300);
            add_to_sequence();
        }
    }
}

void modo_2() {
	uint8_t player_sequence[MAX_SEQUENCE];
	uint8_t player_length = 0;
	uint8_t input_index = 0;

	// Limpa a sequência anterior
	for (int i = 0; i < MAX_SEQUENCE; i++) {
		player_sequence[i] = 0;
	}

	while (1) {
		// --- FASE DE REPETIÇÃO ---
		input_index = 0;
		while (input_index < player_length) {

			int b1 = debounce_falling_edge(GPIOB, GPIO_PIN_13, &last_time_b1, &last_state_b1);
			int b2 = debounce_falling_edge(GPIOB, GPIO_PIN_12, &last_time_b2, &last_state_b2);
			int b3 = debounce_falling_edge(GPIOB, GPIO_PIN_15, &last_time_b3, &last_state_b3);
			int b4 = debounce_falling_edge(GPIOB, GPIO_PIN_14, &last_time_b4, &last_state_b4);

			int b5 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_RESET ||
					 HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_RESET ||
					 HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET;

			uint8_t button_value = 0;

			if (b1) button_value = 1;
			else if (b2) button_value = 2;
			else if (b3) button_value = 3;
			else if (b4) button_value = 4;
			else if (b5) button_value = 5;

			if (button_value != 0) {
				// Toca o som e acende o LED correspondente
				switch (button_value) {
					case 1:
						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
						set_pwm_freq_safe(540);
						break;
					case 2:
						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
						set_pwm_freq_safe(800);
						break;
					case 3:
						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
						set_pwm_freq_safe(1100);
						break;
					case 4:
						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
						set_pwm_freq_safe(1500);
						break;
					case 5:
						return;
						break;
				}
				HAL_Delay(400);
				HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);

				// Verifica se apertou o botão certo da sequência
				if (button_value != player_sequence[input_index]) {
					play_error();
					return;
				}

				input_index++;
			}

			HAL_Delay(10);
		}

		// --- FASE DE NOVA COR ---

		// Espera o jogador escolher a próxima cor
		uint8_t nova_cor = 0;
		while (nova_cor == 0) {

			int b1 = debounce_falling_edge(GPIOB, GPIO_PIN_13, &last_time_b1, &last_state_b1);
			int b2 = debounce_falling_edge(GPIOB, GPIO_PIN_12, &last_time_b2, &last_state_b2);
			int b3 = debounce_falling_edge(GPIOB, GPIO_PIN_15, &last_time_b3, &last_state_b3);
			int b4 = debounce_falling_edge(GPIOB, GPIO_PIN_14, &last_time_b4, &last_state_b4);

			int b5 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_RESET ||
					 HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_RESET ||
					 HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET;

			if (b1) nova_cor = 1;
			else if (b2) nova_cor = 2;
			else if (b3) nova_cor = 3;
			else if (b4) nova_cor = 4;
			else if (b5) nova_cor = 5;

			if (nova_cor != 0) {
				// Toca o som e acende o LED correspondente
				switch (nova_cor) {
					case 1:
						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
						set_pwm_freq_safe(540);
						break;
					case 2:
						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
						set_pwm_freq_safe(800);
						break;
					case 3:
						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
						set_pwm_freq_safe(1100);
						break;
					case 4:
						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
						set_pwm_freq_safe(1500);
						break;
					case 5:
						return;
						break;
				}
				HAL_Delay(400);
				HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);

				// Adiciona a nova cor à sequência
				if (player_length < MAX_SEQUENCE) {
					player_sequence[player_length] = nova_cor;
					player_length++;
				} else {
					play_error();
					return;
				}
			}

			HAL_Delay(10);
		}

		HAL_Delay(300);
	}
}

void modo_3() {
    reset_game();

    while (1) {
        play_sequence();
        player_index = 0;
        while (player_index < sequence_length) {
            uint32_t start_time = HAL_GetTick();
            uint8_t expected_color = sequence[player_index];
            uint8_t button_pressed = 0;
            uint8_t button_value = 0;
            while (!button_pressed && (HAL_GetTick() - start_time < TIMEOUT_MS_MODO3)) {
                int b1 = debounce_falling_edge(GPIOB, GPIO_PIN_13, &last_time_b1, &last_state_b1);
                int b2 = debounce_falling_edge(GPIOB, GPIO_PIN_12, &last_time_b2, &last_state_b2);
                int b3 = debounce_falling_edge(GPIOB, GPIO_PIN_15, &last_time_b3, &last_state_b3);
                int b4 = debounce_falling_edge(GPIOB, GPIO_PIN_14, &last_time_b4, &last_state_b4);
                int b5 = check_mode_change();
                if (b1) {
                    button_pressed = 1;
                    button_value = 1;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
                    set_pwm_freq_safe(540);
                    HAL_Delay(400);
                    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
                } else if (b2) {
                    button_pressed = 1;
                    button_value = 2;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
                    set_pwm_freq_safe(800);
                    HAL_Delay(400);
                    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
                } else if (b3) {
                    button_pressed = 1;
                    button_value = 3;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
                    set_pwm_freq_safe(1100);
                    HAL_Delay(400);
                    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
                } else if (b4) {
                    button_pressed = 1;
                    button_value = 4;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
                    set_pwm_freq_safe(1500);
                    HAL_Delay(400);
                    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
                } else if (b5) {
                    return;
                }
                HAL_Delay(10);
            }
            if (!button_pressed) {
                // Timeout: elimina a cor esperada
                play_error_modo_3(expected_color);
                eliminate_color(expected_color);
                if (num_active_colors == 1) {
                    // Apenas uma cor resta, ela é a vencedora
                    play_victory(active_colors[0]);
                    return; // Fim do jogo
                }
                break; // Reinicia com nova sequência
            } else if (button_value != expected_color) {
                // Erro: elimina a cor pressionada errada
                play_error_modo_3(button_value);
                eliminate_color(button_value);
                if (num_active_colors == 1) {
                    // Apenas uma cor resta, ela é a vencedora
                    play_victory(active_colors[0]);
                    return; // Fim do jogo
                }
                break; // Reinicia com nova sequência
            } else {
                player_index++;
            }
        }
        if (player_index == sequence_length) {
            if (sequence_length >= MAX_SEQUENCE) {
                play_final_victory();
                HAL_Delay(1500);
                reset_game();
                continue;
            }
            HAL_Delay(300);
            add_to_sequence();
        }
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);

      // Inicializa LEDs apagados
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);

  reset_game();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // Estados dos botões com debounce
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_SET);
	  HAL_Delay(500); // Som de erro por 1 segundo
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);
	  HAL_Delay(500);

	  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_RESET) {
		  selected_mode = 1;
	  }
	  else if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_RESET) {
		  selected_mode = 2;
	  }
	  else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET) {
		  selected_mode = 3;
	  }


	  switch (selected_mode) {
		  case 1:
			  modo_1();
			  break;
		  case 2:
			  modo_2();
			  break;
		  case 3:
			  // modo_3();  // Você vai criar depois
			  modo_3();
			  break;
	  }
	  // Quando terminar o modo, volta pra tela de espera
  }

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL15;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED4_Pin|LED3_Pin|LED2_Pin|LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED4_Pin LED3_Pin LED2_Pin LED1_Pin */
  GPIO_InitStruct.Pin = LED4_Pin|LED3_Pin|LED2_Pin|LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : Botao2_Pin Botao1_Pin Botao4_Pin Botao3_Pin
                           Selec2_Pin Selec1_Pin */
  GPIO_InitStruct.Pin = Botao2_Pin|Botao1_Pin|Botao4_Pin|Botao3_Pin
                          |Selec2_Pin|Selec1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : Selec3_Pin */
  GPIO_InitStruct.Pin = Selec3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Selec3_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
