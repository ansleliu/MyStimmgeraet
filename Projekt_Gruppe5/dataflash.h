#ifndef _DATAFLASH_H_
#define _DATAFLASH_H_

// Read Commands
#define DATAFLASH_MAIN_MEMORY_PAGE_READ                                           0xd2
#define DATAFLASH_CONTINUOUS_ARRAY_READ_LC                                        0xe8
#define DATAFLASH_CONTINUOUS_ARRAY_READ_LF                                        0x03
#define DATAFLASH_CONTINUOUS_ARRAY_READ_HF                                        0x0b
#define DATAFLASH_BUFFER_1_READ_LF                                                0xd1
#define DATAFLASH_BUFFER_2_READ_LF                                                0xd3
#define DATAFLASH_BUFFER_1_READ                                                   0xd4
#define DATAFLASH_BUFFER_2_READ                                                   0xd6

// Program and Erase Commands
#define DATAFLASH_BUFFER_1_WRITE                                                  0x84
#define DATAFLASH_BUFFER_2_WRITE                                                  0x87
#define DATAFLASH_BUFFER_1_TO_MAIN_MEMORY_PAGE_PROGRAM_WITH_BUILT_IN_ERASE        0x83
#define DATAFLASH_BUFFER_2_TO_MAIN_MEMORY_PAGE_PROGRAM_WITH_BUILT_IN_ERASE        0x86
#define DATAFLASH_BUFFER_1_TO_MAIN_MEMORY_PAGE_PROGRAM_WITHOUT_BUILT_IN_ERASE     0x88
#define DATAFLASH_BUFFER_2_TO_MAIN_MEMORY_PAGE_PROGRAM_WITHOUT_BUILT_IN_ERASE     0x89
#define DATAFLASH_PAGE_ERASE                                                      0x81
#define DATAFLASH_BLOCK_ERASE                                                     0x50
#define DATAFLASH_SECTOR_ERASE                                                    0x7c
#define DATAFLASH_CHIP_ERASE_1                                                    0xc7
#define DATAFLASH_CHIP_ERASE_2                                                    0x94
#define DATAFLASH_CHIP_ERASE_3                                                    0x80
#define DATAFLASH_CHIP_ERASE_4                                                    0x9a
#define DATAFLASH_MAIN_MEMORY_PAGE_PROGRAM_THROUGH_BUFFER_1                       0x82
#define DATAFLASH_MAIN_MEMORY_PAGE_PROGRAM_THROUGH_BUFFER_2                       0x85

// Additional Commands
#define DATAFLASH_MAIN_MEMORY_PAGE_TO_BUFFER_1_TRANSFER                           0x53
#define DATAFLASH_MAIN_MEMORY_PAGE_TO_BUFFER_2_TRANSFER                           0x55
#define DATAFLASH_MAIN_MEMORY_PAGE_TO_BUFFER_1_COMPARE                            0x60
#define DATAFLASH_MAIN_MEMORY_PAGE_TO_BUFFER_2_COMPARE                            0x61
#define DATAFLASH_AUTO_PAGE_REWRITE_THROUGH_BUFFER_1                              0x58
#define DATAFLASH_AUTO_PAGE_REWRITE_THROUGH_BUFFER_2                              0x59
#define DATAFLASH_DEEP_POWER_DOWN                                                 0xb9
#define DATAFLASH_RESUME_FROM_DEEP_POWER_DOWN                                     0xab
#define DATAFLASH_STATUS_REGISTER_READ                                            0xd7
#define DATAFLASH_MANUFACTURER_AND_DEVICE_ID_READ                                 0x9f

#define DATAFLASH_Chip_Select         (PORTB &= ~(1<<PB4))
#define DATAFLASH_Chip_Unselect       (PORTB |= (1<<PB4))

// Function Prototypes
void dataflash_init (void);
void dataflash_buffer_to_page (unsigned int page, unsigned char buffer);
void dataflash_page_to_buffer (unsigned int page, unsigned char buffer);
void dataflash_buffer_read (unsigned char buffer, unsigned int offset,
                            unsigned int length, unsigned char *array);
void dataflash_buffer_write (unsigned char buffer, unsigned int offset,
                             unsigned int length, unsigned char *array);
void dataflash_read (unsigned int page, unsigned int offset,
                     unsigned int length, unsigned char *array);

void dataflash_chip_erase (void);

#endif
