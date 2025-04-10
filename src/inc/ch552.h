/*--------------------------------------------------------------------------
CH552.H
Header file for CH552 microcontrollers.
****************************************
**  Copyright  (C)  W.ch  1999-2016   **
**  Web:              http://wch.cn   **
****************************************
--------------------------------------------------------------------------*/

// Blocks: __CH552_H__, __USB_DEF__

#ifndef __CH552_H__
#define __CH552_H__

#include <compiler.h>
#include <stdint.h>

/*----- SFR --------------------------------------------------------------*/
/*  sbit are bit addressable, others are byte addressable */

/*  System Registers  */
SFR(PSW, 0xD0);          // program status word
SBIT(CY, 0xD0, 7);       // carry flag
SBIT(AC, 0xD0, 6);       // auxiliary carry flag
SBIT(F0, 0xD0, 5);       // bit addressable general purpose flag 0
SBIT(RS1, 0xD0, 4);      // register R0-R7 bank selection high bit
SBIT(RS0, 0xD0, 3);      // register R0-R7 bank selection low bit
#define MASK_PSW_RS 0x18 // bit mask of register R0-R7 bank selection
                         // RS1 & RS0: register R0-R7 bank selection
                         //    00 - bank 0, R0-R7 @ address 0x00-0x07
                         //    01 - bank 1, R0-R7 @ address 0x08-0x0F
                         //    10 - bank 2, R0-R7 @ address 0x10-0x17
                         //    11 - bank 3, R0-R7 @ address 0x18-0x1F
SBIT(OV, 0xD0, 2);       // overflow flag
SBIT(F1, 0xD0, 1);       // bit addressable general purpose flag 1
SBIT(P, 0xD0, 0);        // ReadOnly: parity flag
SFR(ACC, 0xE0);          // accumulator
SFR(B, 0xF0);            // general purpose register B
SFR(SP, 0x81);           // stack pointer
SFR16(DPTR, 0x82);       // DPTR pointer, little-endian
SFR(DPL, 0x82);          // data pointer low
SFR(DPH, 0x83);          // data pointer high
SFR(SAFE_MOD, 0xA1);     // WriteOnly: writing safe mode
SFR(CHIP_ID, 0xA1);      // ReadOnly: reading chip ID
#define CHIP_ID SAFE_MOD
SFR(GLOBAL_CFG, 0xB1);   // global config, Write@SafeMode
#define bBOOT_LOAD 0x20  // ReadOnly: boot loader status for discriminating BootLoader or Application: set 1 by power on reset, clear 0 by software reset
#define bSW_RESET 0x10   // software reset bit, auto clear by hardware
#define bCODE_WE 0x08    // enable flash-ROM (include code & Data-Flash) being program or erasing: 0=writing protect, 1=enable program and erase
#define bDATA_WE 0x04    // enable Data-Flash (flash-ROM data area) being program or erasing: 0=writing protect, 1=enable program and erase
#define bLDO3V3_OFF 0x02 // disable 5V->3.3V LDO: 0=enable LDO for USB and internal oscillator under 5V power, 1=disable LDO, V33 pin input external 3.3V power
#define bWDOG_EN 0x01    // enable watch-dog reset if watch-dog timer overflow: 0=as timer only, 1=enable reset if timer overflow

/* Clock and Sleep and Power Registers */
SFR(PCON, 0x87);           // power control and reset flag
#define SMOD 0x80          // baud rate selection for UART0 mode 1/2/3: 0=slow(Fsys/128 @mode2, TF1/32 @mode1/3, no effect for TF2),
                           //   1=fast(Fsys/32 @mode2, TF1/16 @mode1/3, no effect for TF2)
#define bRST_FLAG1 0x20    // ReadOnly: recent reset flag high bit
#define bRST_FLAG0 0x10    // ReadOnly: recent reset flag low bit
#define MASK_RST_FLAG 0x30 // ReadOnly: bit mask of recent reset flag
#define RST_FLAG_SW 0x00
#define RST_FLAG_POR 0x10
#define RST_FLAG_WDOG 0x20
#define RST_FLAG_PIN 0x30
// bPC_RST_FLAG1 & bPC_RST_FLAG0: recent reset flag
//    00 - software reset, by bSW_RESET=1 @(bBOOT_LOAD=0 or bWDOG_EN=1)
//    01 - power on reset
//    10 - watch-dog timer overflow reset
//    11 - external input manual reset by RST pin
#define GF1 0x08             // general purpose flag bit 1
#define GF0 0x04             // general purpose flag bit 0
#define PD 0x02              // power-down enable bit, auto clear by wake-up hardware
SFR(CLOCK_CFG, 0xB9);        // system clock config: lower 3 bits for system clock Fsys, Write@SafeMode
#define bOSC_EN_INT 0x80     // internal oscillator enable and original clock selection: 1=enable & select internal clock, 0=disable & select external clock
#define bOSC_EN_XT 0x40      // external oscillator enable, need quartz crystal or ceramic resonator between XI and XO pins
#define bWDOG_IF_TO 0x20     // ReadOnly: watch-dog timer overflow interrupt flag, cleared by reload watch-dog count or auto cleared when MCU enter interrupt routine
#define bROM_CLK_FAST 0x10   // flash-ROM clock frequency selection: 0=normal(for Fosc>=16MHz), 1=fast(for Fosc<16MHz)
#define bRST 0x08            // ReadOnly: pin RST input
#define bT2EX_ 0x08          // alternate pin for T2EX
#define bCAP2_ 0x08          // alternate pin for CAP2
#define MASK_SYS_CK_SEL 0x07 // bit mask of system clock Fsys selection
/*
   Fxt = 24MHz(8MHz~25MHz for non-USB application), from external oscillator @XI&XO
   Fosc = bOSC_EN_INT ? 24MHz : Fxt
   Fpll = Fosc * 4 => 96MHz (32MHz~100MHz for non-USB application)
   Fusb4x = Fpll / 2 => 48MHz (Fixed)
              MASK_SYS_CK_SEL[2] [1] [0]
   Fsys = Fpll/3   =  32MHz:  1   1   1
   Fsys = Fpll/4   =  24MHz:  1   1   0
   Fsys = Fpll/6   =  16MHz:  1   0   1
   Fsys = Fpll/8   =  12MHz:  1   0   0
   Fsys = Fpll/16  =   6MHz:  0   1   1
   Fsys = Fpll/32  =   3MHz:  0   1   0
   Fsys = Fpll/128 = 750KHz:  0   0   1
   Fsys = Fpll/512 =187.5KHz: 0   0   0
*/
SFR(WAKE_CTRL, 0xA9);      // wake-up control, Write@SafeMode
#define bWAK_BY_USB 0x80   // enable wake-up by USB event
#define bWAK_RXD1_LO 0x40  // enable wake-up by RXD1 low level
#define bWAK_P1_5_LO 0x20  // enable wake-up by pin P1.5 low level
#define bWAK_P1_4_LO 0x10  // enable wake-up by pin P1.4 low level
#define bWAK_P1_3_LO 0x08  // enable wake-up by pin P1.3 low level
#define bWAK_RST_HI 0x04   // enable wake-up by pin RST high level
#define bWAK_P3_2E_3L 0x02 // enable wake-up by pin P3.2 (INT0) edge or pin P3.3 (INT1) low level
#define bWAK_RXD0_LO 0x01  // enable wake-up by RXD0 low level
SFR(RESET_KEEP, 0xFE);     // value keeper during reset
SFR(WDOG_COUNT, 0xFF);     // watch-dog count, count by clock frequency Fsys/65536

/*  Interrupt Registers  */
#define IE_ADDR (0xA8)         // interrupt enable address
SFR(IE, IE_ADDR);              // interrupt enable
SBIT(EA, IE_ADDR, 7);          // enable global interrupts: 0=disable, 1=enable if E_DIS=0
SBIT(E_DIS, IE_ADDR, 6);       // disable global interrupts, intend to inhibit interrupt during some flash-ROM operation: 0=enable if EA=1, 1=disable
SBIT(ET2, IE_ADDR, 5);         // enable timer2 interrupt
SBIT(ES, IE_ADDR, 4);          // enable UART0 interrupt
SBIT(ET1, IE_ADDR, 3);         // enable timer1 interrupt
SBIT(EX1, IE_ADDR, 2);         // enable external interrupt INT1
SBIT(ET0, IE_ADDR, 1);         // enable timer0 interrupt
SBIT(EX0, IE_ADDR, 0);         // enable external interrupt INT0
#define IP_ADDR (0xB8)         // interrupt priority and current priority address
SFR(IP, IP_ADDR);              // interrupt priority and current priority
SBIT(PH_FLAG, IP_ADDR, 7);     // ReadOnly: high level priority action flag
SBIT(PL_FLAG, IP_ADDR, 6);     // ReadOnly: low level priority action flag
                               // PH_FLAG & PL_FLAG: current interrupt priority
                               //    00 - no interrupt now
                               //    01 - low level priority interrupt action now
                               //    10 - high level priority interrupt action now
                               //    11 - unknown error
