#include "debug.h"

// --- Pin Definitions ---
#define IR_LID_Pin       GPIO_Pin_3   // PD3 → lid ir
#define IR_STATUS_Pin    GPIO_Pin_2   // PD2 → bin ir

#define MOTOR_IN1_Pin    GPIO_Pin_4   // PD4 → L298N IN1
#define MOTOR_IN2_Pin    GPIO_Pin_5   // PD5 → L298N IN2
#define MOTOR_EN_Pin     GPIO_Pin_1   // PD1 → L298N ENA (set HIGH permanently)

#define LED_LID_Pin      GPIO_Pin_6   // PD6 → 1stled
#define LED_STATUS_Pin   GPIO_Pin_0   // PD0 → 2ndled
#define LED_STATUS2_Pin  GPIO_Pin_0   // PC0 → 3rd led

// --- Function Prototypes ---
void GPIO_Init_All(void);
void Motor_Open(void);
void Motor_Close(void);
void Motor_Stop(void);

// --- GPIO Initialization ---
void GPIO_Init_All(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC, ENABLE);

    // Outputs: Motor pins + LEDs on GPIOD
    GPIO_InitStructure.GPIO_Pin = MOTOR_IN1_Pin | MOTOR_IN2_Pin | MOTOR_EN_Pin | LED_LID_Pin | LED_STATUS_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // Outputs: LED_STATUS2 on GPIOC
    GPIO_InitStructure.GPIO_Pin = LED_STATUS2_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // Set MOTOR_EN HIGH permanently
    GPIO_SetBits(GPIOD, MOTOR_EN_Pin);

    // Inputs: IR sensors
    GPIO_InitStructure.GPIO_Pin = IR_LID_Pin | IR_STATUS_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; // Pull-down
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // Initially, turn off all LEDs
    GPIO_ResetBits(GPIOD, LED_LID_Pin | LED_STATUS_Pin);
    GPIO_ResetBits(GPIOC, LED_STATUS2_Pin);
}

// --- Motor Control ---
void Motor_Open(void)
{
    GPIO_SetBits(GPIOD, MOTOR_IN1_Pin);
    GPIO_ResetBits(GPIOD, MOTOR_IN2_Pin);
}

void Motor_Close(void)
{
    GPIO_ResetBits(GPIOD, MOTOR_IN1_Pin);
    GPIO_SetBits(GPIOD, MOTOR_IN2_Pin);
}

void Motor_Stop(void)
{
    GPIO_ResetBits(GPIOD, MOTOR_IN1_Pin | MOTOR_IN2_Pin);
}

// --- Main Program ---
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    GPIO_Init_All();

    uint8_t ir_lid_state = 0;
    uint8_t ir_status_state = 0;
    uint8_t lid_open = 0;

    while(1)
    {
        // --- Read IR sensors ---
        ir_lid_state = GPIO_ReadInputDataBit(GPIOD, IR_LID_Pin);
        ir_status_state = GPIO_ReadInputDataBit(GPIOD, IR_STATUS_Pin);

        // --- First IR (lid) detection ---
        if(ir_lid_state == SET && lid_open == 0)
        {
            // Turn on first LED
            GPIO_SetBits(GPIOD, LED_LID_Pin);

            // Open lid
            Motor_Open();
            Delay_Ms(150);  // adjust for full open
            Motor_Stop();

            // Keep LED ON for a while
            Delay_Ms(500);
            GPIO_ResetBits(GPIOD, LED_LID_Pin);

            // Close lid after a delay
            Delay_Ms(500);
            Motor_Close();
            Delay_Ms(150);
            Motor_Stop();

            lid_open = 1;
        }
        else if(ir_lid_state == RESET)
        {
            lid_open = 0;
        }

        // --- Second IR (status inside bin) ---
        if(ir_status_state == RESET)
        {
            // Sequence: LED 1 → LED 2 → LED 3
            GPIO_SetBits(GPIOD, LED_LID_Pin);    // First LED
            Delay_Ms(1000);

            GPIO_SetBits(GPIOD, LED_STATUS_Pin); // Second LED
            Delay_Ms(1000);

            GPIO_SetBits(GPIOC, LED_STATUS2_Pin); // Third LED
            Delay_Ms(1000);

            // Turn off all LEDs
            GPIO_ResetBits(GPIOD, LED_LID_Pin | LED_STATUS_Pin);
            GPIO_ResetBits(GPIOC, LED_STATUS2_Pin);
        }

        Delay_Ms(100);
    }
}
