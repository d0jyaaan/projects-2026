/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
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
#include "ssd1306.h"
#include "ssd1306_tests.h"
#include "ssd1306_fonts.h"
#include "RC522.h"
#include "string.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18

// Row Mappings (PC8, PC6, PC5, PA12)
#define R1_Pin GPIO_PIN_8
#define R1_GPIO_Port GPIOC
#define R2_Pin GPIO_PIN_6
#define R2_GPIO_Port GPIOC
#define R3_Pin GPIO_PIN_5
#define R3_GPIO_Port GPIOC
#define R4_Pin GPIO_PIN_12
#define R4_GPIO_Port GPIOA

// Column Mappings (PA11, PB12, PB2, PB1)
#define C1_Pin GPIO_PIN_11
#define C1_GPIO_Port GPIOA
#define C2_Pin GPIO_PIN_12
#define C2_GPIO_Port GPIOB
#define C3_Pin GPIO_PIN_2
#define C3_GPIO_Port GPIOB
#define C4_Pin GPIO_PIN_1
#define C4_GPIO_Port GPIOB

// LED Mappings
#define Green_Pin GPIO_PIN_10
#define Green_GPIO_port GPIOC
#define Yellow_Pin GPIO_PIN_11
#define Yellow_GPIO_port GPIOC
#define Red_Pin GPIO_PIN_12
#define Red_GPIO_port GPIOC

#define Register_Size 100

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
//	Keymap for 4x4 membrane keypad
const char key_map[4][4] = { { '1', '2', '3', 'A' }, { '4', '5', '6', 'B' }, {
		'7', '8', '9', 'C' }, { '*', '0', '#', 'D' } };

typedef struct {
	char bytes[5]; // 4 characters + 1 for null terminator
} Password_t;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int _write(int file, char *ptr, int len) {
	for (int i = 0; i < len; i++) {
		ITM_SendChar((*ptr++));
	}

	return len;
}

void on_all_LEDs() {
	HAL_GPIO_WritePin(Green_GPIO_port, Green_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(Yellow_GPIO_port, Yellow_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(Red_GPIO_port, Red_Pin, GPIO_PIN_SET);
}

void off_all_LEDs() {
	HAL_GPIO_WritePin(Green_GPIO_port, Green_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(Yellow_GPIO_port, Yellow_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(Red_GPIO_port, Red_Pin, GPIO_PIN_RESET);
}

void buzzer_ring(uint32_t frequency) {
	if (frequency == 0) {

		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0); // Turn off

		__HAL_TIM_SET_COUNTER(&htim2, 0);
		return;
	} else {
//		Timer clock is 84MHz
		uint32_t arr_value = (84000000 / frequency) - 1;

		__HAL_TIM_SET_AUTORELOAD(&htim2, arr_value);
		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, arr_value / 2);

		HAL_TIM_GenerateEvent(&htim2, TIM_EVENTSOURCE_UPDATE);
	}
}

void buzzer_scan_success() {
//	Make a beep sound
	buzzer_ring(2000);
	HAL_Delay(100);
	buzzer_ring(0);
	HAL_Delay(50);
}

void buzzer_error_sequence() {
	buzzer_ring(1000);
	HAL_Delay(100);
	buzzer_ring(0);

	HAL_Delay(100);

	buzzer_ring(1000);
	HAL_Delay(100);
	buzzer_ring(0);
}

char keypad_scan(void) {
	for (int row = 0; row < 4; row++) {
		// 1. Set all rows HIGH first
		HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(R4_GPIO_Port, R4_Pin, GPIO_PIN_SET);

		// 2. Set the current row LOW
		if (row == 0)
			HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, GPIO_PIN_RESET);
		if (row == 1)
			HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, GPIO_PIN_RESET);
		if (row == 2)
			HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_RESET);
		if (row == 3)
			HAL_GPIO_WritePin(R4_GPIO_Port, R4_Pin, GPIO_PIN_RESET);

		// 3. Read Columns
		if (HAL_GPIO_ReadPin(C1_GPIO_Port, C1_Pin) == GPIO_PIN_RESET)
			return key_map[row][0];
		if (HAL_GPIO_ReadPin(C2_GPIO_Port, C2_Pin) == GPIO_PIN_RESET)
			return key_map[row][1];
		if (HAL_GPIO_ReadPin(C3_GPIO_Port, C3_Pin) == GPIO_PIN_RESET)
			return key_map[row][2];
		if (HAL_GPIO_ReadPin(C4_GPIO_Port, C4_Pin) == GPIO_PIN_RESET)
			return key_map[row][3];
	}
	return '\0'; // No key pressed
}

