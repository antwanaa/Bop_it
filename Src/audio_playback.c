#define DEBUG_THIS_FILE     DEBUG_AUDIO_PLAYBACK_FILE

#include <string.h>
#include "main.h"
#include "dac.h"

#include "debug.h"
#include "audio_playback.h"

/* --------- Local Defines ----------- */
// Uncomment to load the audio samples in flash, keep commented for game builds
// #define LOAD_AUDIO

/* Due to compiler dead code elimination the actual array of sample data only gets
loaded if the LOAD_AUDIO macro is defined, otherwise only the **_sample_length variable is used. */
#include "samples/bop_it.h"
#include "samples/twist_it.h"
#include "samples/success.h"
#include "samples/fail.h"
#include "samples/start_part1.h"
#include "samples/start_part2.h"
#include "samples/blow_it.h"
#include "samples/win.h"


// The memory of the Flash when in memory mapped mode
#define QSPI_FLASH_MAPPED_ADDR   (0x90000000)


/* ---------- Local functions ---------- */
inline static void Load_Audio_Sample(uint8_t block, const uint8_t *sample, uint32_t sample_length);
static void QSPI_GetInfo();
static void QSPI_Erase(uint8_t block);
static void QSPI_Read(uint32_t address, uint8_t *buffer, uint32_t length);
static void QSPI_Write(uint32_t address, uint8_t *data, uint32_t length);

static uint32_t block_mapped_address(uint8_t block);

/* 
QSPI Flash chip: MX25R6435F
    - size: 64 Mb (Mbits) = 8MBytes
    - Blocks are 64kB, SubBlock are 32kB, Sectors are 4kB, Pages are 256B
Using one block per sample:
Block 0 -> Sectors  0-15 -> Address: 0x000000 to 0x00FFFF
Block 1 -> Sectors 16-31 -> Address: 0x010000 to 0x01FFFF
Block 2 -> Sectors 32-47 -> Address: 0x020000 to 0x02FFFF
Block 3 -> Sectors 48-63 -> Address: 0x030000 to 0x03FFFF
...

Block 0:   Bop It sample
Block 1:   Twist It sample
Block 2:   Success sample
Block 3:   Fail sample
Block 4-5: Start sample
Block 6:   Blow It sample
Block 7:   Win sample
*/


/* --- QSPI Flash functions --- */

void Audio_Init() {

    debugf("Init QSPI Flash\n");

    BSP_QSPI_DeInit();

    uint8_t ret = BSP_QSPI_Init();
    if (ret != 0) {
        printf("Error: Initialising QSPI Flash\n");
        return;
    }

    // Warning! Takes 50 seconds to do!
    // ret = BSP_QSPI_Erase_Chip();
    // if (ret != 0) {
    //  printf("Error: Erasing QSPI Flash\n");
    //  return;
    // }

    uint8_t status = BSP_QSPI_GetStatus();
    debugf("QSPI status: %d\n", status);

    QSPI_GetInfo();

#ifdef LOAD_AUDIO
    Load_Audio_Sample(0, bop_it_sample, bop_it_sample_length);
    Load_Audio_Sample(1, twist_it_sample, twist_it_sample_length);
    Load_Audio_Sample(2, success_sample, success_sample_length);
    Load_Audio_Sample(3, fail_sample, fail_sample_length);
    Load_Audio_Sample(4, start_part1_sample, start_part1_sample_length);
    Load_Audio_Sample(5, start_part2_sample, start_part2_sample_length);
    Load_Audio_Sample(6, blow_it_sample, blow_it_sample_length);
    Load_Audio_Sample(7, win_sample, win_sample_length);
#endif

    ret = BSP_QSPI_EnableMemoryMappedMode();
    if (ret != 0) {
     printf("Error: Initialising QSPI Flash in Memory Mapped mode\n");
     return;
    }

}


inline static void Load_Audio_Sample(uint8_t block, const uint8_t *sample, uint32_t sample_length) {

    debugf("Loading sample\n");
    QSPI_Erase(block);
    uint32_t block_address = block * MX25R6435F_BLOCK_SIZE;
    QSPI_Write(block_address, (uint8_t *)sample, sample_length);
}