SBIT(PT2, IP_ADDR, 5);         // timer2 interrupt priority level
SBIT(PS, IP_ADDR, 4);          // UART0 interrupt priority level
SBIT(PT1, IP_ADDR, 3);         // timer1 interrupt priority level
SBIT(PX1, IP_ADDR, 2);         // external interrupt INT1 priority level
SBIT(PT0, IP_ADDR, 1);         // timer0 interrupt priority level
SBIT(PX0, IP_ADDR, 0);         // external interrupt INT0 priority level
#define IE_EX_ADDR (0xE8)      // extend interrupt enable
SFR(IE_EX, IE_EX_ADDR);        // extend interrupt enable
SBIT(IE_WDOG, IE_EX_ADDR, 7);  // enable watch-dog timer interrupt
SBIT(IE_GPIO, IE_EX_ADDR, 6);  // enable GPIO input interrupt
SBIT(IE_PWMX, IE_EX_ADDR, 5);  // enable PWM1/2 interrupt
SBIT(IE_UART1, IE_EX_ADDR, 4); // enable UART1 interrupt
SBIT(IE_ADC, IE_EX_ADDR, 3);   // enable ADC interrupt
SBIT(IE_USB, IE_EX_ADDR, 2);   // enable USB interrupt
SBIT(IE_TKEY, IE_EX_ADDR, 1);  // enable touch-key timer interrupt
SBIT(IE_SPI0, IE_EX_ADDR, 0);  // enable SPI0 interrupt
SFR(IP_EX, 0xE9);              // extend interrupt priority
#define bIP_LEVEL 0x80         // ReadOnly: current interrupt nested level: 0=no interrupt or two levels, 1=one level
#define bIP_GPIO 0x40          // GPIO input interrupt priority level
#define bIP_PWMX 0x20          // PWM1/2 interrupt priority level
#define bIP_UART1 0x10         // UART1 interrupt priority level
#define bIP_ADC 0x08           // ADC interrupt priority level
#define bIP_USB 0x04           // USB interrupt priority level
#define bIP_TKEY 0x02          // touch-key timer interrupt priority level
#define bIP_SPI0 0x01          // SPI0 interrupt priority level
SFR(GPIO_IE, 0xC7);            // GPIO interrupt enable
#define bIE_IO_EDGE 0x80       // enable GPIO edge interrupt: 0=low/high level, 1=falling/rising edge
#define bIE_RXD1_LO 0x40       // enable interrupt by RXD1 low level / falling edge
#define bIE_P1_5_LO 0x20       // enable interrupt by pin P1.5 low level / falling edge
#define bIE_P1_4_LO 0x10       // enable interrupt by pin P1.4 low level / falling edge
#define bIE_P1_3_LO 0x08       // enable interrupt by pin P1.3 low level / falling edge
#define bIE_RST_HI 0x04        // enable interrupt by pin RST high level / rising edge
#define bIE_P3_1_LO 0x02       // enable interrupt by pin P3.1 low level / falling edge
#define bIE_RXD0_LO 0x01       // enable interrupt by RXD0 low level / falling edge

/*  FlashROM and Data-Flash Registers  */
SFR16(ROM_ADDR, 0x84);     // address for flash-ROM, little-endian
SFR(ROM_ADDR_L, 0x84);     // address low byte for flash-ROM
SFR(ROM_ADDR_H, 0x85);     // address high byte for flash-ROM
SFR16(ROM_DATA, 0x8E);     // data for flash-ROM writing, little-endian
SFR(ROM_DATA_L, 0x8E);     // data low byte for flash-ROM writing, data byte for Data-Flash reading/writing
SFR(ROM_DATA_H, 0x8F);     // data high byte for flash-ROM writing
SFR(ROM_CTRL, 0x86);       // WriteOnly: flash-ROM control
#define ROM_CMD_WRITE 0x9A // WriteOnly: flash-ROM word or Data-Flash byte write operation command
#define ROM_CMD_READ 0x8E  // WriteOnly: Data-Flash byte read operation command
// sfr ROM_STATUS      = 0x86;         // ReadOnly: flash-ROM status
#define ROM_STATUS ROM_CTRL
#define bROM_ADDR_OK 0x40 // ReadOnly: flash-ROM writing operation address valid flag, can be reviewed before or after operation: 0=invalid parameter, 1=address valid
#define bROM_CMD_ERR 0x02 // ReadOnly: flash-ROM operation command error flag: 0=command accepted, 1=unknown command

/*  Port Registers  */
#define P1_ADDR (0x90)      // port address
SFR(P1, P1_ADDR);           // port 1 input & output
SBIT(SCK, P1_ADDR, 7);      // serial clock for SPI0
SBIT(TXD1, P1_ADDR, 7);     // TXD output for UART1
SBIT(TIN5, P1_ADDR, 7);     // TIN5 for Touch-Key
SBIT(MISO, P1_ADDR, 6);     // master serial data input or slave serial data output for SPI0
SBIT(RXD1, P1_ADDR, 6);     // RXD input for UART1
SBIT(TIN4, P1_ADDR, 6);     // TIN4 for Touch-Key
SBIT(MOSI, P1_ADDR, 5);     // master serial data output or slave serial data input for SPI0
SBIT(PWM1, P1_ADDR, 5);     // PWM output for PWM1
SBIT(TIN3, P1_ADDR, 5);     // TIN3 for Touch-Key
SBIT(UCC2, P1_ADDR, 5);     // CC2 for USB type-C
SBIT(AIN2, P1_ADDR, 5);     // AIN2 for ADC
SBIT(T2_, P1_ADDR, 4);      // alternate pin for T2
SBIT(CAP1_, P1_ADDR, 4);    // alternate pin for CAP1
SBIT(SCS, P1_ADDR, 4);      // slave chip-selection input for SPI0
SBIT(TIN2, P1_ADDR, 4);     // TIN2 for Touch-Key
SBIT(UCC1, P1_ADDR, 4);     // CC1 for USB type-C
SBIT(AIN1, P1_ADDR, 4);     // AIN1 for ADC
SBIT(TXD_, P1_ADDR, 3);     // alternate pin for TXD of UART0
SBIT(RXD_, P1_ADDR, 2);     // alternate pin for RXD of UART0
SBIT(T2EX, P1_ADDR, 1);     // external trigger input for timer2 reload & capture
SBIT(CAP2, P1_ADDR, 1);     // capture2 input for timer2
SBIT(TIN1, P1_ADDR, 1);     // TIN1 for Touch-Key
SBIT(VBUS2, P1_ADDR, 1);    // VBUS2 for USB type-C
SBIT(AIN0, P1_ADDR, 1);     // AIN0 for ADC
SBIT(T2, P1_ADDR, 0);       // external count input
SBIT(CAP1, P1_ADDR, 0);     // capture1 input for timer2
SBIT(TIN0, P1_ADDR, 0);     // TIN0 for Touch-Key
#define P1_MOD_OC_ADDR 0x92 // port 1 output mode address
SFR(P1_MOD_OC, 0x92);       // port 1 output mode: 0=push-pull, 1=open-drain
SFR(P1_DIR_PU, 0x93);       // port 1 direction for push-pull or pullup enable for open-drain
                            // Pn_MOD_OC & Pn_DIR_PU: pin input & output configuration for Pn (n=1/3)
                            //   0 0:  float input only, without pullup resistance
                            //   0 1:  push-pull output, strong driving high level and low level
                            //   1 0:  open-drain output and input without pullup resistance
                            //   1 1:  quasi-bidirectional (standard 8051 mode), open-drain output and input with pullup resistance, just driving high level strongly for 2 clocks if turning output level from low to high
