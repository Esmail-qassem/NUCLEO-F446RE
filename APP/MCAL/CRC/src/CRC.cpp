/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : CRC                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "CRC.hpp"
#include "RCC.hpp"

void CRC::Init(void)
{
    RCC::EnableClock(RCC_Bus::AHB1, RCC_Peripheral::CRC);
    Reset();
}

void CRC::Reset(void)
{
    CRC_REGS->CR = (1u << 0u);
}

uint32 CRC::AccumulateWord(uint32 word)
{
    CRC_REGS->DR = word;
    return CRC_REGS->DR;
}

uint32 CRC::Calculate(const uint8 *buf, uint32 len)
{
    if (!buf) return 0u;
    Reset();
    uint32 full  = len / 4u;
    uint32 rem   = len % 4u;

    for (uint32 i = 0u; i < full; i++) {
        uint32 w = (static_cast<uint32>(buf[i*4u+0u]) << 24u)
                 | (static_cast<uint32>(buf[i*4u+1u]) << 16u)
                 | (static_cast<uint32>(buf[i*4u+2u]) <<  8u)
                 |  static_cast<uint32>(buf[i*4u+3u]);
        CRC_REGS->DR = w;
    }
    if (rem > 0u) {
        uint32 word = 0u;
        uint32 base = full * 4u;
        switch (rem) {
            case 3u: word |= static_cast<uint32>(buf[base+2u]) <<  8u; /* fall */
            case 2u: word |= static_cast<uint32>(buf[base+1u]) << 16u; /* fall */
            case 1u: word |= static_cast<uint32>(buf[base+0u]) << 24u; break;
            default: break;
        }
        CRC_REGS->DR = word;
    }
    return CRC_REGS->DR;
}

uint8 CRC::Verify(const uint8 *buf, uint32 total_len)
{
    if (!buf || total_len < 5u) return CRC_ERR;
    uint32 payload = total_len - 4u;
    uint32 expected = (static_cast<uint32>(buf[payload+0u]) << 24u)
                    | (static_cast<uint32>(buf[payload+1u]) << 16u)
                    | (static_cast<uint32>(buf[payload+2u]) <<  8u)
                    |  static_cast<uint32>(buf[payload+3u]);
    uint32 computed = Calculate(buf, payload);
    return (computed == expected) ? CRC_OK : CRC_ERR;
}

uint32 CRC::GetResult(void)
{
    return CRC_REGS->DR;
}
