#include "FLASH.h"

/* Private function prototypes */
static FlashDrv_Status_t FlashDrv_WaitForLastOperation(void);
static uint8 FlashDrv_IsValidAddress(uint32 Address);

/**
  * @brief  Unlocks the FLASH control register access
  */
void FlashDrv_Unlock(void)
{
    if (FLASH_CR & FLASH_CR_LOCK) {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
    }
}

/**
  * @brief  Locks the FLASH control register access
  */
void FlashDrv_Lock(void)
{
    FLASH_CR |= FLASH_CR_LOCK;
}

/**
  * @brief  Checks if FLASH is locked
  * @return 1 if locked, 0 if unlocked
  */
uint8 FlashDrv_IsLocked(void)
{
    return (FLASH_CR & FLASH_CR_LOCK) ? 1 : 0;
}

/**
  * @brief  Clears all FLASH error flags
  */
void FlashDrv_ClearErrors(void)
{
    FLASH_SR |= FLASH_SR_EOP | FLASH_SR_ERRORS_MASK;
}

/**
  * @brief  Waits for a FLASH operation to complete
  * @return FlashDrv_Status_t status
  */
static FlashDrv_Status_t FlashDrv_WaitForLastOperation(void)
{
    uint32 timeout = FLASH_DRV_TIMEOUT;
    
    /* Wait for busy flag to be cleared */
    while ((FLASH_SR & FLASH_SR_BSY) && (timeout > 0)) {
        timeout--;
    }
    
    if (timeout == 0) {
        return FLASH_DRV_TIMEOUT_ERROR;
    }
    
    /* Check for errors */
    if (FLASH_SR & FLASH_SR_ERRORS_MASK) {
        return FLASH_DRV_ERROR;
    }
    
    return FLASH_DRV_OK;
}

/**
  * @brief  Checks if address is within valid flash range
  * @param  Address: Address to check
  * @return 1 if valid, 0 if invalid
  */
static uint8 FlashDrv_IsValidAddress(uint32 Address)
{
    return ((Address >= FLASH_BASE_ADDR) && (Address < FLASH_END_ADDR)) ? 1 : 0;
}

/**
  * @brief  Gets sector number for a given address
  * @param  Address: Address in flash memory
  * @return Sector number (0-7)
  */
uint32 FlashDrv_GetSector(uint32 Address)
{
    if (Address < SECTOR1_BASE) return 0;
    if (Address < SECTOR2_BASE) return 1;
    if (Address < SECTOR3_BASE) return 2;
    if (Address < SECTOR4_BASE) return 3;
    if (Address < SECTOR5_BASE) return 4;
    if (Address < SECTOR6_BASE) return 5;
    if (Address < SECTOR7_BASE) return 6;
    return 7;
}

/**
  * @brief  Gets base address of a sector
  * @param  SectorNumber: Sector number (0-7)
  * @return Base address of the sector
  */
uint32 FlashDrv_GetSectorBase(uint32 SectorNumber)
{
    switch (SectorNumber) {
        case 0: return SECTOR0_BASE;
        case 1: return SECTOR1_BASE;
        case 2: return SECTOR2_BASE;
        case 3: return SECTOR3_BASE;
        case 4: return SECTOR4_BASE;
        case 5: return SECTOR5_BASE;
        case 6: return SECTOR6_BASE;
        case 7: return SECTOR7_BASE;
        default: return 0;
    }
}

/**
  * @brief  Gets size of a sector
  * @param  SectorNumber: Sector number (0-7)
  * @return Size of the sector in bytes
  */
uint32 FlashDrv_GetSectorSize(uint32 SectorNumber)
{
    switch (SectorNumber) {
        case 0: return SECTOR0_SIZE;
        case 1: return SECTOR1_SIZE;
        case 2: return SECTOR2_SIZE;
        case 3: return SECTOR3_SIZE;
        case 4: return SECTOR4_SIZE;
        case 5: return SECTOR5_SIZE;
        case 6: return SECTOR6_SIZE;
        case 7: return SECTOR7_SIZE;
        default: return 0;
    }
}

/**
  * @brief  Erases a specified FLASH sector
  * @param  SectorNumber: The sector number to be erased (0-7)
  * @return FlashDrv_Status_t status
  */
