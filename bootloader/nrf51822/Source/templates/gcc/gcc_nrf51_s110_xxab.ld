/* Linker script to configure memory regions. */
SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x14000, LENGTH = 0xC000 /* 80 kB is taken by S110, 48 kB available for application. */
  RAM (rwx) : ORIGIN = 0x20002000, LENGTH = 0x2000 /* 8 kB, 8 kB is taken by S110. */
}
INCLUDE "gcc_nrf51_common.ld"