#define bSCK 0x80           // serial clock for SPI0
#define bTXD1 0x80          // TXD output for UART1
#define bMISO 0x40          // master serial data input or slave serial data output for SPI0
#define bRXD1 0x40          // RXD input for UART1
#define bMOSI 0x20          // master serial data output or slave serial data input for SPI0
#define bPWM1 0x20          // PWM output for PWM1
#define bUCC2 0x20          // CC2 for USB type-C
#define bAIN2 0x20          // AIN2 for ADC
#define bT2_ 0x10           // alternate pin for T2
#define bCAP1_ 0x10         // alternate pin for CAP1
#define bSCS 0x10           // slave chip-selection input for SPI0
#define bUCC1 0x10          // CC1 for USB type-C
#define bAIN1 0x10          // AIN1 for ADC
#define bTXD_ 0x08          // alternate pin for TXD of UART0
#define bRXD_ 0x04          // alternate pin for RXD of UART0
#define bT2EX 0x02          // external trigger input for timer2 reload & capture
#define bCAP2 bT2EX         // capture2 input for timer2
#define bVBUS2 0x02         // VBUS2 for USB type-C
#define bAIN0 0x02          // AIN0 for ADC
#define bT2 0x01            // external count input or clock output for timer2
#define bCAP1 bT2           // capture1 input for timer2
SFR(P2, 0xA0);              // port 2
#define P3_ADDR (0xB0)      // port 3 address
SFR(P3, P3_ADDR);           // port 3 input & output
SBIT(UDM, P3_ADDR, 7);      // ReadOnly: pin UDM input
SBIT(UDP, P3_ADDR, 6);      // ReadOnly: pin UDP input
SBIT(T1, P3_ADDR, 5);       // external count input for timer1
SBIT(PWM2, P3_ADDR, 4);     // PWM output for PWM2
SBIT(RXD1_, P3_ADDR, 4);    // alternate pin for RXD1
SBIT(T0, P3_ADDR, 4);       // external count input for timer0
SBIT(INT1, P3_ADDR, 3);     // external interrupt 1 input
SBIT(TXD1_, P3_ADDR, 2);    // alternate pin for TXD1
SBIT(INT0, P3_ADDR, 2);     // external interrupt 0 input
SBIT(VBUS1, P3_ADDR, 2);    // VBUS1 for USB type-C
SBIT(AIN3, P3_ADDR, 2);     // AIN3 for ADC
SBIT(PWM2_, P3_ADDR, 1);    // alternate pin for PWM2
SBIT(TXD, P3_ADDR, 1);      // TXD output for UART0
SBIT(PWM1_, P3_ADDR, 0);    // alternate pin for PWM1
SBIT(RXD, P3_ADDR, 0);      // RXD input for UART0
#define P3_MOD_OC_ADDR 0x96 // port 3 output mode address
SFR(P3_MOD_OC, 0x96);       // port 3 output mode: 0=push-pull, 1=open-drain
SFR(P3_DIR_PU, 0x97);       // port 3 direction for push-pull or pullup enable for open-drain
#define bUDM 0x80           // ReadOnly: pin UDM input
#define bUDP 0x40           // ReadOnly: pin UDP input
#define bT1 0x20            // external count input for timer1
#define bPWM2 0x10          // PWM output for PWM2
#define bRXD1_ 0x10         // alternate pin for RXD1
#define bT0 0x10            // external count input for timer0
#define bINT1 0x08          // external interrupt 1 input
#define bTXD1_ 0x04         // alternate pin for TXD1
#define bINT0 0x04          // external interrupt 0 input
#define bVBUS1 0x04         // VBUS1 for USB type-C
#define bAIN3 0x04          // AIN3 for ADC
#define bPWM2_ 0x02         // alternate pin for PWM2
#define bTXD 0x02           // TXD output for UART0
#define bPWM1_ 0x01         // alternate pin for PWM1
#define bRXD 0x01           // RXD input for UART0
SFR(PIN_FUNC, 0xC6);        // pin function selection
#define bUSB_IO_EN 0x80     // USB UDP/UDM I/O pin enable: 0=P3.6/P3.7 as GPIO, 1=P3.6/P3.7 as USB
#define bIO_INT_ACT 0x40    // ReadOnly: GPIO interrupt request action status
#define bUART1_PIN_X 0x20   // UART1 alternate pin enable: 0=RXD1/TXD1 on P1.6/P1.7, 1=RXD1/TXD1 on P3.4/P3.2
#define bUART0_PIN_X 0x10   // UART0 alternate pin enable: 0=RXD0/TXD0 on P3.0/P3.1, 1=RXD0/TXD0 on P1.2/P1.3
#define bPWM2_PIN_X 0x08    // PWM2 alternate pin enable: 0=PWM2 on P3.4, 1=PWM2 on P3.1
#define bPWM1_PIN_X 0x04    // PWM1 alternate pin enable: 0=PWM1 on P1.5, 1=PWM1 on P3.0
#define bT2EX_PIN_X 0x02    // T2EX/CAP2 alternate pin enable: 0=T2EX/CAP2 on P1.1, 1=T2EX/CAP2 on RST
#define bT2_PIN_X 0x01      // T2/CAP1 alternate pin enable: 0=T2/CAP1 on P1.1, 1=T2/CAP1 on P1.4
SFR(XBUS_AUX, 0xA2);        // xBUS auxiliary setting
#define bUART0_TX 0x80      // ReadOnly: indicate UART0 transmittal status
#define bUART0_RX 0x40      // ReadOnly: indicate UART0 receiving status
#define bSAFE_MOD_ACT 0x20  // ReadOnly: safe mode action status
#define GF2 0x08            // general purpose flag bit 2
#define bDPTR_AUTO_INC 0x04 // enable DPTR auto increase if finished MOVX_@DPTR instruction
#define DPS 0x01            // dual DPTR selection: 0=DPTR0 selected, 1=DPTR1 selected

/*  Timer0/1 Registers  */
#define TCON_ADDR (0x88) // timer 0/1 control and external interrupt control address
SFR(TCON, TCON_ADDR);    // timer 0/1 control and external interrupt control
SBIT(TF1, TCON_ADDR, 7); // timer1 overflow & interrupt flag, auto cleared when MCU enter interrupt routine
SBIT(TR1, TCON_ADDR, 6); // timer1 run enable
SBIT(TF0, TCON_ADDR, 5); // timer0 overflow & interrupt flag, auto cleared when MCU enter interrupt routine
SBIT(TR0, TCON_ADDR, 4); // timer0 run enable
SBIT(IE1, TCON_ADDR, 3); // INT1 interrupt flag, auto cleared when MCU enter interrupt routine
SBIT(IT1, TCON_ADDR, 2); // INT1 interrupt type: 0=low level action, 1=falling edge action
SBIT(IE0, TCON_ADDR, 1); // INT0 interrupt flag, auto cleared when MCU enter interrupt routine
SBIT(IT0, TCON_ADDR, 0); // INT0 interrupt type: 0=low level action, 1=falling edge action
SFR(TMOD, 0x89);         // timer 0/1 mode
#define bT1_GATE 0x80    // gate control of timer1: 0=timer1 run enable while TR1=1, 1=timer1 run enable while P3.3 (INT1) pin is high and TR1=1
#define bT1_CT 0x40      // counter or timer mode selection for timer1: 0=timer, use internal clock, 1=counter, use P3.5 (T1) pin falling edge as clock
#define bT1_M1 0x20      // timer1 mode high bit
#define bT1_M0 0x10      // timer1 mode low bit
#define MASK_T1_MOD 0x30 // bit mask of timer1 mode
                         // bT1_M1 & bT1_M0: timer1 mode
                         //   00: mode 0, 13-bit timer or counter by cascaded TH1 and lower 5 bits of TL1, the upper 3 bits of TL1 are ignored
                         //   01: mode 1, 16-bit timer or counter by cascaded TH1 and TL1
                         //   10: mode 2, TL1 operates as 8-bit timer or counter, and TH1 provide initial value for TL1 auto-reload
                         //   11: mode 3, stop timer1
#define bT0_GATE 0x08    // gate control of timer0: 0=timer0 run enable while TR0=1, 1=timer0 run enable while P3.2 (INT0) pin is high and TR0=1
#define bT0_CT 0x04      // counter or timer mode selection for timer0: 0=timer, use internal clock, 1=counter, use P3.4 (T0) pin falling edge as clock
#define bT0_M1 0x02      // timer0 mode high bit
#define bT0_M0 0x01      // timer0 mode low bit
#define MASK_T0_MOD 0x03 // bit mask of timer0 mode
                         // bT0_M1 & bT0_M0: timer0 mode
                         //   00: mode 0, 13-bit timer or counter by cascaded TH0 and lower 5 bits of TL0, the upper 3 bits of TL0 are ignored
                         //   01: mode 1, 16-bit timer or counter by cascaded TH0 and TL0
                         //   10: mode 2, TL0 operates as 8-bit timer or counter, and TH0 provide initial value for TL0 auto-reload
                         //   11: mode 3, TL0 is 8-bit timer or counter controlled by standard timer0 bits, TH0 is 8-bit timer using TF1 and controlled by TR1, timer1 run enable if it is not mode 3
SFR(TL0, 0x8A);          // low byte of timer 0 count
SFR(TL1, 0x8B);          // low byte of timer 1 count
SFR(TH0, 0x8C);          // high byte of timer 0 count
SFR(TH1, 0x8D);          // high byte of timer 1 count

/*  UART0 Registers  */
#define SCON_ADDR (0x98)    // UART0 control (serial port control) address
SFR(SCON, SCON_ADDR);       // UART0 control (serial port control)
SBIT(SM0, SCON_ADDR, 7);    // UART0 mode bit0, selection data bit: 0=8 bits data, 1=9 bits data
SBIT(SM1, SCON_ADDR, 6);    // UART0 mode bit1, selection baud rate: 0=fixed, 1=variable
                            // SM0 & SM1: UART0 mode
                            //    00 - mode 0, shift Register, baud rate fixed at: Fsys/12
                            //    01 - mode 1, 8-bit UART,     baud rate = variable by timer1 or timer2 overflow rate
                            //    10 - mode 2, 9-bit UART,     baud rate fixed at: Fsys/128@SMOD=0, Fsys/32@SMOD=1
                            //    11 - mode 3, 9-bit UART,     baud rate = variable by timer1 or timer2 overflow rate
