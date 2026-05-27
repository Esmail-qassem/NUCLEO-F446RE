/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : Bootloader Parser                                      */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "Parse.hpp"
#include "STD_TYPES.h"

/*===========================================================================
                        GLOBAL VARIABLES
===========================================================================*/
uint8  UART_START  = 0U;
static uint8 flash_done = 0U;
extern uint32 ms_ticks;
constexpr uint32 UART_TIMEOUT_MS = 20000U;
uint32 ImageSize     = 0U;
uint32 MagicNumber   = 0U;
uint32 received_crc  = 0U;
uint32 crc32_table[256];
uint8  crc32_table_ready = 0U;

/*===========================================================================
                        BIN MODE VARIABLES
===========================================================================*/
constexpr uint32 APP_START_ADDRESS = 0x08008000UL;
constexpr uint32 BIN_BUFFER_SIZE   = 40900U;
volatile uint8  bin_buffer[BIN_BUFFER_SIZE];
volatile uint32 bin_index   = 0U;
uint32 flash_address = APP_START_ADDRESS;
uint8  Start_Flashing = 0U;

/*===========================================================================
                        CRC32 LOOKUP TABLE (zlib compatible)
===========================================================================*/
void CRC32_Init(void)
{
    for (uint32 i = 0U; i < 256U; i++)
    {
        uint32 c = i;
        for (uint8 j = 0U; j < 8U; j++)
        {
            if (c & 1U) c = 0xEDB88320UL ^ (c >> 1U);
            else        c >>= 1U;
        }
        crc32_table[i] = c;
    }
    crc32_table_ready = 1U;
}

static uint32 CRC32_Calculate(const uint8 *data, uint32 length)
{
    uint32 crc = 0xFFFFFFFFUL;
    for (uint32 i = 0U; i < length; i++)
        crc = (crc >> 8U) ^ crc32_table[(crc ^ data[i]) & 0xFFU];
    return crc ^ 0xFFFFFFFFUL;
}

/*===========================================================================
                        BOOTLOADER ISR HANDLER
===========================================================================*/
static uint8 sync_received = 0U;
static uint8 retry_count   = 0U;

constexpr uint8 BTLD_NAK   = 0xFAU;
constexpr uint8 BTLD_ACK   = 0xACU;
constexpr uint8 BTLD_FATAL = 0xFBU;
constexpr uint8 MAX_RETRY  = 3U;

extern "C" void BootLoader_Handler(uint8 byte)
{
    if (sync_received == 0U)
    {
        if (byte == 0x55U)
        {
            sync_received = 1U;
            retry_count   = 0U;
        }
        return;
    }
    UART_START = 1U;
    ms_ticks   = 0U;
    if (bin_index < BIN_BUFFER_SIZE)
        bin_buffer[bin_index++] = byte;
}