char get_key_debounced(void) {

	char key = keypad_scan();

	if (key != '\0') {
		HAL_Delay(15);
		if (keypad_scan() == key) {
			// Wait for key release to prevent multiple triggers
			while (keypad_scan() != '\0') {
			}
			return key;
		}
	}
	return '\0';
}

void success_sequence() {
	//	Blink green light
	off_all_LEDs();
	HAL_GPIO_WritePin(Green_GPIO_port, Green_Pin, GPIO_PIN_SET);

	//			clear screen and show success
	ssd1306_Fill(Black);

	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Success!", Font_11x18, White);

	ssd1306_UpdateScreen();

	//	triple beep to indicate success
	buzzer_scan_success();
	HAL_Delay(50);
	buzzer_scan_success();
	HAL_Delay(50);
	buzzer_scan_success();

	HAL_Delay(1000);

//	Clear screen
	ssd1306_Fill(Black);
	ssd1306_UpdateScreen();

	HAL_GPIO_WritePin(Green_GPIO_port, Green_Pin, GPIO_PIN_RESET);

}

void display_error(char *errorMsg) {

	off_all_LEDs();
	HAL_GPIO_WritePin(Red_GPIO_port, Red_Pin, GPIO_PIN_SET);

	ssd1306_Fill(Black);
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Error", Font_7x10, White);

	ssd1306_SetCursor(0, 20);
	ssd1306_WriteString(errorMsg, Font_7x10, White);

	ssd1306_UpdateScreen();
	buzzer_error_sequence();

	HAL_Delay(2000);
	ssd1306_Fill(Black);
	ssd1306_UpdateScreen();

	HAL_GPIO_WritePin(Red_GPIO_port, Red_Pin, GPIO_PIN_RESET);
}

Password_t get_input(uint8_t y_pos, uint8_t timeout) {

	uint8_t text_width = 8 * 11;

	uint8_t x_pos = (SCREEN_WIDTH - text_width) / 2;

//	User input
	uint8_t positionIndex = 0;
	Password_t password = { "____" };

	uint32_t last_execution_time = HAL_GetTick();
	uint32_t interval = 10000;

	while (1) {

//		0 second timeout, if timeout, return "____"
		if (timeout) {
			if (HAL_GetTick() - last_execution_time >= interval) {
				for (int i = 0; i < 4; i++) {
					password.bytes[i] = '_';
				}
				return password;
			}
		}

		char uInput = get_key_debounced();
//		If valid key press from user
		if (uInput != '\0') {

//			Don't timeout as long as there has been a key press
			last_execution_time = HAL_GetTick();

			if (uInput != 'A' && uInput != 'B' && uInput != 'C'
					&& uInput != 'D') {

//				delete char at current position
				if (uInput == '*') {
					if (positionIndex - 1 >= 0
							&& password.bytes[positionIndex - 1] != '_') {
						password.bytes[positionIndex] = '_';
						password.bytes[positionIndex - 1] = '_';
						positionIndex--;
					}
					buzzer_scan_success();
				}

//				confirm password
				else if (uInput == '#') {
					if (positionIndex == 4
							&& password.bytes[positionIndex] != '_') {
						buzzer_scan_success();
						return password;

//					Invalid confirm
					} else {
						buzzer_error_sequence();
					}
				}
//				assign character to password
				else {
					if (positionIndex < 4) {
						password.bytes[positionIndex] = uInput;
						positionIndex++;
					}
					buzzer_scan_success();
				}

			}
			printf("%c %c %c %c\n", password.bytes[0], password.bytes[1],
					password.bytes[2], password.bytes[3]);

//		make current position digit blink
		} else {

			if (password.bytes[positionIndex] == '_') {
				password.bytes[positionIndex] = ' ';
			} else if (password.bytes[positionIndex] == ' ') {
				password.bytes[positionIndex] = '_';
			}
		}

		char display_buffer[40];

//		clear space
		ssd1306_SetCursor(x_pos, y_pos);
		ssd1306_WriteString("       ", Font_11x18, White);

		snprintf(display_buffer, sizeof(display_buffer), "%c %c %c %c",
				password.bytes[0], password.bytes[1], password.bytes[2],
				password.bytes[3]);

		ssd1306_SetCursor(x_pos, y_pos);
		ssd1306_WriteString(display_buffer, Font_11x18, White);

		ssd1306_UpdateScreen();

		HAL_Delay(200);

	}
}