FlashDrv_Status_t FlashDrv_EraseSector(uint32 SectorNumber)
{
    FlashDrv_Status_t status;
    
    if (SectorNumber > 7) {
        return FLASH_DRV_INVALID_ADDRESS;
    }
    
    FlashDrv_Unlock();
    FlashDrv_ClearErrors();
    
    /* Wait for any ongoing operation */
    status = FlashDrv_WaitForLastOperation();
    if (status != FLASH_DRV_OK) {
        FlashDrv_Lock();
        return status;
    }
    
    /* Configure sector erase */
    FLASH_CR &= ~FLASH_CR_SNB_Msk;
    FLASH_CR |= (SectorNumber << FLASH_CR_SNB_Pos);
    FLASH_CR |= FLASH_CR_SER;
    
    /* Start erase operation */
    FLASH_CR |= FLASH_CR_STRT;
    
    /* Wait for operation to complete */
    status = FlashDrv_WaitForLastOperation();
    
    /* Clear SER bit */
    FLASH_CR &= ~FLASH_CR_SER;
    
    FlashDrv_Lock();
    return status;
}

/**
  * @brief  Erases all sectors in the specified range
  * @param  startAddress: Starting address
  * @param  length: Number of bytes to erase
  * @return FlashDrv_Status_t status
  */
FlashDrv_Status_t FlashDrv_EraseRange(uint32 startAddress, uint32 length)
{
    uint32 endAddress = startAddress + length - 1;
    uint32 currentAddress = startAddress;
    FlashDrv_Status_t status;
    
    if (!FlashDrv_IsValidAddress(startAddress) || 
        !FlashDrv_IsValidAddress(endAddress)) {
        return FLASH_DRV_INVALID_ADDRESS;
    }
    
    while (currentAddress <= endAddress) {
        uint32 sector = FlashDrv_GetSector(currentAddress);
        status = FlashDrv_EraseSector(sector);
        if (status != FLASH_DRV_OK) {
            return status;
        }
        /* Move to next sector */
        currentAddress = FlashDrv_GetSectorBase(sector) + FlashDrv_GetSectorSize(sector);
    }
    
    return FLASH_DRV_OK;
}

/**
  * @brief  Performs a mass erase of entire flash
  * @return FlashDrv_Status_t status
  */
FlashDrv_Status_t FlashDrv_MassErase(void)
{
    FlashDrv_Status_t status;
    
    FlashDrv_Unlock();
    FlashDrv_ClearErrors();
    
    /* Wait for any ongoing operation */
    status = FlashDrv_WaitForLastOperation();
    if (status != FLASH_DRV_OK) {
        FlashDrv_Lock();
        return status;
    }
    
    /* Set mass erase bit */
    FLASH_CR |= FLASH_CR_MER;
    
    /* Start erase operation */
    FLASH_CR |= FLASH_CR_STRT;
    
    /* Wait for operation to complete */
    status = FlashDrv_WaitForLastOperation();
    
    /* Clear MER bit */
    FLASH_CR &= ~FLASH_CR_MER;
    
    FlashDrv_Lock();
    return status;
}

/**
  * @brief  Programs a byte to FLASH
  * @param  Address: Address to program
  * @param  Data: Byte data to program
  * @return FlashDrv_Status_t status
  */
FlashDrv_Status_t FlashDrv_ProgramByte(uint32 Address, uint8 Data)
{
    FlashDrv_Status_t status;
    
    if (!FlashDrv_IsValidAddress(Address)) {
        return FLASH_DRV_INVALID_ADDRESS;
    }
    
    FlashDrv_Unlock();
    FlashDrv_ClearErrors();
    
    /* Wait for any ongoing operation */
    status = FlashDrv_WaitForLastOperation();
    if (status != FLASH_DRV_OK) {
        FlashDrv_Lock();
        return status;
    }
    
    /* Set programming size to 8 bits */
    FLASH_CR &= ~FLASH_CR_PSIZE_Msk;
    FLASH_CR |= (FLASH_PSIZE_BYTE << FLASH_CR_PSIZE_Pos);
    
    /* Enable programming */
    FLASH_CR |= FLASH_CR_PG;
    
    /* Program the data */
    *((volatile uint8*)Address) = Data;
    
    /* Wait for completion */
    status = FlashDrv_WaitForLastOperation();
    
    /* Disable programming */
    FLASH_CR &= ~FLASH_CR_PG;
    
    FlashDrv_Lock();
    return status;
}