/*===========================================================================
                        BOOTLOADER MAIN FUNCTION
===========================================================================*/
void BootLoader_MainFunction(void)
{
    /* Magic number */
    if (bin_index == 4U)
    {
        MagicNumber = (static_cast<uint32>(bin_buffer[3]) << 24U) |
                      (static_cast<uint32>(bin_buffer[2]) << 16U) |
                      (static_cast<uint32>(bin_buffer[1]) <<  8U) |
                      (static_cast<uint32>(bin_buffer[0]));
    }
    /* CRC32 */
    if (bin_index == 8U)
    {
        received_crc = (static_cast<uint32>(bin_buffer[7]) << 24U) |
                       (static_cast<uint32>(bin_buffer[6]) << 16U) |
                       (static_cast<uint32>(bin_buffer[5]) <<  8U) |
                       (static_cast<uint32>(bin_buffer[4]));
    }
    /* Image size */
    if (bin_index == 12U)
    {
        ImageSize = (static_cast<uint32>(bin_buffer[11]) << 24U) |
                    (static_cast<uint32>(bin_buffer[10]) << 16U) |
                    (static_cast<uint32>(bin_buffer[ 9]) <<  8U) |
                    (static_cast<uint32>(bin_buffer[ 8]));
    }

    if (bin_index == (ImageSize + 12U) && Start_Flashing == 0U)
    {
        uint32 calculated_crc = CRC32_Calculate(
            const_cast<const uint8*>(reinterpret_cast<volatile uint8*>(bin_buffer + 12)), ImageSize);

        if (calculated_crc == received_crc)
        {
            UART::SendSyncBuffer(UART_HardWare::UART2,
                                 reinterpret_cast<const uint8*>("\nCRC OK!\n"), 9U);
            FlashDrv::EraseSector(2U);
            FlashDrv::EraseSector(3U);
            Start_Flashing = 1U;
        }
        else
        {
            retry_count++;
            UART::SendSyncBuffer(UART_HardWare::UART2,
                                 reinterpret_cast<const uint8*>("\nCRC FAILED! Attempt "), 21U);
            UART::SendNumber(UART_HardWare::UART2, static_cast<sint32>(retry_count));
            UART::SendSyncBuffer(UART_HardWare::UART2,
                                 reinterpret_cast<const uint8*>("/3\n"), 3U);

            if (retry_count >= MAX_RETRY)
            {
                uint8 fatal = BTLD_FATAL;
                UART::SendSyncBuffer(UART_HardWare::UART1, &fatal, 1U);
                UART::SendSyncBuffer(UART_HardWare::UART2,
                                     reinterpret_cast<const uint8*>("FATAL: max retries - aborting\n"), 30U);
                retry_count   = 0U;
                sync_received = 0U;
            }
            else
            {
                uint8 nak = BTLD_NAK;
                UART::SendSyncBuffer(UART_HardWare::UART1, &nak, 1U);
            }
            ImageSize      = 0U;
            bin_index      = 0U;
            received_crc   = 0U;
            Start_Flashing = 0U;
            flash_done     = 0U;
            flash_address  = APP_START_ADDRESS;
        }
    }

    if (Start_Flashing == 1U && flash_done == 0U)
    {
        __asm volatile ("CPSID I");
        FlashDrv::ProgramBufferAligned(flash_address,
            const_cast<const uint8*>(reinterpret_cast<volatile uint8*>(bin_buffer + 12)), ImageSize);
        __asm volatile ("CPSIE I");
        flash_done = 1U;
    }

    if (flash_done == 1U && UART_START == 1U)
    {
        uint32 stack_ptr     = *reinterpret_cast<volatile uint32*>(0x08008000UL);
        uint32 reset_handler = *reinterpret_cast<volatile uint32*>(0x08008004UL);

        if ((stack_ptr >= 0x20000000UL && stack_ptr <= 0x20020000UL) &&
            (reset_handler >= 0x08008000UL && reset_handler <= 0x08080000UL))
        {
            uint8 ack = BTLD_ACK;
            UART::SendSyncBuffer(UART_HardWare::UART1, &ack, 1U);
            UART::SendSyncBuffer(UART_HardWare::UART2,
                                 reinterpret_cast<const uint8*>("\nVerification OK!\n"), 18U);
            retry_count    = 0U;
            flash_address  = APP_START_ADDRESS;
            sync_received  = 0U;
            received_crc   = 0U;
            Start_Flashing = 0U;
            flash_done     = 0U;
            bin_index      = 0U;
            ImageSize      = 0U;
            /* System reset */
            *reinterpret_cast<volatile uint32*>(0xE000ED0CUL) = 0x5FA0004UL;
        }
        else
        {
            retry_count++;
            UART::SendSyncBuffer(UART_HardWare::UART2,
                                 reinterpret_cast<const uint8*>("\nVerification FAILED! Attempt "), 30U);
            UART::SendNumber(UART_HardWare::UART2, static_cast<sint32>(retry_count));
            UART::SendSyncBuffer(UART_HardWare::UART2,
                                 reinterpret_cast<const uint8*>("/3\n"), 3U);

            if (retry_count >= MAX_RETRY)
            {
                uint8 fatal = BTLD_FATAL;
                UART::SendSyncBuffer(UART_HardWare::UART1, &fatal, 1U);
                UART::SendSyncBuffer(UART_HardWare::UART2,
                                     reinterpret_cast<const uint8*>("FATAL: max retries - aborting\n"), 30U);
                retry_count = 0U;
            }
            else
            {
                uint8 nak = BTLD_NAK;
                UART::SendSyncBuffer(UART_HardWare::UART1, &nak, 1U);
            }
            received_crc   = 0U;
            sync_received  = 0U;
            Start_Flashing = 0U;
            flash_done     = 0U;
            bin_index      = 0U;
            ImageSize      = 0U;
            flash_address  = APP_START_ADDRESS;
        }
        UART_START = 0U;
    }

    /* Timeout: no bytes for 20 seconds → reset */
    if (ms_ticks > UART_TIMEOUT_MS)
    {
        *reinterpret_cast<volatile uint32*>(0xE000ED0CUL) = 0x5FA0004UL;
    }
}

/* Stub implementations for hex-mode functions (not used in BIN mode) */
uint8 parseByte(uint8 high, uint8 low)
{
    (void)high; (void)low; return 0U;
}

uint8 processRecord(uint8 *recordBuffer)
{
    (void)recordBuffer; return 0U;
}