static void QSPI_GetInfo() {
    QSPI_Info pInfo;

    BSP_QSPI_GetInfo(&pInfo);

    debugf("QSPI info:\n");
    debugf("\tFlash size: %ld\n", pInfo.FlashSize);
    debugf("\tSector size: %ld\n", pInfo.EraseSectorSize);
    debugf("\tSector num: %ld\n", pInfo.EraseSectorsNumber);
    debugf("\tPage size: %ld\n", pInfo.ProgPageSize);
    debugf("\tPage num: %ld\n", pInfo.ProgPagesNumber);

}


static void QSPI_Erase(uint8_t block) {

    uint32_t block_address = block * MX25R6435F_BLOCK_SIZE;

    debugf("Erasing block %d (%dkB) at 0x%.5lx \n", block, MX25R6435F_BLOCK_SIZE, block_address);

    uint8_t ret = BSP_QSPI_Erase_Block(block_address);
    if (ret != 0) {
        printf("Error: Erasing QSPI Flash\n");
        return;
    }

}


static void QSPI_Read(uint32_t address, uint8_t *buffer, uint32_t length) {

    debugf("Reading %ld Bytes from 0x%lx\n", length, address);

    memset(buffer, 0x00, length);

    uint8_t status;
    while ((status = BSP_QSPI_GetStatus()) != QSPI_OK) {
        debugf("Chip is busy\n");
    } 

    uint8_t ret = BSP_QSPI_Read(buffer, address, length);
    if (ret) {
     printf("Error %d: Reading to QSPI flash at address 0x%.5lx\n", ret, address);
    }

    debugf("Data:\n");
    for (uint8_t i = 0; i < length; i++) {
        debugf("%d ", buffer[i]);
    }
    debugf("\n");
}

static void QSPI_Write(uint32_t address, uint8_t *data, uint32_t length) {

    debugf("Writing %ld Bytes to 0x%lx\n", length, address);

    uint8_t status;
    while ((status = BSP_QSPI_GetStatus()) != QSPI_OK) {
        debugf("Chip is busy\n");
    } 

    uint8_t ret = BSP_QSPI_Write(data, address, length);
    if (ret) {
     printf("Error %d: Writing to QSPI flash at address 0x%lx\n", ret, address);
    }

}



/* --- Audio playback functions -- */

// Get the memory mapped address based on the block number
static uint32_t block_mapped_address(uint8_t block) {

    uint32_t block_address = block * MX25R6435F_BLOCK_SIZE;
    block_address = QSPI_FLASH_MAPPED_ADDR | block_address;
    debugf("Block %d address (mem mapped): 0x%lx\n", block, block_address);

    return block_address;
}


void play_bop_it_sample() {
    debugf("Playing Bop It sample\n");
    uint32_t addr = block_mapped_address(0);
    HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t *) addr, bop_it_sample_length, DAC_ALIGN_8B_R);
}

void play_twist_it_sample() {
    debugf("Playing Twist It sample\n");
    uint32_t addr = block_mapped_address(1);
    HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t *) addr, twist_it_sample_length, DAC_ALIGN_8B_R);
}

void play_blow_it_sample() {
    debugf("Playing Blow It sample\n");
    uint32_t addr = block_mapped_address(6);
    HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t *) addr, blow_it_sample_length, DAC_ALIGN_8B_R);
}

void play_success_sample() {
    debugf("Playing Success sample\n");
    uint32_t addr = block_mapped_address(2);
    HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t *) addr, success_sample_length, DAC_ALIGN_8B_R);
}

void play_fail_sample() {
    debugf("Playing Fail sample\n");
    uint32_t addr = block_mapped_address(3);
    HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t *) addr, fail_sample_length, DAC_ALIGN_8B_R);
}

void play_start_part1_sample() {
    debugf("Playing Start part 1 sample\n");
    uint32_t addr = block_mapped_address(4);
    HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t *) addr, start_part1_sample_length, DAC_ALIGN_8B_R);
}

void play_start_part2_sample() {
    debugf("Playing Start part 2 sample\n");
    uint32_t addr = block_mapped_address(5);
    HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t *) addr, start_part2_sample_length, DAC_ALIGN_8B_R);
}

void play_win_sample() {
    debugf("Playing Win sample\n");
    uint32_t addr = block_mapped_address(7);
    HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t *) addr, win_sample_length, DAC_ALIGN_8B_R);
}