SBIT(SM2, SCON_ADDR, 5);    // enable multi-device communication in mode 2/3
#define MASK_UART0_MOD 0xE0 // bit mask of UART0 mode
SBIT(REN, SCON_ADDR, 4);    // enable UART0 receiving
SBIT(TB8, SCON_ADDR, 3);    // the 9th transmitted data bit in mode 2/3
SBIT(RB8, SCON_ADDR, 2);    // 9th data bit received in mode 2/3, or stop bit received for mode 1
SBIT(TI, SCON_ADDR, 1);     // transmit interrupt flag, set by hardware after completion of a serial transmittal, need software clear
SBIT(RI, SCON_ADDR, 0);     // receive interrupt flag, set by hardware after completion of a serial receiving, need software clear
SFR(SBUF, 0x99);            // UART0 data buffer: reading for receiving, writing for transmittal

/*  Timer2/Capture2 Registers  */
#define T2CON_ADDR (0xC8)    // timer 2 control address
SFR(T2CON, T2CON_ADDR);      // timer 2 control
SBIT(TF2, T2CON_ADDR, 7);    // timer2 overflow & interrupt flag, need software clear, the flag will not be set when either RCLK=1 or TCLK=1
SBIT(CAP1F, T2CON_ADDR, 7);  // timer2 capture 1 interrupt flag, set by T2 edge trigger if bT2_CAP1_EN=1, need software clear
SBIT(EXF2, T2CON_ADDR, 6);   // timer2 external flag, set by T2EX edge trigger if EXEN2=1, need software clear
SBIT(RCLK, T2CON_ADDR, 5);   // selection UART0 receiving clock: 0=timer1 overflow pulse, 1=timer2 overflow pulse
SBIT(TCLK, T2CON_ADDR, 4);   // selection UART0 transmittal clock: 0=timer1 overflow pulse, 1=timer2 overflow pulse
SBIT(EXEN2, T2CON_ADDR, 3);  // enable T2EX trigger function: 0=ignore T2EX, 1=trigger reload or capture by T2EX edge
SBIT(TR2, T2CON_ADDR, 2);    // timer2 run enable
SBIT(C_T2, T2CON_ADDR, 1);   // timer2 clock source selection: 0=timer base internal clock, 1=external edge counter base T2 falling edge
SBIT(CP_RL2, T2CON_ADDR, 0); // timer2 function selection (force 0 if RCLK=1 or TCLK=1): 0=timer and auto reload if count overflow or T2EX edge, 1=capture by T2EX edge
SFR(T2MOD, 0xC9);            // timer 2 mode and timer 0/1/2 clock mode
#define bTMR_CLK 0x80        // fastest internal clock mode for timer 0/1/2 under faster clock mode: 0=use divided clock, 1=use original Fsys as clock without dividing
#define bT2_CLK 0x40         // timer2 internal clock frequency selection: 0=standard clock, Fsys/12 for timer mode, Fsys/4 for UART0 clock mode,
                             //   1=faster clock, Fsys/4 @bTMR_CLK=0 or Fsys @bTMR_CLK=1 for timer mode, Fsys/2 @bTMR_CLK=0 or Fsys @bTMR_CLK=1 for UART0 clock mode
#define bT1_CLK 0x20         // timer1 internal clock frequency selection: 0=standard clock, Fsys/12, 1=faster clock, Fsys/4 if bTMR_CLK=0 or Fsys if bTMR_CLK=1
#define bT0_CLK 0x10         // timer0 internal clock frequency selection: 0=standard clock, Fsys/12, 1=faster clock, Fsys/4 if bTMR_CLK=0 or Fsys if bTMR_CLK=1
#define bT2_CAP_M1 0x08      // timer2 capture mode high bit
#define bT2_CAP_M0 0x04      // timer2 capture mode low bit
// bT2_CAP_M1 & bT2_CAP_M0: timer2 capture point selection
//   x0: from falling edge to falling edge
//   01: from any edge to any edge (level changing)
//   11: from rising edge to rising edge
#define T2OE 0x02        // enable timer2 generated clock output: 0=disable output, 1=enable clock output at T2 pin, frequency = TF2/2
#define bT2_CAP1_EN 0x01 // enable T2 trigger function for capture 1 of timer2 if RCLK=0 & TCLK=0 & CP_RL2=1 & C_T2=0 & T2OE=0
SFR16(RCAP2, 0xCA);      // reload & capture value, little-endian
SFR(RCAP2L, 0xCA);       // low byte of reload & capture value
SFR(RCAP2H, 0xCB);       // high byte of reload & capture value
SFR16(T2COUNT, 0xCC);    // counter, little-endian
SFR(TL2, 0xCC);          // low byte of timer 2 count
SFR(TH2, 0xCD);          // high byte of timer 2 count
SFR16(T2CAP1, 0xCE);     // ReadOnly: capture 1 value for timer2
SFR(T2CAP1L, 0xCE);      // ReadOnly: capture 1 value low byte for timer2
SFR(T2CAP1H, 0xCF);      // ReadOnly: capture 1 value high byte for timer2

/*  PWM1/2 Registers  */
SFR(PWM_DATA2, 0x9B);     // PWM data for PWM2
SFR(PWM_DATA1, 0x9C);     // PWM data for PWM1
SFR(PWM_CTRL, 0x9D);      // PWM 1/2 control
#define bPWM_IE_END 0x80  // enable interrupt for PWM mode cycle end
#define bPWM2_POLAR 0x40  // PWM2 output polarity: 0=default low and high action, 1=default high and low action
#define bPWM1_POLAR 0x20  // PWM1 output polarity: 0=default low and high action, 1=default high and low action
#define bPWM_IF_END 0x10  // interrupt flag for cycle end, write 1 to clear or write PWM_CYCLE or load new data to clear
#define bPWM2_OUT_EN 0x08 // PWM2 output enable
#define bPWM1_OUT_EN 0x04 // PWM1 output enable
#define bPWM_CLR_ALL 0x02 // force clear FIFO and count of PWM1/2
SFR(PWM_CK_SE, 0x9E);     // clock divisor setting

/*  SPI0/Master0/Slave Registers  */
#define SPI0_STAT_ADDR (0xF8)         // SPI 0 status address
SFR(SPI0_STAT, SPI0_STAT_ADDR);       // SPI 0 status
SBIT(S0_FST_ACT, SPI0_STAT_ADDR, 7);  // ReadOnly: indicate first byte received status for SPI0
SBIT(S0_IF_OV, SPI0_STAT_ADDR, 6);    // interrupt flag for slave mode FIFO overflow, direct bit address clear or write 1 to clear
SBIT(S0_IF_FIRST, SPI0_STAT_ADDR, 5); // interrupt flag for first byte received, direct bit address clear or write 1 to clear
SBIT(S0_IF_BYTE, SPI0_STAT_ADDR, 4);  // interrupt flag for a byte data exchanged, direct bit address clear or write 1 to clear or accessing FIFO to clear if bS0_AUTO_IF=1
SBIT(S0_FREE, SPI0_STAT_ADDR, 3);     // ReadOnly: SPI0 free status
SBIT(S0_T_FIFO, SPI0_STAT_ADDR, 2);   // ReadOnly: tx FIFO count for SPI0
SBIT(S0_R_FIFO, SPI0_STAT_ADDR, 0);   // ReadOnly: rx FIFO count for SPI0
SFR(SPI0_DATA, 0xF9);                 // FIFO data port: reading for receiving, writing for transmittal
SFR(SPI0_CTRL, 0xFA);                 // SPI 0 control
#define bS0_MISO_OE 0x80              // SPI0 MISO output enable
#define bS0_MOSI_OE 0x40              // SPI0 MOSI output enable
#define bS0_SCK_OE 0x20               // SPI0 SCK output enable
#define bS0_DATA_DIR 0x10             // SPI0 data direction: 0=out(master_write), 1=in(master_read)
#define bS0_MST_CLK 0x08              // SPI0 master clock mode: 0=mode 0 with default low, 1=mode 3 with default high
#define bS0_2_WIRE 0x04               // enable SPI0 two wire mode: 0=3 wire (SCK+MOSI+MISO), 1=2 wire (SCK+MISO)
#define bS0_CLR_ALL 0x02              // force clear FIFO and count of SPI0
#define bS0_AUTO_IF 0x01              // enable FIFO accessing to auto clear S0_IF_BYTE interrupt flag
SFR(SPI0_CK_SE, 0xFB);                // clock divisor setting
SFR(SPI0_S_PRE, 0xFB);                // preset value for SPI slave
SFR(SPI0_SETUP, 0xFC);                // SPI 0 setup
#define bS0_MODE_SLV 0x80             // SPI0 slave mode: 0=master, 1=slave
#define bS0_IE_FIFO_OV 0x40           // enable interrupt for slave mode FIFO overflow
#define bS0_IE_FIRST 0x20             // enable interrupt for first byte received for SPI0 slave mode
#define bS0_IE_BYTE 0x10              // enable interrupt for a byte received
#define bS0_BIT_ORDER 0x08            // SPI0 bit data order: 0=MSB first, 1=LSB first
#define bS0_SLV_SELT 0x02             // ReadOnly: SPI0 slave mode chip selected status: 0=unselected, 1=selected
#define bS0_SLV_PRELOAD 0x01          // ReadOnly: SPI0 slave mode data pre-loading status just after chip-selection

