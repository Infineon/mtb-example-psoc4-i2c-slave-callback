/******************************************************************************
* File Name:  main.c
*
* Description:  This is the source code for the PSoC 4 MCU I2C Slave Using
*               Callbacks example
*
* Related Document: See Readme.md
*
*******************************************************************************
* (c) 2020, Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* ("Software"), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries ("Cypress") and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software ("EULA").
*
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress's integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer of such
* system or application assumes all risk of such use and in doing so agrees to
* indemnify Cypress against all liability.
*******************************************************************************/


/* Header file includes */
#include "cy_pdl.h"
#include "cybsp.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* Valid command packet size of three bytes */
#define PACKET_SIZE          (0x03u)

/* Master write and read buffer of size three bytes */
#define SL_RD_BUFFER_SIZE    (PACKET_SIZE)
#define SL_WR_BUFFER_SIZE    (PACKET_SIZE)

/* Start and end of packet markers */
#define PACKET_SOP           (0x01u)
#define PACKET_EOP           (0x17u)

/* Command valid status */
#define STS_CMD_DONE         (0x00u)
#define STS_CMD_FAIL         (0xFFu)

/* Packet positions */
#define PACKET_SOP_POS       (0x00u)
#define PACKET_STS_POS       (0x01u)
#define PACKET_LED_POS       (0x01u)
#define PACKET_EOP_POS       (0x02u)


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void sI2C_InterruptHandler(void);
static void ExecuteCommand(void);
static void SlaveCallback(uint32 event);


/*******************************************************************************
* Global Variables
*******************************************************************************/
/** The instance-specific context structure.
 * It is used by the driver for internal configuration and
 * data keeping for the I2C. Do not modify anything in this structure.
 */
cy_stc_scb_i2c_context_t CYBSP_I2C_context;

/* CYBSP_I2C_SCB_IRQ */
const cy_stc_sysint_t CYBSP_I2C_SCB_IRQ_config = {
        .intrSrc = (IRQn_Type) CYBSP_I2C_IRQ,
        .intrPriority = 3u
};

/* I2C read and write buffers */
uint8_t i2cReadBuffer [SL_RD_BUFFER_SIZE] = {PACKET_SOP, STS_CMD_FAIL,
                                            PACKET_EOP};
uint8_t i2cWriteBuffer[SL_WR_BUFFER_SIZE];


/*******************************************************************************
 * Function Name: main
 *******************************************************************************
 *
 *  The main function performs the following actions:
 *   1. Sets up I2C to be in I2C slave mode.
 *   2. Initializes PWM to control the LED.
 *   3. I2C slave receives packets from master and configures the PWM to
 *      drive the LED.
 *   4. Slave responds with the acknowledgment packet.
 *
 ******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize and enable I2C Component in slave mode.
     * If initialization fails process error
     */
    result = Cy_SCB_I2C_Init(CYBSP_I2C_HW, &CYBSP_I2C_config,
                            &CYBSP_I2C_context);
    if(result != CY_SCB_I2C_SUCCESS)
    {
        CY_ASSERT(0);
    }

    result = Cy_SysInt_Init(&CYBSP_I2C_SCB_IRQ_config, &sI2C_InterruptHandler);
    if(result != CY_SYSINT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Configure read buffer */
    Cy_SCB_I2C_SlaveConfigReadBuf(CYBSP_I2C_HW, i2cReadBuffer,
                                  SL_RD_BUFFER_SIZE, &CYBSP_I2C_context);

    /* Configure write buffer */
    Cy_SCB_I2C_SlaveConfigWriteBuf(CYBSP_I2C_HW, i2cWriteBuffer,
                                   SL_WR_BUFFER_SIZE, &CYBSP_I2C_context);

    /* Register Callback function for interrupt */
    Cy_SCB_I2C_RegisterEventCallback(CYBSP_I2C_HW,
                                    (cy_cb_scb_i2c_handle_events_t) SlaveCallback,
                                    &CYBSP_I2C_context);

    /*  Enable interrupt and I2C block */
    NVIC_EnableIRQ((IRQn_Type) CYBSP_I2C_SCB_IRQ_config.intrSrc);
    Cy_SCB_I2C_Enable(CYBSP_I2C_HW);

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize and enables TCPWM to control red LED. If initialization
     * fails report error.
     */
    result = Cy_TCPWM_PWM_Init(CYBSP_PWM_HW, CYBSP_PWM_NUM, &CYBSP_PWM_config);
    if(result != CY_TCPWM_SUCCESS)
    {
        CY_ASSERT(0);
    }

    Cy_TCPWM_Enable_Multiple(CYBSP_PWM_HW, CYBSP_PWM_MASK);
    Cy_TCPWM_TriggerStart(CYBSP_PWM_HW, CYBSP_PWM_MASK);

    for(;;)
    {

    }
}


