#include "Clock_INRQ.h"
#include "Delay.h"
#include "stm32f1xx.h"
#include "Clock.h"

#define BUFFER_SIZE 10 

uint8_t uartRxBuffer[BUFFER_SIZE];
uint8_t uartTxBuffer[BUFFER_SIZE];

extern "C" void DMA1_Channel5_IRQHandler(void) {
  // DMA receive complete interrupt handler
  if (DMA1->ISR & DMA_ISR_TCIF5) {
    // Data has been received
    // Process the received data (e.g., echo it back)
    for (int i = 0; i < BUFFER_SIZE; i++) {
      uartTxBuffer[i] = uartRxBuffer[i]; // Echo received data
    }

    // Clear the DMA receive interrupt flag
    DMA1->IFCR = DMA_IFCR_CTCIF5;
  }
}

void USART1_Transmit(uint8_t *data, uint16_t length) {
  for (uint16_t i = 0; i < length; i++) {
    // Wait for the UART data register to be empty
    while (!(USART1->SR & USART_SR_TXE))
      ;

    // Send the character
    USART1->DR = data[i];
    // Wait for the last character to be transmitted
    while (!(USART1->SR & USART_SR_TC))
      ;
  }
}

Clock_INRQ clock_header;

int main(void) {
  // Enable the system clock for USART1, DMA1, and GPIOA
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  RCC->AHBENR |= RCC_AHBENR_DMA1EN;
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

  clock_header.clock_control_INRQ.enable_HSE = 1;
  clock_header.clock_control_INRQ.enable_HSI = 1;
  clock_header.clock_control_INRQ.enable_PLL = 1;
  clock_header.clock_control_INRQ.enbale_CSS = 0;
  clock_header.clock_control_INRQ.enalbe_HSEBYP = 0;

  clock_header.clock_configuration_INRQ.clock_source =
      Clock_system_source_selector::PLL;
  clock_header.clock_configuration_INRQ.PLL_multiplier = 0b0111; // 9
  clock_header.clock_configuration_INRQ.PLL_enable_HSE = 1;
  clock_header.clock_configuration_INRQ.PLL_prescaler = 2;
  clock_header.clock_configuration_INRQ.APB1_prescaler = 0;
  clock_header.clock_configuration_INRQ.APB2_prescaler = 0;
  clock_header.clock_configuration_INRQ.AHB_prescaler = 0b1000;

  Clock().__init__(clock_header);
  

  // Configure USART1
  // GPIOA->CRH = (GPIOA->CRH & ~GPIO_CRH_CNF9) | GPIO_CRH_CNF9_1; // USART1 TX as Push-Pull
  //
  GPIOA->CRH &= ~(GPIO_CRH_MODE9_Msk);
  GPIOA->CRH |= GPIO_CRH_MODE9;
  GPIOA->CRH &= ~(GPIO_CRH_CNF9_Msk);
  GPIOA->CRH |= GPIO_CRH_CNF9_1;

  GPIOA->CRH &= ~(GPIO_CRH_CNF10_0);
  GPIOA->CRH |= GPIO_CRH_CNF10_1;
  GPIOA->CRH &= ~(GPIO_CRH_MODE10);
  GPIOA->ODR |= GPIO_ODR_ODR10;

  USART1->BRR = 3750;
  USART1->CR1 =
      USART_CR1_UE | USART_CR1_TE | USART_CR1_RE; // Enable UART, TX, and RX
                                                  //
  // USART1->CR3 |= USART_CR3_DMAT;
  USART1->CR3 |= USART_CR3_DMAR;

  // // Configure DMA1 Channel 5 for USART1 RX
  DMA1_Channel5->CPAR = (uint32_t)&USART1->DR;  // Source: UART1 DR
  DMA1_Channel5->CMAR = (uint32_t)uartRxBuffer; // Destination: Buffer
  DMA1_Channel5->CNDTR = BUFFER_SIZE; // Number of data items to transfer
  DMA1_Channel5->CCR =
      DMA_CCR_MINC | DMA_CCR_TCIE |
      DMA_CCR_EN; // Memory increment, Transfer complete interrupt, Enable DMA

  DMA1_Channel5->CCR |= DMA_CCR_CIRC;

  // Enable the DMA interrupt for USART1 RX
  NVIC_EnableIRQ(DMA1_Channel5_IRQn);

  SystemCoreClockUpdate();
  __IO uint32_t clock_value = SystemCoreClock;

  Delay().__init__(8);

  uint8_t msg[] = "fuck";
  while (1) {
    USART1_Transmit(uartRxBuffer, sizeof(uartRxBuffer));
    Delay().wait(6000);
  }
}