/*  UART1 Registers  */
#define SCON1_ADDR (0xC0)    // UART1 control (serial port control) address
SFR(SCON1, SCON1_ADDR);      // UART1 control (serial port control)
SBIT(U1SM0, SCON1_ADDR, 7);  // UART1 mode, selection data bit: 0=8 bits data, 1=9 bits data
SBIT(U1SMOD, SCON1_ADDR, 5); // UART1 2X baud rate selection: 0=slow(Fsys/32/(256-SBAUD1)), 1=fast(Fsys/16/(256-SBAUD1))
SBIT(U1REN, SCON1_ADDR, 4);  // enable UART1 receiving
SBIT(U1TB8, SCON1_ADDR, 3);  // the 9th transmitted data bit in 9 bits data mode
SBIT(U1RB8, SCON1_ADDR, 2);  // 9th data bit received in 9 bits data mode, or stop bit received for 8 bits data mode
SBIT(U1TI, SCON1_ADDR, 1);   // transmit interrupt flag, set by hardware after completion of a serial transmittal, need software clear
SBIT(U1RI, SCON1_ADDR, 0);   // receive interrupt flag, set by hardware after completion of a serial receiving, need software clear
SFR(SBUF1, 0xC1);            // UART1 data buffer: reading for receiving, writing for transmittal
SFR(SBAUD1, 0xC2);           // UART1 baud rate setting

/*  ADC and comparator Registers  */
#define ADC_CTRL_ADDR (0x80)       // ADC control address
SFR(ADC_CTRL, ADC_CTRL_ADDR);      // ADC control
SBIT(CMPO, ADC_CTRL_ADDR, 7);      // ReadOnly: comparator result input
SBIT(CMP_IF, ADC_CTRL_ADDR, 6);    // flag for comparator result changed, direct bit address clear
SBIT(ADC_IF, ADC_CTRL_ADDR, 5);    // interrupt flag for ADC finished, direct bit address clear
SBIT(ADC_START, ADC_CTRL_ADDR, 4); // set 1 to start ADC, auto cleared when ADC finished
SBIT(CMP_CHAN, ADC_CTRL_ADDR, 3);  // comparator IN- input channel selection: 0=AIN1, 1=AIN3
SBIT(ADC_CHAN1, ADC_CTRL_ADDR, 1); // ADC/comparator IN+ channel selection high bit
SBIT(ADC_CHAN0, ADC_CTRL_ADDR, 0); // ADC/comparator IN+ channel selection low bit
// ADC_CHAN1 & ADC_CHAN0: ADC/comparator IN+ channel selection
//   00: AIN0(P1.1)
//   01: AIN1(P1.4)
//   10: AIN2(P1.5)
//   11: AIN3(P3.2)
SFR(ADC_CFG, 0x9A);   // ADC config
#define bADC_EN 0x08  // control ADC power: 0=shut down ADC, 1=enable power for ADC
#define bCMP_EN 0x04  // control comparator power: 0=shut down comparator, 1=enable power for comparator
#define bADC_CLK 0x01 // ADC clock frequency selection: 0=slow clock, 384 Fosc cycles for each ADC, 1=fast clock, 96 Fosc cycles for each ADC
SFR(ADC_DATA, 0x9F);  // ReadOnly: ADC data

/*  Touch-key timer Registers  */
SFR(TKEY_CTRL, 0xC3);   // touch-key control
#define bTKC_IF 0x80    // ReadOnly: interrupt flag for touch-key timer, cleared by writing touch-key control or auto cleared when start touch-key checking
#define bTKC_2MS 0x10   // touch-key timer cycle selection: 0=1mS, 1=2mS
#define bTKC_CHAN2 0x04 // touch-key channel selection high bit
#define bTKC_CHAN1 0x02 // touch-key channel selection middle bit
#define bTKC_CHAN0 0x01 // touch-key channel selection low bit
// bTKC_CHAN2 & bTKC_CHAN1 & bTKC_CHAN0: touch-key channel selection
//   000: disable touch-key
//   001: TIN0(P1.0)
//   010: TIN1(P1.1)
//   011: TIN2(P1.4)
//   100: TIN3(P1.5)
//   101: TIN4(P1.6)
//   110: TIN5(P1.7)
//   111: enable touch-key but disable all channel
SFR16(TKEY_DAT, 0xC4); // ReadOnly: touch-key data, little-endian
SFR(TKEY_DATL, 0xC4);  // ReadOnly: low byte of touch-key data
SFR(TKEY_DATH, 0xC5);  // ReadOnly: high byte of touch-key data
#define bTKD_CHG 0x80  // ReadOnly: indicate control changed, current data maybe invalid

