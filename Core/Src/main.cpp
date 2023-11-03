#include "Clock.h"
#include "LED.h"
#include "UART.h"
#include "stm32f1xx.h"

UART_INRQ uart_header;
Clock_INRQ clock_header;

uint8_t dma_uart_buffer[10] = {0};

extern "C" void USART1_IRQHandler(void) {
  // __disable_irq();

  // if (USART1->SR & USART_SR_RXNE) {
  // uint8_t rx = USART1->DR;
  // UART().transmit(&rx, sizeof(rx));
  // }

  // __enable_irq();
}

extern "C" void DMA1_Channel5_IRQHandler(void) {
  // __disable_irq();

  LED().led_on();
  UART().transmit(dma_uart_buffer, sizeof(dma_uart_buffer));

  // clearing interrupt flag
  DMA1->IFCR |= DMA_IFCR_CTCIF5;
  DMA1->IFCR |= DMA_IFCR_CHTIF5;
  DMA1->IFCR |= DMA_IFCR_CTCIF5;
  DMA1->IFCR |= DMA_IFCR_CGIF5;

  // __enable_irq();
}

typedef struct {
  uint8_t priority = 0;
  uint8_t mem_size = 0;
  uint8_t peripheral_size = 0;
  uint8_t enable_mem2mem = 0;
  uint8_t enable_mem_increment = 0;
  uint8_t enable_peripheral_increment = 0;
  uint8_t enable_circular_mode = 0;
  uint8_t data_transfer_direction = 0;
  uint8_t enable_error_interrupt = 0;
  uint8_t enable_half_transfer_interrupt = 0;
  uint8_t enable_transfer_cmpl_interrupt = 0;
  uint8_t enable_channel = 0;

  uint8_t data_number_transfer = 0;

  uint32_t memory_address = 0;
  uint32_t peripheral_address = 0;
} DMA_INRQ;

DMA_INRQ uart_dma_header;

class DMA {
private:
  void configure_interrupts(DMA_INRQ header) {
    // enbling interrupt
    //

    DMA1_Channel5->CCR |= DMA_CCR_TCIE;
    DMA1_Channel5->CCR |= DMA_CCR_HTIE;
    DMA1_Channel5->CCR |= DMA_CCR_TEIE;

    DMA1->IFCR |= DMA_IFCR_CTEIF5;
    DMA1->IFCR |= DMA_IFCR_CHTIF5;
    DMA1->IFCR |= DMA_IFCR_CTCIF5;
    DMA1->IFCR |= DMA_IFCR_CGIF5;

    NVIC_SetPriority(DMA1_Channel5_IRQn, 0);
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
  }

  void configure_priority(DMA_INRQ header) {
    // priority
    DMA1_Channel5->CCR &= ~(DMA_CCR_PL_Msk);
    DMA1_Channel5->CCR |= header.priority << DMA_CCR_PL_Pos;
  }

public:
  void __init__(DMA_INRQ header) {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    DMA1_Channel5->CCR &= ~(DMA_CCR_MEM2MEM_Msk);
    DMA1_Channel5->CCR |= header.enable_mem2mem << DMA_CCR_MEM2MEM_Pos;

    DMA1_Channel5->CCR &= (DMA_CCR_PSIZE_Msk);
    DMA1_Channel5->CCR |= header.peripheral_size << DMA_CCR_PSIZE_Pos;

    DMA1_Channel5->CCR &= (DMA_CCR_MSIZE_Msk);
    DMA1_Channel5->CCR |= header.mem_size << DMA_CCR_MSIZE_Pos;

    DMA1_Channel5->CCR &= (DMA_CCR_MINC_Msk);
    DMA1_Channel5->CCR |= header.enable_mem_increment << DMA_CCR_MINC_Pos;

    DMA1_Channel5->CCR &= (DMA_CCR_PINC_Msk);
    DMA1_Channel5->CCR |= header.enable_peripheral_increment
                          << DMA_CCR_PINC_Pos;

    DMA1_Channel5->CCR &= (DMA_CCR_CIRC_Msk);
    DMA1_Channel5->CCR |= header.enable_circular_mode << DMA_CCR_CIRC_Pos;

    DMA1_Channel5->CCR &= (DMA_CCR_DIR_Msk);
    DMA1_Channel5->CCR |= header.data_transfer_direction << DMA_CCR_DIR_Pos;

    DMA1_Channel5->CNDTR = header.data_number_transfer;

    DMA1_Channel5->CMAR = (uint32_t)dma_uart_buffer;

    DMA1_Channel5->CPAR = (uint32_t)(&USART1->DR);

    this->configure_interrupts(header);
    this->configure_priority(header);

    DMA1_Channel5->CCR &= (DMA_CCR_EN_Msk);
    DMA1_Channel5->CCR |= header.enable_channel << DMA_CCR_EN_Pos;
  };

  void start_dma() {
    DMA1_Channel5->CCR &= (DMA_CCR_EN_Msk);
    DMA1_Channel5->CCR |= 1 << DMA_CCR_EN_Pos;
    // DMA1_Channel5->CCR |= DMA_CCR;
  };
};

int main() {
  uart_dma_header.enable_mem2mem = 0;
  uart_dma_header.priority = 0;
  uart_dma_header.enable_circular_mode = 1;
  uart_dma_header.enable_mem_increment = 1;
  uart_dma_header.enable_peripheral_increment = 0;

  uart_dma_header.enable_transfer_cmpl_interrupt = 1;
  uart_dma_header.peripheral_size = 0;
  uart_dma_header.mem_size = 0;
  uart_dma_header.enable_channel = 0;
  uart_dma_header.data_transfer_direction = 0;

  // adresses
  uart_dma_header.memory_address = 0;
  // uart_dma_header.peripheral_address = USART1->DR;
  // uart_dma_header.memory_address = (uint32_t)dma_uart_buffer;
  uart_dma_header.data_number_transfer = 10;

  uart_header.baudrate = 3750;
  uart_header.enable_transmitter = 1;
  uart_header.enable_reciever = 1;
  uart_header.enable_word_9bit = 0;
  uart_header.enable_parity = 0;

  uart_header.enable_reciever = 1;
  uart_header.enable_transmitter = 1;

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

  SystemCoreClockUpdate();
  __IO uint32_t clock_value = SystemCoreClock;

  Delay().__init__(36);

  UART().__init__(uart_header);
  UART().enable_dma_rx();

  DMA().__init__(uart_dma_header);
  DMA().start_dma();

  while (1) {
    // UART().transmit(dma_uart_buffer, sizeof(dma_uart_buffer));
    Delay().wait(1000);
  }

  return 0;
}
