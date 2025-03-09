/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <fal.h>
#include <flash/flash.h>

#define PAGE_SIZE     4096

static int init(void)
{
  flash_init();
  return 1;
}


static int ef_err_port_cnt = 0;
int on_ic_read_cnt  = 0;
int on_ic_write_cnt = 0;

static int read(long offset, uint8_t *buf, size_t size)
{
  uint32_t addr = spi_flash.addr + offset;
  flash_read(addr, buf, size);
  return size;
}


static int write(long offset, const uint8_t *buf, size_t size)
{
  uint32_t addr = spi_flash.addr + offset;
  flash_program(addr, (uint8_t *)buf, size);
  return size;
}


static int erase(long offset, size_t size)
{
  uint32_t addr = spi_flash.addr + offset;
  flash_erase_bulk(addr, size);
  return size;
}


/*
  "stm32_onchip" : Flash 设备的名字。
  0x08000000: 对 Flash 操作的起始地址。
  1024*1024：Flash 的总大小（1MB）。
  128*1024：Flash 块/扇区大小（因为 STM32F2 各块大小不均匀，所以擦除粒度为最大块的大小：128K）。
  {init, read, write, erase} ：Flash 的操作函数。 如果没有 init 初始化过程，第一个操作函数位置可以置空。
  8 : 设置写粒度，单位 bit， 0 表示未生效（默认值为 0 ），该成员是 fal 版本大于 0.4.0 的新增成员。各个 flash 写入粒度不尽相同，可通过该成员进行设置，以下列举几种常见 Flash 写粒度：
  nor flash:  1 bit
  stm32f2/f4: 8 bit
  stm32f1:    32 bit
  stm32l4:    64 bit
 */

//1.定义 flash 设备

const struct fal_flash_dev spi_flash =
{
  .name       = "zd25wq16",
  .addr       = 0x00000000,
  .len        = 2*1024*1024, // 16 * 1024 * 1024 / 8 byte
  .blk_size   = 4*1024,      // 4k erase sector
  .ops        = {init, read, write, erase},
  .write_gran = 1
};