/*  USB/Host/Device Registers  */
SFR(USB_C_CTRL, 0x91);    // USB type-C control
#define bVBUS2_PD_EN 0x80 // USB VBUS2 10K pulldown resistance: 0=disable, 1=enable pullup
#define bUCC2_PD_EN 0x40  // USB CC2 5.1K pulldown resistance: 0=disable, 1=enable pulldown
#define bUCC2_PU1_EN 0x20 // USB CC2 pullup resistance control high bit
#define bUCC2_PU0_EN 0x10 // USB CC2 pullup resistance control low bit
#define bVBUS1_PD_EN 0x08 // USB VBUS1 10K pulldown resistance: 0=disable, 1=enable pullup
#define bUCC1_PD_EN 0x04  // USB CC1 5.1K pulldown resistance: 0=disable, 1=enable pulldown
#define bUCC1_PU1_EN 0x02 // USB CC1 pullup resistance control high bit
#define bUCC1_PU0_EN 0x01 // USB CC1 pullup resistance control low bit
// bUCC?_PU1_EN & bUCC?_PU0_EN: USB CC pullup resistance selection
//   00: disable pullup resistance
//   01: enable 56K pullup resistance for default USB power
//   10: enable 22K pullup resistance for 1.5A USB power
//   11: enable 10K pullup resistance for 3A USB power
SFR(UDEV_CTRL, 0xD1);      // USB device physical port control
#define bUD_PD_DIS 0x80    // disable USB UDP/UDM pulldown resistance: 0=enable pulldown, 1=disable
#define bUD_DP_PIN 0x20    // ReadOnly: indicate current UDP pin level
#define bUD_DM_PIN 0x10    // ReadOnly: indicate current UDM pin level
#define bUD_LOW_SPEED 0x04 // enable USB physical port low speed: 0=full speed, 1=low speed
#define bUD_GP_BIT 0x02    // general purpose bit
#define bUD_PORT_EN 0x01   // enable USB physical port I/O: 0=disable, 1=enable
// sfr UHOST_CTRL      = 0xD1;         // USB host physical port control
#define UHOST_CTRL UDEV_CTRL
#define bUH_PD_DIS 0x80     // disable USB UDP/UDM pulldown resistance: 0=enable pulldown, 1=disable
#define bUH_DP_PIN 0x20     // ReadOnly: indicate current UDP pin level
#define bUH_DM_PIN 0x10     // ReadOnly: indicate current UDM pin level
#define bUH_LOW_SPEED 0x04  // enable USB port low speed: 0=full speed, 1=low speed
#define bUH_BUS_RESET 0x02  // control USB bus reset: 0=normal, 1=force bus reset
#define bUH_PORT_EN 0x01    // enable USB port: 0=disable, 1=enable port, automatic disabled if USB device detached
SFR(UEP1_CTRL, 0xD2);       // endpoint 1 control
#define bUEP_R_TOG 0x80     // expected data toggle flag of USB endpoint X receiving (OUT): 0=DATA0, 1=DATA1
#define bUEP_T_TOG 0x40     // prepared data toggle flag of USB endpoint X transmittal (IN): 0=DATA0, 1=DATA1
#define bUEP_AUTO_TOG 0x10  // enable automatic toggle after successful transfer completion on endpoint 1/2/3: 0=manual toggle, 1=automatic toggle
#define bUEP_R_RES1 0x08    // handshake response type high bit for USB endpoint X receiving (OUT)
#define bUEP_R_RES0 0x04    // handshake response type low bit for USB endpoint X receiving (OUT)
#define MASK_UEP_R_RES 0x0C // bit mask of handshake response type for USB endpoint X receiving (OUT)
#define UEP_R_RES_ACK 0x00
#define UEP_R_RES_TOUT 0x04
#define UEP_R_RES_NAK 0x08
#define UEP_R_RES_STALL 0x0C
// bUEP_R_RES1 & bUEP_R_RES0: handshake response type for USB endpoint X receiving (OUT)
//   00: ACK (ready)
//   01: no response, time out to host, for non-zero endpoint isochronous transactions
//   10: NAK (busy)
//   11: STALL (error)
#define bUEP_T_RES1 0x02    // handshake response type high bit for USB endpoint X transmittal (IN)
#define bUEP_T_RES0 0x01    // handshake response type low bit for USB endpoint X transmittal (IN)
#define MASK_UEP_T_RES 0x03 // bit mask of handshake response type for USB endpoint X transmittal (IN)
#define UEP_T_RES_ACK 0x00
#define UEP_T_RES_TOUT 0x01
#define UEP_T_RES_NAK 0x02
#define UEP_T_RES_STALL 0x03
// bUEP_T_RES1 & bUEP_T_RES0: handshake response type for USB endpoint X transmittal (IN)
//   00: DATA0 or DATA1 then expecting ACK (ready)
//   01: DATA0 or DATA1 then expecting no response, time out from host, for non-zero endpoint isochronous transactions
//   10: NAK (busy)
//   11: STALL (error)
SFR(UEP1_T_LEN, 0xD3);                  // endpoint 1 transmittal length
SFR(UEP2_CTRL, 0xD4);                   // endpoint 2 control
SFR(UEP2_T_LEN, 0xD5);                  // endpoint 2 transmittal length
SFR(UEP3_CTRL, 0xD6);                   // endpoint 3 control
SFR(UEP3_T_LEN, 0xD7);                  // endpoint 3 transmittal length
#define USB_INT_FG_ADDR (0xD8)          // USB interrupt flag address
SFR(USB_INT_FG, USB_INT_FG_ADDR);       // USB interrupt flag
SBIT(U_IS_NAK, USB_INT_FG_ADDR, 7);     // ReadOnly: indicate current USB transfer is NAK received
SBIT(U_TOG_OK, USB_INT_FG_ADDR, 6);     // ReadOnly: indicate current USB transfer toggle is OK
SBIT(U_SIE_FREE, USB_INT_FG_ADDR, 5);   // ReadOnly: indicate USB SIE free status
SBIT(UIF_FIFO_OV, USB_INT_FG_ADDR, 4);  // FIFO overflow interrupt flag for USB, direct bit address clear or write 1 to clear
SBIT(UIF_HST_SOF, USB_INT_FG_ADDR, 3);  // host SOF timer interrupt flag for USB host, direct bit address clear or write 1 to clear
SBIT(UIF_SUSPEND, USB_INT_FG_ADDR, 2);  // USB suspend or resume event interrupt flag, direct bit address clear or write 1 to clear
SBIT(UIF_TRANSFER, USB_INT_FG_ADDR, 1); // USB transfer completion interrupt flag, direct bit address clear or write 1 to clear
SBIT(UIF_DETECT, USB_INT_FG_ADDR, 0);   // device detected event interrupt flag for USB host mode, direct bit address clear or write 1 to clear
SBIT(UIF_BUS_RST, USB_INT_FG_ADDR, 0);  // bus reset event interrupt flag for USB device mode, direct bit address clear or write 1 to clear
SFR(USB_INT_ST, 0xD9);                  // ReadOnly: USB interrupt status
#define bUIS_IS_NAK 0x80                // ReadOnly: indicate current USB transfer is NAK received for USB device mode
#define bUIS_TOG_OK 0x40                // ReadOnly: indicate current USB transfer toggle is OK
#define bUIS_TOKEN1 0x20                // ReadOnly: current token PID code bit 1 received for USB device mode
#define bUIS_TOKEN0 0x10                // ReadOnly: current token PID code bit 0 received for USB device mode
#define MASK_UIS_TOKEN 0x30             // ReadOnly: bit mask of current token PID code received for USB device mode
#define UIS_TOKEN_OUT 0x00
#define UIS_TOKEN_SOF 0x10
#define UIS_TOKEN_IN 0x20
#define UIS_TOKEN_SETUP 0x30
// bUIS_TOKEN1 & bUIS_TOKEN0: current token PID code received for USB device mode
//   00: OUT token PID received
//   01: SOF token PID received
//   10: IN token PID received
//   11: SETUP token PID received
#define MASK_UIS_ENDP 0x0F    // ReadOnly: bit mask of current transfer endpoint number for USB device mode
#define MASK_UIS_H_RES 0x0F   // ReadOnly: bit mask of current transfer handshake response for USB host mode: 0000=no response, time out from device, others=handshake response PID received
SFR(USB_MIS_ST, 0xDA);        // ReadOnly: USB miscellaneous status
#define bUMS_SOF_PRES 0x80    // ReadOnly: indicate host SOF timer presage status
#define bUMS_SOF_ACT 0x40     // ReadOnly: indicate host SOF timer action status for USB host
#define bUMS_SIE_FREE 0x20    // ReadOnly: indicate USB SIE free status
#define bUMS_R_FIFO_RDY 0x10  // ReadOnly: indicate USB receiving FIFO ready status (not empty)
#define bUMS_BUS_RESET 0x08   // ReadOnly: indicate USB bus reset status
#define bUMS_SUSPEND 0x04     // ReadOnly: indicate USB suspend status
#define bUMS_DM_LEVEL 0x02    // ReadOnly: indicate UDM level saved at device attached to USB host
#define bUMS_DEV_ATTACH 0x01  // ReadOnly: indicate device attached status on USB host
SFR(USB_RX_LEN, 0xDB);        // ReadOnly: USB receiving length
SFR(UEP0_CTRL, 0xDC);         // endpoint 0 control
SFR(UEP0_T_LEN, 0xDD);        // endpoint 0 transmittal length
SFR(UEP4_CTRL, 0xDE);         // endpoint 4 control
SFR(UEP4_T_LEN, 0xDF);        // endpoint 4 transmittal length
SFR(USB_INT_EN, 0xE1);        // USB interrupt enable
#define bUIE_DEV_SOF 0x80     // enable interrupt for SOF received for USB device mode
#define bUIE_DEV_NAK 0x40     // enable interrupt for NAK responded for USB device mode
#define bUIE_FIFO_OV 0x10     // enable interrupt for FIFO overflow
#define bUIE_HST_SOF 0x08     // enable interrupt for host SOF timer action for USB host mode
#define bUIE_SUSPEND 0x04     // enable interrupt for USB suspend or resume event
#define bUIE_TRANSFER 0x02    // enable interrupt for USB transfer completion
#define bUIE_DETECT 0x01      // enable interrupt for USB device detected event for USB host mode
#define bUIE_BUS_RST 0x01     // enable interrupt for USB bus reset event for USB device mode
SFR(USB_CTRL, 0xE2);          // USB base control
#define bUC_HOST_MODE 0x80    // enable USB host mode: 0=device mode, 1=host mode
#define bUC_LOW_SPEED 0x40    // enable USB low speed: 0=full speed, 1=low speed
#define bUC_DEV_PU_EN 0x20    // USB device enable and internal pullup resistance enable
#define bUC_SYS_CTRL1 0x20    // USB system control high bit
#define bUC_SYS_CTRL0 0x10    // USB system control low bit
#define MASK_UC_SYS_CTRL 0x30 // bit mask of USB system control
// bUC_HOST_MODE & bUC_SYS_CTRL1 & bUC_SYS_CTRL0: USB system control
//   0 00: disable USB device and disable internal pullup resistance
//   0 01: enable USB device and disable internal pullup resistance, need external pullup resistance
//   0 1x: enable USB device and enable internal pullup resistance
//   1 00: enable USB host and normal status
//   1 01: enable USB host and force UDP/UDM output SE0 state
//   1 10: enable USB host and force UDP/UDM output J state
//   1 11: enable USB host and force UDP/UDM output resume or K state
#define bUC_INT_BUSY 0x08  // enable automatic responding busy for device mode or automatic pause for host mode during interrupt flag UIF_TRANSFER valid
#define bUC_RESET_SIE 0x04 // force reset USB SIE, need software clear
#define bUC_CLR_ALL 0x02   // force clear FIFO and count of USB
#define bUC_DMA_EN 0x01    // DMA enable and DMA interrupt enable for USB
SFR(USB_DEV_AD, 0xE3);     // USB device address, lower 7 bits for USB device address
#define bUDA_GP_BIT 0x80   // general purpose bit
#define MASK_USB_ADDR 0x7F // bit mask for USB device address
SFR16(UEP2_DMA, 0xE4);     // endpoint 2 buffer start address, little-endian
SFR(UEP2_DMA_L, 0xE4);     // endpoint 2 buffer start address low byte
SFR(UEP2_DMA_H, 0xE5);     // endpoint 2 buffer start address high byte
SFR16(UEP3_DMA, 0xE6);     // endpoint 3 buffer start address, little-endian
SFR(UEP3_DMA_L, 0xE6);     // endpoint 3 buffer start address low byte
SFR(UEP3_DMA_H, 0xE7);     // endpoint 3 buffer start address high byte
SFR(UEP4_1_MOD, 0xEA);     // endpoint 4/1 mode
#define bUEP1_RX_EN 0x80   // enable USB endpoint 1 receiving (OUT)
#define bUEP1_TX_EN 0x40   // enable USB endpoint 1 transmittal (IN)
#define bUEP1_BUF_MOD 0x10 // buffer mode of USB endpoint 1
// bUEPn_RX_EN & bUEPn_TX_EN & bUEPn_BUF_MOD: USB endpoint 1/2/3 buffer mode, buffer start address is UEPn_DMA
//   0 0 x:  disable endpoint and disable buffer
//   1 0 0:  64 bytes buffer for receiving (OUT endpoint)
//   1 0 1:  dual 64 bytes buffer by toggle bit bUEP_R_TOG selection for receiving (OUT endpoint), total=128bytes
//   0 1 0:  64 bytes buffer for transmittal (IN endpoint)
//   0 1 1:  dual 64 bytes buffer by toggle bit bUEP_T_TOG selection for transmittal (IN endpoint), total=128bytes
//   1 1 0:  64 bytes buffer for receiving (OUT endpoint) + 64 bytes buffer for transmittal (IN endpoint), total=128bytes
//   1 1 1:  dual 64 bytes buffer by bUEP_R_TOG selection for receiving (OUT endpoint) + dual 64 bytes buffer by bUEP_T_TOG selection for transmittal (IN endpoint), total=256bytes
#define bUEP4_RX_EN 0x08 // enable USB endpoint 4 receiving (OUT)
#define bUEP4_TX_EN 0x04 // enable USB endpoint 4 transmittal (IN)
// bUEP4_RX_EN & bUEP4_TX_EN: USB endpoint 4 buffer mode, buffer start address is UEP0_DMA
//   0 0:  single 64 bytes buffer for endpoint 0 receiving & transmittal (OUT & IN endpoint)
//   1 0:  single 64 bytes buffer for endpoint 0 receiving & transmittal (OUT & IN endpoint) + 64 bytes buffer for endpoint 4 receiving (OUT endpoint), total=128bytes
//   0 1:  single 64 bytes buffer for endpoint 0 receiving & transmittal (OUT & IN endpoint) + 64 bytes buffer for endpoint 4 transmittal (IN endpoint), total=128bytes
//   1 1:  single 64 bytes buffer for endpoint 0 receiving & transmittal (OUT & IN endpoint)
//           + 64 bytes buffer for endpoint 4 receiving (OUT endpoint) + 64 bytes buffer for endpoint 4 transmittal (IN endpoint), total=192bytes
SFR(UEP2_3_MOD, 0xEB);     // endpoint 2/3 mode
#define bUEP3_RX_EN 0x80   // enable USB endpoint 3 receiving (OUT)
#define bUEP3_TX_EN 0x40   // enable USB endpoint 3 transmittal (IN)
#define bUEP3_BUF_MOD 0x10 // buffer mode of USB endpoint 3
#define bUEP2_RX_EN 0x08   // enable USB endpoint 2 receiving (OUT)
#define bUEP2_TX_EN 0x04   // enable USB endpoint 2 transmittal (IN)
#define bUEP2_BUF_MOD 0x01 // buffer mode of USB endpoint 2
SFR16(UEP0_DMA, 0xEC);     // endpoint 0 buffer start address, little-endian
SFR(UEP0_DMA_L, 0xEC);     // endpoint 0 buffer start address low byte
SFR(UEP0_DMA_H, 0xED);     // endpoint 0 buffer start address high byte
SFR16(UEP1_DMA, 0xEE);     // endpoint 1 buffer start address, little-endian
SFR(UEP1_DMA_L, 0xEE);     // endpoint 1 buffer start address low byte
SFR(UEP1_DMA_H, 0xEF);     // endpoint 1 buffer start address high byte