int Scan_RFID_Tag(uint8_t *str) {
	if (MFRC522_Request(PICC_REQIDL, str) == MI_OK) {
		if (MFRC522_Anticoll(str) == MI_OK) {
			// Success: Return the success status code (typically MI_OK)
			buzzer_scan_success();
			return MI_OK;
		}
		// Buffer of 2 seconds before can scan again if anticollision fails
		HAL_Delay(2000);
	}
	return MI_ERR; // Return error code if no tag is found
}

void startup_config(Password_t *masterPassword, Password_t *userPassword) {
	// config mode LED on
	on_all_LEDs();

	// master password logic
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Debug Mode", Font_7x10, White);

	ssd1306_SetCursor(0, 20);
	ssd1306_WriteString("Master Password:", Font_7x10, White);

	// Use the dereference operator (*) to update the original variable
	*masterPassword = get_input(40, 0);

	success_sequence();

	on_all_LEDs();
	ssd1306_Fill(Black);

	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Debug Mode", Font_7x10, White);

	ssd1306_SetCursor(0, 20);
	ssd1306_WriteString("User Password:", Font_7x10, White);

	// Use the dereference operator (*) to update the original variable
	*userPassword = get_input(40, 0);

	success_sequence();
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

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
	MX_USART2_UART_Init();
	MX_TIM2_Init();
	MX_I2C1_Init();
	MX_SPI2_Init();
	/* USER CODE BEGIN 2 */

	ssd1306_Init();
	MFRC522_Init();

//	Initialise PWM
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

	uint8_t RFIDBuffer[4];

	uint8_t registeredIDs[Register_Size][4] = { 0 };
	uint8_t activeCount = 0;

	uint8_t startupFlag = 1;
	uint8_t alarmFlag = 0;
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */

	Password_t masterPassword = { "" };
	Password_t userPassword = { "" };

	Password_t emptyPassword = { "____" };

	while (1) {

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

//		LED Light GPIO mapping
//		PC_10 -> Green LED
//		PC_11 -> Yellow LED
//		PC_12 -> Red LED
//		If first time boot system, prompt user to do set master password
//		if A, B, C or D is pressed. Run specific function tied to the letter
//		A - Register User
//		B - Delete User
//		C - Reset Master Password / user password
//		D - Reset system
		if (startupFlag == 1) {
			startup_config(&masterPassword, &userPassword);
			startupFlag = 0;
		}
//		Main display
		ssd1306_Fill(Black);

		ssd1306_SetCursor(0, 0);
		ssd1306_WriteString("Scanning....", Font_7x10, White);

		ssd1306_SetCursor(0, 12);
		ssd1306_WriteString("A - Register", Font_7x10, White);

		ssd1306_SetCursor(0, 24);
		ssd1306_WriteString("B - Delete", Font_7x10, White);

		ssd1306_SetCursor(0, 36);
		ssd1306_WriteString("C - Reset Password", Font_7x10, White);

		ssd1306_SetCursor(0, 48);
		ssd1306_WriteString("D - Reset System", Font_7x10, White);

		ssd1306_UpdateScreen();

		char uInput = get_key_debounced();

		if (uInput != '\0') {
			buzzer_scan_success();

			if (uInput == 'A') {

				Password_t inputPassword = { "" };
				on_all_LEDs();
//				Password verification
				ssd1306_Fill(Black);
				ssd1306_SetCursor(0, 0);
				ssd1306_WriteString("Register ID", Font_7x10, White);

				ssd1306_SetCursor(0, 20);
				ssd1306_WriteString("Master Password:", Font_7x10, White);

				inputPassword = get_input(40, 1);

//				No timeout
				if (memcmp(emptyPassword.bytes, inputPassword.bytes, 4) != 0) {

					if (memcmp(inputPassword.bytes, masterPassword.bytes, 4)
							== 0) {

						success_sequence();

						if (activeCount != Register_Size) {

							//						OLED Display logic
							ssd1306_Fill(Black);
							ssd1306_SetCursor(0, 0);
							ssd1306_WriteString("Register ID", Font_7x10,
									White);

							ssd1306_SetCursor(0, 20);
							ssd1306_WriteString("Scanning....", Font_7x10,
									White);

							ssd1306_SetCursor(0, 40);
							ssd1306_WriteString("Cancel (*)", Font_7x10, White);

							ssd1306_UpdateScreen();

							uint8_t cancelFlag = 0;
							while (Scan_RFID_Tag(RFIDBuffer) != MI_OK) {

								//							While scanning for RFID, user can choose to cancel
								if (get_key_debounced() == '*') {
									buzzer_scan_success();
									cancelFlag = 1;
									break;
								}
								HAL_Delay(200);
							}

							if (cancelFlag == 0) {
								uint8_t emptySlot[4] = { 0, 0, 0, 0 };
								uint8_t existFlag = 0;

								//						Track the firstEmpty slot
								int firstEmpty = -1;

								//						Check whether this RFID already exists in system
								for (int i = 0; i < Register_Size; i++) {

									//							Check whether element at index i is empty. Note down the very first instance of empty index
									if (firstEmpty < 0) {
										if (memcmp(registeredIDs[i], emptySlot,
												4) == 0) {
											firstEmpty = i;
											continue;
										}
									}

									//							If RFID buffer already exists, stop checking
									if (memcmp(registeredIDs[i], RFIDBuffer, 4)
											== 0) {
										existFlag = 1;
										break;
									}
								}

								//						Register ID
								if (existFlag == 0) {

									//	Prompt user for confirmation before register
									while (1) {

										ssd1306_Fill(Black);

										ssd1306_SetCursor(0, 0);
										ssd1306_WriteString("Are you sure?",
												Font_7x10, White);

										ssd1306_SetCursor(0, 20);
										ssd1306_WriteString("No (*)  Yes (#)",
												Font_7x10, White);
										ssd1306_UpdateScreen();

										char confirm = get_key_debounced();
										if (confirm != '\0') {
											buzzer_scan_success();
											// Cancel
											if (confirm == '*') {
												break;
												// Confirm
											} else if (confirm == '#') {
												memcpy(
														registeredIDs[firstEmpty],
														RFIDBuffer, 4);
												activeCount++;
												success_sequence();
												break;
											}
										}
									}

									//						ID already exists in system
								} else {
									display_error("ID already exists!");
								}
							}
						}
						//				Array for storing IDs is already full
						else {
							display_error("Memory is full!");
						}
					} else {
						display_error("Incorrect password");
					}
				} else {
					display_error("TIMEOUT");
				}

				off_all_LEDs();

			} else if (uInput == 'B') {

				Password_t inputPassword = { "" };
				on_all_LEDs();
//				Password verification
				ssd1306_Fill(Black);
				ssd1306_SetCursor(0, 0);
				ssd1306_WriteString("Delete ID", Font_7x10, White);

				ssd1306_SetCursor(0, 20);
				ssd1306_WriteString("Master Password:", Font_7x10, White);

				inputPassword = get_input(40, 1);

				//				No timeout
				if (memcmp(emptyPassword.bytes, inputPassword.bytes, 4) != 0) {
					if (memcmp(inputPassword.bytes, masterPassword.bytes, 4)
							== 0) {

						success_sequence();

//					OLED Display logic
						ssd1306_Fill(Black);
						ssd1306_SetCursor(0, 0);
						ssd1306_WriteString("Delete ID", Font_7x10, White);

						ssd1306_SetCursor(0, 20);
						ssd1306_WriteString("Scanning....", Font_7x10, White);

						ssd1306_SetCursor(0, 40);
						ssd1306_WriteString("Cancel (*)", Font_7x10, White);

						ssd1306_UpdateScreen();

						uint8_t cancelFlag = 0;
						while (Scan_RFID_Tag(RFIDBuffer) != MI_OK) {

//						While scanning for RFID, user can choose to cancel
							if (get_key_debounced() == '*') {
								buzzer_scan_success();
								cancelFlag = 1;
								break;
							}

							HAL_Delay(200);
						}

						if (!cancelFlag) {
							int indexToRemove = -1;

							for (int i = 0; i < Register_Size; i++) {
//					Check whether RFID exist in system. If exists, prompt user to double confirm
								if (memcmp(registeredIDs[i], RFIDBuffer, 4)
										== 0) {
									indexToRemove = i;
									break;
								}
							}

							if (indexToRemove != -1) {
								//	Prompt user for confirmation before deletion
								while (1) {

									ssd1306_Fill(Black);

									ssd1306_SetCursor(0, 0);
									ssd1306_WriteString("Are you sure?",
											Font_7x10, White);

									ssd1306_SetCursor(0, 20);
									ssd1306_WriteString("No (*)  Yes (#)",
											Font_7x10, White);
									ssd1306_UpdateScreen();

									char confirm = get_key_debounced();
									if (confirm != '\0') {
										buzzer_scan_success();
										// Cancel
										if (confirm == '*') {
											break;
											// Confirm
										} else if (confirm == '#') {
											memset(registeredIDs[indexToRemove],
													0, 4);
											activeCount--;
											success_sequence();
											break;
										}
									}
								}
							}

							else {
//							ID does not exist
								display_error("ID doesnt't exist!");
							}
						}

					} else {
						display_error("Incorrect password!");
					}
				} else {
					display_error("TIMEOUT");
				}
				off_all_LEDs();

			} else if (uInput == 'C') {

				Password_t inputPassword = { "" };
				on_all_LEDs();
				//				Password verification
				ssd1306_Fill(Black);
				ssd1306_SetCursor(0, 0);
				ssd1306_WriteString("Delete ID", Font_7x10, White);

				ssd1306_SetCursor(0, 20);
				ssd1306_WriteString("Master Password:", Font_7x10, White);

				inputPassword = get_input(40, 1);

				if (memcmp(emptyPassword.bytes, inputPassword.bytes, 4) != 0) {

					if (memcmp(inputPassword.bytes, masterPassword.bytes, 4)
							== 0) {
						success_sequence();

						//					OLED Display logic
						ssd1306_Fill(Black);
						ssd1306_SetCursor(0, 0);
						ssd1306_WriteString("Reset Password", Font_7x10, White);

						ssd1306_SetCursor(0, 10);
						ssd1306_WriteString("1-Master 2-User", Font_7x10,
								White);

						ssd1306_SetCursor(0, 20);
						ssd1306_WriteString("(*) Cancel ", Font_7x10, White);

						ssd1306_UpdateScreen();

						while (1) {
							char choice = get_key_debounced();

							if (choice != '\0') {
								buzzer_scan_success();

								if (choice == '*') {
									// Cancel
									break;

								} else if (choice == '1') {
									// Reset master password
									//								TODO, DONT ALLOW DIRECT CHANGE masterPassword / userPassword
									Password_t tempPassword = get_input(40, 1);

									if (memcmp(emptyPassword.bytes,
											tempPassword.bytes, 4) != 0) {
										masterPassword = tempPassword;
										success_sequence();
										break;
									} else {
										display_error("TIMEOUT");
									}

								} else if (choice == '2') {

									Password_t tempPassword = get_input(40, 1);

									if (memcmp(emptyPassword.bytes,
											tempPassword.bytes, 4) != 0) {
										userPassword = tempPassword;
										success_sequence();
										break;
									} else {
										display_error("TIMEOUT");
									}
								}
							}
						}

					} else {
						display_error("Incorrect password!");
					}
				} else {
					display_error("TIMEOUT");
				}
				off_all_LEDs();

			} else if (uInput == 'D') {

				Password_t inputPassword = { "" };
				on_all_LEDs();
				//				Password verification
				ssd1306_Fill(Black);
				ssd1306_SetCursor(0, 0);
				ssd1306_WriteString("Reset System", Font_7x10, White);

				ssd1306_SetCursor(0, 20);
				ssd1306_WriteString("Master Password:", Font_7x10, White);

				inputPassword = get_input(40, 1);

				if (memcmp(emptyPassword.bytes, inputPassword.bytes, 4) != 0) {

					if (memcmp(inputPassword.bytes, masterPassword.bytes, 4)
							== 0) {
						success_sequence();

						while (1) {
							ssd1306_Fill(Black);

							ssd1306_SetCursor(0, 0);
							ssd1306_WriteString("Are you sure?", Font_7x10,
									White);

							ssd1306_SetCursor(0, 20);
							ssd1306_WriteString("No (*)  Yes (#)", Font_7x10,
									White);
							ssd1306_UpdateScreen();

							char confirm = get_key_debounced();
							if (confirm != '\0') {
								buzzer_scan_success();
								// Cancel
								if (confirm == '*') {
									break;
									// Confirm
								} else if (confirm == '#') {

									on_all_LEDs();

//									Reset entire system
									ssd1306_Fill(Black);
									ssd1306_SetCursor(0, 0);
									ssd1306_WriteString("Resetting", Font_11x18,
											White);

									ssd1306_UpdateScreen();

									memset(registeredIDs, 0,
											sizeof(registeredIDs));
									startupFlag = 1;

									masterPassword = (Password_t ) { "" };
									userPassword = (Password_t ) { "" };

									HAL_Delay(3000);

									off_all_LEDs();
									success_sequence();

									break;
								}
							}
						}
					} else {
						display_error("Incorrect password");
					}
				} else {
					display_error("TIMEOUT");
				}

			}
		}

//		If no key press, wait for RFID tag. Check whether access should be granted or not
//		Yellow -> Standby for scanning
		HAL_GPIO_WritePin(Yellow_GPIO_port, Yellow_Pin, GPIO_PIN_SET);

//		Every time card is scanned, turn off yellow LED and make buzzer beep
		if (Scan_RFID_Tag(RFIDBuffer) == MI_OK) {

			for (int i = 0; i < Register_Size; i++) {
//				RFID is registered. success
				if (memcmp(registeredIDs[i], RFIDBuffer, 4) == 0) {

					//			Turn off yellow light
					HAL_GPIO_WritePin(Yellow_GPIO_port, Yellow_Pin,
							GPIO_PIN_RESET);

					on_all_LEDs();
					//				Password verification
					uint8_t wrongPasswordCount = 0;

					while (1) {

						ssd1306_Fill(Black);
						ssd1306_SetCursor(0, 0);
						ssd1306_WriteString("Enter Password", Font_7x10, White);

//						Show remaining attempts
						char attempts[20];
						snprintf(attempts, sizeof(attempts), "%i attempts left",
								3 - wrongPasswordCount);
						ssd1306_SetCursor(0, 20);
						ssd1306_WriteString(attempts, Font_7x10, White);

						ssd1306_UpdateScreen();
//						Timeout system

//						Get password
						Password_t inputPassword = { "" };
						inputPassword = get_input(40, 1);

//					No timeout
						if (memcmp(emptyPassword.bytes, inputPassword.bytes, 4)
								!= 0) {
							//						Correct password
							if (memcmp(inputPassword.bytes, userPassword.bytes,
									4) == 0) {
								success_sequence();
								break;
							}
							//						incorrect password (allow 3 times)
							else {
								wrongPasswordCount++;
								if (wrongPasswordCount == 3) {
									buzzer_error_sequence();
									alarmFlag = 1;
									break;
								}
							}
						} else {
							display_error("TIMEOUT");
							break;
						}
					}
					break;
				}
//				If not registered, error sequence
				if (i == Register_Size - 1) {
//					Turn off yellow light
					display_error("ID not registered");
				}
			}
		}

//		Sound alarm if user password inputted incorrectly 3 times
		while (alarmFlag == 1) {

			ssd1306_Fill(Black);
			ssd1306_SetCursor(0, 0);
			ssd1306_WriteString("Disarm alarm", Font_7x10, White);

			ssd1306_SetCursor(0, 20);
			ssd1306_WriteString("Press any key", Font_7x10, White);

			ssd1306_UpdateScreen();

			off_all_LEDs();
			HAL_GPIO_WritePin(Red_GPIO_port, Red_Pin, GPIO_PIN_SET);

			buzzer_ring(500);
			HAL_Delay(50);
			buzzer_ring(0);

			HAL_Delay(100);

			buzzer_ring(500);
			HAL_Delay(50);
			buzzer_ring(0);

			HAL_GPIO_WritePin(Red_GPIO_port, Red_Pin, GPIO_PIN_RESET);

			if (get_key_debounced() != '\0') {

				buzzer_scan_success();

				ssd1306_Fill(Black);
				ssd1306_SetCursor(0, 0);
				ssd1306_WriteString("Disarm alarm", Font_7x10, White);

				ssd1306_SetCursor(0, 20);
				ssd1306_WriteString("Password:", Font_7x10, White);

				//			Get password to disarm alarm
				Password_t inputPassword = { "" };
				inputPassword = get_input(40, 1);

				if (memcmp(emptyPassword.bytes, inputPassword.bytes, 4) != 0) {
					if (memcmp(inputPassword.bytes, masterPassword.bytes, 4)
							== 0) {
						success_sequence();
						alarmFlag = 0;
					}
				}
			}
		}
//		RFID buffer checking logic

//		Reset RFID Buffer
		memset(RFIDBuffer, 0, sizeof(RFIDBuffer));
	}
}
/* USER CODE END 3 */

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 16;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 2;
	RCC_OscInitStruct.PLL.PLLR = 2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 400000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief SPI2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI2_Init(void) {

	/* USER CODE BEGIN SPI2_Init 0 */

	/* USER CODE END SPI2_Init 0 */

	/* USER CODE BEGIN SPI2_Init 1 */

	/* USER CODE END SPI2_Init 1 */
	/* SPI2 parameter configuration*/
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN SPI2_Init 2 */

	/* USER CODE END SPI2_Init 2 */

}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void) {

	/* USER CODE BEGIN TIM2_Init 0 */

	/* USER CODE END TIM2_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };

	/* USER CODE BEGIN TIM2_Init 1 */

	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 0;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 1024;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 25;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */
	HAL_TIM_MspPostInit(&htim2);

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */

	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, LD2_Pin | GPIO_PIN_8 | R4_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC,
	R3_Pin | R2_Pin | R1_Pin | Green_LED_Pin | Yellow_LED_Pin | Red_LED_Pin,
			GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);

	/*Configure GPIO pins : LD2_Pin PA8 */
	GPIO_InitStruct.Pin = LD2_Pin | GPIO_PIN_8;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : R3_Pin R2_Pin R1_Pin Green_LED_Pin
	 Yellow_LED_Pin Red_LED_Pin */
	GPIO_InitStruct.Pin = R3_Pin | R2_Pin | R1_Pin | Green_LED_Pin
			| Yellow_LED_Pin | Red_LED_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : C4_Pin C3_Pin C2_Pin */
	GPIO_InitStruct.Pin = C4_Pin | C3_Pin | C2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : C1_Pin */
	GPIO_InitStruct.Pin = C1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(C1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : R4_Pin */
	GPIO_InitStruct.Pin = R4_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(R4_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : PB9 */
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