/**
  * @brief  Programs a half-word (16-bit) to FLASH
  * @param  Address: Address to program (must be 2-byte aligned)
  * @param  Data: Half-word data to program
  * @return FlashDrv_Status_t status
  */
FlashDrv_Status_t FlashDrv_ProgramHalfWord(uint32 Address, uint16 Data)
{
    FlashDrv_Status_t status;
    
    if (!FlashDrv_IsValidAddress(Address)) {
        return FLASH_DRV_INVALID_ADDRESS;
    }
    
    if (Address & 0x1U) {
        return FLASH_DRV_ALIGNMENT_ERROR;
    }
    
    FlashDrv_Unlock();
    FlashDrv_ClearErrors();
    
    /* Wait for any ongoing operation */
    status = FlashDrv_WaitForLastOperation();
    if (status != FLASH_DRV_OK) {
        FlashDrv_Lock();
        return status;
    }
    
    /* Set programming size to 16 bits */
    FLASH_CR &= ~FLASH_CR_PSIZE_Msk;
    FLASH_CR |= (FLASH_PSIZE_HALF_WORD << FLASH_CR_PSIZE_Pos);
    
    /* Enable programming */
    FLASH_CR |= FLASH_CR_PG;
    
    /* Program the data */
    *((volatile uint16*)Address) = Data;
    
    /* Wait for completion */
    status = FlashDrv_WaitForLastOperation();
    
    /* Disable programming */
    FLASH_CR &= ~FLASH_CR_PG;
    
    FlashDrv_Lock();
    return status;
}

/**
  * @brief  Programs a word (32-bit) to FLASH
  * @param  Address: Address to program (must be 4-byte aligned)
  * @param  Data: Word data to program
  * @return FlashDrv_Status_t status
  */
FlashDrv_Status_t FlashDrv_ProgramWord(uint32 Address, uint32 Data)
{
    FlashDrv_Status_t status;
    
    if (!FlashDrv_IsValidAddress(Address)) {
        return FLASH_DRV_INVALID_ADDRESS;
    }
    
    if (Address & 0x3U) {
        return FLASH_DRV_ALIGNMENT_ERROR;
    }
    
    FlashDrv_Unlock();
    FlashDrv_ClearErrors();
    
    /* Wait for any ongoing operation */
    status = FlashDrv_WaitForLastOperation();
    if (status != FLASH_DRV_OK) {
        FlashDrv_Lock();
        return status;
    }
    
    /* Set programming size to 32 bits */
    FLASH_CR &= ~FLASH_CR_PSIZE_Msk;
    FLASH_CR |= (FLASH_PSIZE_WORD << FLASH_CR_PSIZE_Pos);
    
    /* Enable programming */
    FLASH_CR |= FLASH_CR_PG;
    
    /* Program the data */
    *((volatile uint32*)Address) = Data;
    
    /* Wait for completion */
    status = FlashDrv_WaitForLastOperation();
    
    /* Disable programming */
    FLASH_CR &= ~FLASH_CR_PG;
    
    FlashDrv_Lock();
    return status;
}


/**
  * @brief  Programs a buffer to FLASH (handles unaligned data)
  * @param  Address: Starting address to program
  * @param  buffer: Pointer to data buffer
  * @param  length: Number of bytes to program
  * @return FlashDrv_Status_t status
  */
FlashDrv_Status_t FlashDrv_ProgramBuffer(uint32 Address, const uint8 *buffer, uint32 length)
{
    FlashDrv_Status_t status;
    uint32 i = 0;
    
    if (!FlashDrv_IsValidAddress(Address) || 
        !FlashDrv_IsValidAddress(Address + length - 1)) {
        return FLASH_DRV_INVALID_ADDRESS;
    }
    
    /* Check if area is erased before programming */
    if (!FlashDrv_IsErased(Address, length)) {
        return FLASH_DRV_NOT_ERASED;
    }
    
    /* Program byte by byte for maximum compatibility */
    while (i < length) {
        status = FlashDrv_ProgramByte(Address + i, buffer[i]);
        if (status != FLASH_DRV_OK) {
            return status;
        }
        i++;
    }
    
    return FLASH_DRV_OK;
}