/*----- XDATA: xRAM ------------------------------------------*/

#define XDATA_RAM_SIZE 0x0400 // size of expanded xRAM, xdata SRAM embedded chip

/*----- Reference Information --------------------------------------------*/
#define ID_CH554 0x54 // chip ID

/* Interrupt routine address and interrupt number */
#define INT_ADDR_INT0 0x0003  // interrupt vector address for INT0
#define INT_ADDR_TMR0 0x000B  // interrupt vector address for timer0
#define INT_ADDR_INT1 0x0013  // interrupt vector address for INT1
#define INT_ADDR_TMR1 0x001B  // interrupt vector address for timer1
#define INT_ADDR_UART0 0x0023 // interrupt vector address for UART0
#define INT_ADDR_TMR2 0x002B  // interrupt vector address for timer2
#define INT_ADDR_SPI0 0x0033  // interrupt vector address for SPI0
#define INT_ADDR_TKEY 0x003B  // interrupt vector address for touch-key timer
#define INT_ADDR_USB 0x0043   // interrupt vector address for USB
#define INT_ADDR_ADC 0x004B   // interrupt vector address for ADC
#define INT_ADDR_UART1 0x0053 // interrupt vector address for UART1
#define INT_ADDR_PWMX 0x005B  // interrupt vector address for PWM1/2
#define INT_ADDR_GPIO 0x0063  // interrupt vector address for GPIO
#define INT_ADDR_WDOG 0x006B  // interrupt vector address for watch-dog timer
#define INT_NO_INT0 0         // interrupt number for INT0
#define INT_NO_TMR0 1         // interrupt number for timer0
#define INT_NO_INT1 2         // interrupt number for INT1
#define INT_NO_TMR1 3         // interrupt number for timer1
#define INT_NO_UART0 4        // interrupt number for UART0
#define INT_NO_TMR2 5         // interrupt number for timer2
#define INT_NO_SPI0 6         // interrupt number for SPI0
#define INT_NO_TKEY 7         // interrupt number for touch-key timer
#define INT_NO_USB 8          // interrupt number for USB
#define INT_NO_ADC 9          // interrupt number for ADC
#define INT_NO_UART1 10       // interrupt number for UART1
#define INT_NO_PWMX 11        // interrupt number for PWM1/2
#define INT_NO_GPIO 12        // interrupt number for GPIO
#define INT_NO_WDOG 13        // interrupt number for watch-dog timer

/* Special Program Space */
#define DATA_FLASH_ADDR SCON1_ADDR00 // start address of Data-Flash
#define BOOT_LOAD_ADDR 0x3800        // start address of boot loader program
#define ROM_CFG_ADDR 0x3FF8          // chip configuration information address
#define ROM_CHIP_ID_HX 0x3FFA        // chip ID number highest byte (only low byte valid)
#define ROM_CHIP_ID_LO 0x3FFC        // chip ID number low word
#define ROM_CHIP_ID_HI 0x3FFE        // chip ID number high word

/*
New Instruction:   MOVX @DPTR1,A
Instruction Code:  0xA5
Instruction Cycle: 1
Instruction Operation:
   step-1. write ACC @DPTR1 into xdata SRAM embedded chip
   step-2. increase DPTR1
ASM example:
       INC  XBUS_AUX
       MOV  DPTR,#TARGET_ADDR ;DPTR1
       DEC  XBUS_AUX
       MOV  DPTR,#SOURCE_ADDR ;DPTR0
       MOV  R7,#xxH
 LOOP: MOVX A,@DPTR ;DPTR0
       INC  DPTR    ;DPTR0, if need
       DB   0A5H    ;MOVX @DPTR1,A & INC DPTR1
       DJNZ R7,LOOP
*/

#endif // __CH552_H__

#ifndef __USB_DEF__
#define __USB_DEF__

/*----- USB constant and structure define --------------------------------*/

/* USB PID */
#ifndef USB_PID_SETUP
#define USB_PID_NULL 0x00 /* reserved PID */
#define USB_PID_SOF 0x05
#define USB_PID_SETUP 0x0D
#define USB_PID_IN 0x09
#define USB_PID_OUT 0x01
#define USB_PID_ACK 0x02
#define USB_PID_NAK 0x0A
#define USB_PID_STALL 0x0E
#define USB_PID_DATA0 0x03
#define USB_PID_DATA1 0x0B
#define USB_PID_PRE 0x0C
#endif

/* USB standard device request code */
#ifndef USB_GET_DESCRIPTOR
#define USB_GET_STATUS 0x00
#define USB_CLEAR_FEATURE 0x01
#define USB_SET_FEATURE 0x03
#define USB_SET_ADDRESS 0x05
#define USB_GET_DESCRIPTOR 0x06
#define USB_SET_DESCRIPTOR 0x07
#define USB_GET_CONFIGURATION 0x08
#define USB_SET_CONFIGURATION 0x09
#define USB_GET_INTERFACE 0x0A
#define USB_SET_INTERFACE 0x0B
#define USB_SYNCH_FRAME 0x0C
#endif

/* USB HID class request code */
#ifndef HID_GET_REPORT
#define HID_GET_REPORT 0x01
#define HID_GET_IDLE 0x02
#define HID_GET_PROTOCOL 0x03
#define HID_SET_REPORT 0x09
#define HID_SET_IDLE 0x0A
#define HID_SET_PROTOCOL 0x0B
#endif

