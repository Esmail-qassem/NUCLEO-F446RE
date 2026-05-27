#include "GPIO.hpp"

GPIO_RegDef_t* GPIO::GetPort(GPIO_Port port)
{
    switch (port)
    {
        case GPIO_Port::A: return reinterpret_cast<GPIO_RegDef_t*>(GPIOA_BASE);
        case GPIO_Port::B: return reinterpret_cast<GPIO_RegDef_t*>(GPIOB_BASE);
        case GPIO_Port::C: return reinterpret_cast<GPIO_RegDef_t*>(GPIOC_BASE);
        case GPIO_Port::D: return reinterpret_cast<GPIO_RegDef_t*>(GPIOD_BASE);
        case GPIO_Port::E: return reinterpret_cast<GPIO_RegDef_t*>(GPIOE_BASE);
        case GPIO_Port::F: return reinterpret_cast<GPIO_RegDef_t*>(GPIOF_BASE);
        case GPIO_Port::G: return reinterpret_cast<GPIO_RegDef_t*>(GPIOG_BASE);
        case GPIO_Port::H: return reinterpret_cast<GPIO_RegDef_t*>(GPIOH_BASE);
        default:           return nullptr;
    }
}

GPIO_Status GPIO::InitPin(GPIO_Port port, GPIO_Pin pin,
                           GPIO_Mode mode, GPIO_OType otype,
                           GPIO_Speed speed, GPIO_Pull pull)
{
    GPIO_RegDef_t* GPIOx = GetPort(port);
    if (!GPIOx) return GPIO_Status::WrongPort;

    uint8 p = static_cast<uint8>(pin);
    if (p > 15u) return GPIO_Status::WrongPin;

    GPIOx->MODER &= ~(0b11u << (p * 2u));
    GPIOx->MODER |=  (static_cast<uint32>(mode) & 0b11u) << (p * 2u);

    if (mode == GPIO_Mode::OUTPUT || mode == GPIO_Mode::AF)
    {
        GPIOx->OTYPER &= ~(1u << p);
        GPIOx->OTYPER |=  static_cast<uint32>(otype) << p;
    }

    GPIOx->OSPEEDR &= ~(0b11u << (p * 2u));
    GPIOx->OSPEEDR |=  (static_cast<uint32>(speed) & 0b11u) << (p * 2u);

    GPIOx->PUPDR &= ~(0b11u << (p * 2u));
    GPIOx->PUPDR |=  (static_cast<uint32>(pull) & 0b11u) << (p * 2u);

    return GPIO_Status::OK;
}

GPIO_Status GPIO::WritePin(GPIO_Port port, GPIO_Pin pin, uint8 value)
{
    GPIO_RegDef_t* GPIOx = GetPort(port);
    if (!GPIOx) return GPIO_Status::WrongPort;

    uint8 p = static_cast<uint8>(pin);
    if (p > 15u) return GPIO_Status::WrongPin;

    if (value == GPIO_HIGH)
        GPIOx->BSRR = (1u << p);
    else
        GPIOx->BSRR = (1u << (p + 16u));

    return GPIO_Status::OK;
}

GPIO_Status GPIO::TogglePin(GPIO_Port port, GPIO_Pin pin)
{
    GPIO_RegDef_t* GPIOx = GetPort(port);
    if (!GPIOx) return GPIO_Status::WrongPort;

    uint8 p = static_cast<uint8>(pin);
    if (p > 15u) return GPIO_Status::WrongPin;

    TOGGLE_BIT(GPIOx->ODR, p);
    return GPIO_Status::OK;
}

GPIO_Status GPIO::ReadPin(GPIO_Port port, GPIO_Pin pin, uint8 &value)
{
    GPIO_RegDef_t* GPIOx = GetPort(port);
    if (!GPIOx) return GPIO_Status::WrongPort;

    uint8 p = static_cast<uint8>(pin);
    if (p > 15u) return GPIO_Status::WrongPin;

    value = static_cast<uint8>(GET_BIT(GPIOx->IDR, p));
    return GPIO_Status::OK;
}

void GPIO::SetAF(GPIO_Port port, uint8 pin, uint8 AF)
{
    GPIO_RegDef_t* GPIOx = GetPort(port);
    if (!GPIOx) return;

    if (pin < 8u)
    {
        uint8 shift = pin * 4u;
        GPIOx->AFRL &= ~(0xFu << shift);
        GPIOx->AFRL |=  (AF & 0xFu) << shift;
    }
    else
    {
        uint8 shift = (pin - 8u) * 4u;
        GPIOx->AFRH &= ~(0xFu << shift);
        GPIOx->AFRH |=  (AF & 0xFu) << shift;
    }
}