/**
  * @brief  Programs a buffer to FLASH with optimal alignment
  * @param  Address: Starting address to program (should be word aligned)
  * @param  buffer: Pointer to data buffer
  * @param  length: Number of bytes to program
  * @return FlashDrv_Status_t status
  */
FlashDrv_Status_t FlashDrv_ProgramBufferAligned(uint32 Address, const uint8 *buffer, uint32 length)
{
    FlashDrv_Status_t status;
    uint32 i = 0;
    
    if (!FlashDrv_IsValidAddress(Address) || 
        !FlashDrv_IsValidAddress(Address + length - 1)) {
        return FLASH_DRV_INVALID_ADDRESS;
    }
    
    if (Address & 0x3U) {
        return FLASH_DRV_ALIGNMENT_ERROR;
    }
    
    /* Check if area is erased before programming */
    if (!FlashDrv_IsErased(Address, length)) {
        return FLASH_DRV_NOT_ERASED;
    }
    
    /* Program words first */
    while (i + 4 <= length) {
        uint32 word_data = *(uint32*)(buffer + i);
        status = FlashDrv_ProgramWord(Address + i, word_data);
        if (status != FLASH_DRV_OK) {
            return status;
        }
        i += 4;
    }
    
    /* Program remaining bytes */
    while (i < length) {
        status = FlashDrv_ProgramByte(Address + i, buffer[i]);
        if (status != FLASH_DRV_OK) {
            return status;
        }
        i++;
    }
    
    return FLASH_DRV_OK;
}

/**
  * @brief  Verifies programmed data against buffer
  * @param  Address: Starting address to verify
  * @param  buffer: Pointer to expected data
  * @param  length: Number of bytes to verify
  * @return 0 if successful, -1 if verification failed
  */
int FlashDrv_Verify(uint32 Address, const uint8 *buffer, uint32 length)
{
    const uint8 *flash_ptr = (const uint8*)Address;
    
    for (uint32 i = 0; i < length; i++) {
        if (flash_ptr[i] != buffer[i]) {
            return -1;  /* Verification failed */
        }
    }
    
    return 0;  /* Verification successful */
}

/**
  * @brief  Checks if flash region is erased (all 0xFF)
  * @param  Address: Starting address
  * @param  length: Number of bytes to check
  * @return 1 if erased, 0 if not erased
  */
uint8 FlashDrv_IsErased(uint32 Address, uint32 length)
{
    const uint8 *flash_ptr = (const uint8*)Address;
    
    for (uint32 i = 0; i < length; i++) {
        if (flash_ptr[i] != 0xFF) {
            return 0;  /* Not erased */
        }
    }
    
    return 1;  /* Erased */
}

/**
  * @brief  Reads data from flash to buffer
  * @param  Address: Starting address to read from
  * @param  buffer: Pointer to destination buffer
  * @param  length: Number of bytes to read
  * @return FlashDrv_Status_t status
  */
FlashDrv_Status_t FlashDrv_Read(uint32 Address, uint8 *buffer, uint32 length)
{
    const uint8 *flash_ptr = (const uint8*)Address;
    
    if (!FlashDrv_IsValidAddress(Address) || 
        !FlashDrv_IsValidAddress(Address + length - 1)) {
        return FLASH_DRV_INVALID_ADDRESS;
    }
    
    for (uint32 i = 0; i < length; i++) {
        buffer[i] = flash_ptr[i];
    }
    
    return FLASH_DRV_OK;
}

/**
  * @brief  Prepares for write operation (disables interrupts)
  */
void FlashDrv_PrepareForWrite(void)
{
    __asm volatile ("cpsid i"); /* Disable interrupts */
}

/**
  * @brief  Finishes write operation (enables interrupts)
  */
void FlashDrv_FinishWrite(void)
{
    __asm volatile ("cpsie i"); /* Enable interrupts */
}

/**
  * @brief  Safe word programming with interrupt protection
  * @param  Address: Address to program
  * @param  Data: Word data to program
  * @return FlashDrv_Status_t status
  */
FlashDrv_Status_t FlashDrv_ProgramWordSafe(uint32 Address, uint32 Data)
{
    FlashDrv_Status_t status;
    
    FlashDrv_PrepareForWrite();
    status = FlashDrv_ProgramWord(Address, Data);
    FlashDrv_FinishWrite();
    
    return status;
}