/* Bit define for USB request type */
#ifndef USB_REQ_TYP_MASK
#define USB_REQ_TYP_IN 0x80    /* control IN, device to host */
#define USB_REQ_TYP_OUT 0x00   /* control OUT, host to device */
#define USB_REQ_TYP_READ 0x80  /* control read, device to host */
#define USB_REQ_TYP_WRITE 0x00 /* control write, host to device */
#define USB_REQ_TYP_MASK 0x60  /* bit mask of request type */
#define USB_REQ_TYP_STANDARD 0x00
#define USB_REQ_TYP_CLASS 0x20
#define USB_REQ_TYP_VENDOR 0x40
#define USB_REQ_TYP_RESERVED 0x60
#define USB_REQ_RECIP_MASK 0x1F /* bit mask of request recipient */
#define USB_REQ_RECIP_DEVICE 0x00
#define USB_REQ_RECIP_INTERF 0x01
#define USB_REQ_RECIP_ENDP 0x02
#define USB_REQ_RECIP_OTHER 0x03
#endif

/* USB descriptor type */
#ifndef USB_DESCR_TYP_DEVICE
#define USB_DESCR_TYP_DEVICE 0x01
#define USB_DESCR_TYP_CONFIG 0x02
#define USB_DESCR_TYP_STRING 0x03
#define USB_DESCR_TYP_INTERF 0x04
#define USB_DESCR_TYP_ENDP 0x05
#define USB_DESCR_TYP_QUALIF 0x06
#define USB_DESCR_TYP_SPEED 0x07
#define USB_DESCR_TYP_OTG 0x09
#define USB_DESCR_TYP_HID 0x21
#define USB_DESCR_TYP_REPORT 0x22
#define USB_DESCR_TYP_PHYSIC 0x23
#define USB_DESCR_TYP_CS_INTF 0x24
#define USB_DESCR_TYP_CS_ENDP 0x25
#define USB_DESCR_TYP_HUB 0x29
#endif

/* USB device class */
#ifndef USB_DEV_CLASS_HUB
#define USB_DEV_CLASS_RESERVED 0x00
#define USB_DEV_CLASS_AUDIO 0x01
#define USB_DEV_CLASS_COMMUNIC 0x02
#define USB_DEV_CLASS_HID 0x03
#define USB_DEV_CLASS_MONITOR 0x04
#define USB_DEV_CLASS_PHYSIC_IF 0x05
#define USB_DEV_CLASS_POWER 0x06
#define USB_DEV_CLASS_PRINTER 0x07
#define USB_DEV_CLASS_STORAGE 0x08
#define USB_DEV_CLASS_HUB 0x09
#define USB_DEV_CLASS_VEN_SPEC 0xFF
#endif

/* USB endpoint type and attributes */
#ifndef USB_ENDP_TYPE_MASK
#define USB_ENDP_DIR_MASK 0x80
#define USB_ENDP_ADDR_MASK 0x0F
#define USB_ENDP_TYPE_MASK 0x03
#define USB_ENDP_TYPE_CTRL 0x00
#define USB_ENDP_TYPE_ISOCH 0x01
#define USB_ENDP_TYPE_BULK 0x02
#define USB_ENDP_TYPE_INTER 0x03
#endif

#ifndef USB_DEVICE_ADDR
#define USB_DEVICE_ADDR 0x02 /* 默认的USB设备地址 */
#endif
#ifndef DEFAULT_ENDP0_SIZE
#define DEFAULT_ENDP0_SIZE 8 /* default maximum packet size for endpoint 0 */
#endif
#ifndef DEFAULT_ENDP1_SIZE
#define DEFAULT_ENDP1_SIZE 8 /* default maximum packet size for endpoint 1 */
#endif
#ifndef MAX_PACKET_SIZE
#define MAX_PACKET_SIZE 64 /* maximum packet size */
#endif
#ifndef USB_BO_CBW_SIZE
#define USB_BO_CBW_SIZE 0x1F /* 命令块CBW的总长度 */
#define USB_BO_CSW_SIZE 0x0D /* 命令状态块CSW的总长度 */
#endif
#ifndef USB_BO_CBW_SIG0
#define USB_BO_CBW_SIG0 0x55 /* 命令块CBW识别标志'USBC' */
#define USB_BO_CBW_SIG1 0x53
#define USB_BO_CBW_SIG2 0x42
#define USB_BO_CBW_SIG3 0x43
#define USB_BO_CSW_SIG0 0x55 /* 命令状态块CSW识别标志'USBS' */
#define USB_BO_CSW_SIG1 0x53
#define USB_BO_CSW_SIG2 0x42
#define USB_BO_CSW_SIG3 0x53
#endif

typedef struct _USB_SETUP_REQ
{
    uint8_t bRequestType;
    uint8_t bRequest;
    uint8_t wValueL;
    uint8_t wValueH;
    uint8_t wIndexL;
    uint8_t wIndexH;
    uint8_t wLengthL;
    uint8_t wLengthH;
} USB_SETUP_REQ, *PUSB_SETUP_REQ;

typedef __xdata USB_SETUP_REQ *PXUSB_SETUP_REQ;

typedef struct _USB_DEVICE_DESCR
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bcdUSBL;
    uint8_t bcdUSBH;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint8_t idVendorL;
    uint8_t idVendorH;
    uint8_t idProductL;
    uint8_t idProductH;
    uint8_t bcdDeviceL;
    uint8_t bcdDeviceH;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} USB_DEV_DESCR, *PUSB_DEV_DESCR;

typedef __xdata USB_DEV_DESCR *PXUSB_DEV_DESCR;

typedef struct _USB_CONFIG_DESCR
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t wTotalLengthL;
    uint8_t wTotalLengthH;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} USB_CFG_DESCR, *PUSB_CFG_DESCR;

typedef __xdata USB_CFG_DESCR *PXUSB_CFG_DESCR;

typedef struct _USB_INTERF_DESCR
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} USB_ITF_DESCR, *PUSB_ITF_DESCR;

typedef __xdata USB_ITF_DESCR *PXUSB_ITF_DESCR;

typedef struct _USB_ENDPOINT_DESCR
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint8_t wMaxPacketSizeL;
    uint8_t wMaxPacketSizeH;
    uint8_t bInterval;
} USB_ENDP_DESCR, *PUSB_ENDP_DESCR;

typedef __xdata USB_ENDP_DESCR *PXUSB_ENDP_DESCR;

typedef struct _USB_CONFIG_DESCR_LONG
{
    USB_CFG_DESCR cfg_descr;
    USB_ITF_DESCR itf_descr;
    USB_ENDP_DESCR endp_descr[1];
} USB_CFG_DESCR_LONG, *PUSB_CFG_DESCR_LONG;

typedef __xdata USB_CFG_DESCR_LONG *PXUSB_CFG_DESCR_LONG;

typedef struct _USB_HID_DESCR
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bcdHIDL;
    uint8_t bcdHIDH;
    uint8_t bCountryCode;
    uint8_t bNumDescriptors;
    uint8_t bDescriptorTypeX;
    uint8_t wDescriptorLengthL;
    uint8_t wDescriptorLengthH;
} USB_HID_DESCR, *PUSB_HID_DESCR;

typedef __xdata USB_HID_DESCR *PXUSB_HID_DESCR;

typedef struct _UDISK_BOC_CBW
{ /* command of BulkOnly USB-FlashDisk */
    uint8_t mCBW_Sig0;
    uint8_t mCBW_Sig1;
    uint8_t mCBW_Sig2;
    uint8_t mCBW_Sig3;
    uint8_t mCBW_Tag0;
    uint8_t mCBW_Tag1;
    uint8_t mCBW_Tag2;
    uint8_t mCBW_Tag3;
    uint8_t mCBW_DataLen0;
    uint8_t mCBW_DataLen1;
    uint8_t mCBW_DataLen2;
    uint8_t mCBW_DataLen3; /* uppest byte of data length, always is 0 */
    uint8_t mCBW_Flag;     /* transfer direction and etc. */
    uint8_t mCBW_LUN;
    uint8_t mCBW_CB_Len;     /* length of command block */
    uint8_t mCBW_CB_Buf[16]; /* command block buffer */
} UDISK_BOC_CBW, *PUDISK_BOC_CBW;

typedef __xdata UDISK_BOC_CBW *PXUDISK_BOC_CBW;

typedef struct _UDISK_BOC_CSW
{ /* status of BulkOnly USB-FlashDisk */
    uint8_t mCSW_Sig0;
    uint8_t mCSW_Sig1;
    uint8_t mCSW_Sig2;
    uint8_t mCSW_Sig3;
    uint8_t mCSW_Tag0;
    uint8_t mCSW_Tag1;
    uint8_t mCSW_Tag2;
    uint8_t mCSW_Tag3;
    uint8_t mCSW_Residue0; /* return: remainder bytes */
    uint8_t mCSW_Residue1;
    uint8_t mCSW_Residue2;
    uint8_t mCSW_Residue3; /* uppest byte of remainder length, always is 0 */
    uint8_t mCSW_Status;   /* return: result status */
} UDISK_BOC_CSW, *PUDISK_BOC_CSW;

typedef __xdata UDISK_BOC_CSW *PXUDISK_BOC_CSW;

#endif // __USB_DEF__