/*******************************************************************************
 * Function Name: sI2C_InterruptHandler
 *******************************************************************************
 * Summary:
 *  This function executes interrupt service routine.
 *
 ******************************************************************************/
void sI2C_InterruptHandler(void)
{
    /* ISR implementation for I2C */
    Cy_SCB_I2C_Interrupt(CYBSP_I2C_HW, &CYBSP_I2C_context);
}


/*******************************************************************************
 * Function Name: SlaveCallback
 *******************************************************************************
 *
 * Summary:
 *  Handles slave events write and read completion events.
 *
 * Parameters:
 *  event:  Reports slave events.
 *
 ******************************************************************************/
static void SlaveCallback(uint32_t event)
{
    /* Check write complete event */
    if (0UL != (CY_SCB_I2C_SLAVE_WR_CMPLT_EVENT & event))
    {

        /* Check for errors */
        if (0UL == (CY_SCB_I2C_SLAVE_ERR_EVENT & event))
        {
            /* Check packet length */
            if (PACKET_SIZE == Cy_SCB_I2C_SlaveGetWriteTransferCount(CYBSP_I2C_HW,
                                                                    &CYBSP_I2C_context))
            {
                /* Check start and end of packet markers */
                if ((i2cWriteBuffer[PACKET_SOP_POS] == PACKET_SOP) &&
                    (i2cWriteBuffer[PACKET_EOP_POS] == PACKET_EOP))
                {
                    /* Execute command and update reply status for received
                     * command
                     */
                    ExecuteCommand( );
                    i2cReadBuffer[PACKET_STS_POS] = STS_CMD_DONE;
                }
            }
        }

        /* Configure write buffer for the next write */
        Cy_SCB_I2C_SlaveConfigWriteBuf(CYBSP_I2C_HW, i2cWriteBuffer,
                                       SL_WR_BUFFER_SIZE, &CYBSP_I2C_context);
    }

    /* Check read complete event */
    if (0UL != (CY_SCB_I2C_SLAVE_RD_CMPLT_EVENT & event))
    {
        /* Configure read buffer for the next read */
        i2cReadBuffer[PACKET_STS_POS] = STS_CMD_FAIL;
        Cy_SCB_I2C_SlaveConfigReadBuf(CYBSP_I2C_HW, i2cReadBuffer,
                                      SL_RD_BUFFER_SIZE, &CYBSP_I2C_context);
    }
}


/*******************************************************************************
 * Function Name: ExecuteCommand
 *******************************************************************************
 *
 * Sets the compare value for the LED PWM received by slave.
 *
 ******************************************************************************/
static void ExecuteCommand(void)
{
    /* Sets the compare value to control the brightness of theLED. */
    Cy_TCPWM_PWM_SetCompare0(CYBSP_PWM_HW, CYBSP_PWM_NUM,
                            i2cWriteBuffer[PACKET_LED_POS]);
}


/* [] END OF FILE */
