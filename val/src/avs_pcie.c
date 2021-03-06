/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_common.h"

#include "include/sbsa_avs_pcie.h"

PCIE_INFO_TABLE *g_pcie_info_table;

uint64_t
pal_get_mcfg_ptr(void);

/**
  @brief   This API reads 32-bit data from PCIe config space pointed by Bus,
           Device, Function and register offset.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_pcie_create_info_table
  @param   bdf    - concatenated Bus(8-bits), device(8-bits) & function(8-bits)
  @param   offset - Register offset within a device PCIe config space

  @return  32-bit data read from the config space
**/
uint32_t
val_pcie_read_cfg(uint32_t bdf, uint32_t offset)
{
  uint32_t bus     = PCIE_EXTRACT_BDF_BUS(bdf);
  uint32_t dev     = PCIE_EXTRACT_BDF_DEV(bdf);
  uint32_t func    = PCIE_EXTRACT_BDF_FUNC(bdf);
  uint32_t cfg_addr;
  addr_t   ecam_base = 0;
  uint32_t i = 0;

  if ((bus >= PCIE_MAX_BUS) || (dev >= PCIE_MAX_DEV) || (func >= PCIE_MAX_FUNC)) {
     val_print(AVS_PRINT_ERR, "Invalid Bus/Dev/Func  %x \n", bdf);
     return 0;
  }

  if (g_pcie_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "\n    Read_PCIe_CFG: PCIE info table is not created", 0);
      return 0;
  }

  while (i < val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0))
  {

      if ((bus >= val_pcie_get_info(PCIE_INFO_START_BUS, i)) &&
           (bus <= val_pcie_get_info(PCIE_INFO_END_BUS, i))) {
          ecam_base = val_pcie_get_info(PCIE_INFO_ECAM, i);

          if (ecam_base == 0) {
              val_print(AVS_PRINT_ERR, "\n    Read PCIe_CFG: ECAM Base is zero ", 0);
              return 0;
          }
          break;
      }
      i++;
  }

  /* There are 8 functions / device, 32 devices / Bus and each has a 4KB config space */
  cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * 4096) + \
               (dev * PCIE_MAX_FUNC * 4096) + func;

  val_print(AVS_PRINT_INFO, "   calculated config address is %x \n", ecam_base + cfg_addr + offset);

  return pal_mmio_read(ecam_base + cfg_addr + offset);

}

/**
  @brief   This API writes 32-bit data to PCIe config space pointed by Bus,
           Device, Function and register offset.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_pcie_create_info_table
  @param   bdf    - concatenated Bus(8-bits), device(8-bits) & function(8-bits)
  @param   offset - Register offset within a device PCIe config space
  @param   data   - data to be written to the config space

  @return  None
**/
void
val_pcie_write_cfg(uint32_t bdf, uint32_t offset, uint32_t data)
{
  uint32_t bus      = PCIE_EXTRACT_BDF_BUS(bdf);
  uint32_t dev      = PCIE_EXTRACT_BDF_DEV(bdf);
  uint32_t func     = PCIE_EXTRACT_BDF_FUNC(bdf);
  uint32_t cfg_addr;
  addr_t   ecam_base = 0;
  uint32_t i = 0;


  if ((bus >= PCIE_MAX_BUS) || (dev >= PCIE_MAX_DEV) || (func >= PCIE_MAX_FUNC)) {
     val_print(AVS_PRINT_ERR, "Invalid Bus/Dev/Func  %x \n", bdf);
     return;
  }

  if (g_pcie_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "\n Write PCIe_CFG: PCIE info table is not created", 0);
      return;
  }

  while (i < val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0))
  {

      if ((bus >= val_pcie_get_info(PCIE_INFO_START_BUS, i)) &&
           (bus <= val_pcie_get_info(PCIE_INFO_END_BUS, i))) {
          ecam_base = val_pcie_get_info(PCIE_INFO_ECAM, i);

          if (ecam_base == 0) {
              val_print(AVS_PRINT_ERR, "\n    Read PCIe_CFG: ECAM Base is zero ", 0);
              return;
          }
          break;
      }
      i++;
  }

  /* There are 8 functions / device, 32 devices / Bus and each has a 4KB config space */
  cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * 4096) + \
               (dev * PCIE_MAX_FUNC * 4096) + func;

  pal_mmio_write(ecam_base + cfg_addr + offset, data);
}


/**
  @brief   This API executes all the PCIe tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_pcie_create_info_table()
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_pcie_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status, i;

  if (level == 0) {
    val_print(AVS_PRINT_WARN, "PCIe compliance is required  only from Level %d \n", 1);
    return AVS_STATUS_SKIP;
  }

  for (i=0 ; i<MAX_TEST_SKIP_NUM ; i++){
      if (g_skip_test_num[i] == AVS_PCIE_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "\n USER Override - Skipping all PCIe tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }


  status = p001_entry(num_pe);

  if (status != AVS_STATUS_PASS) {
    val_print(AVS_PRINT_WARN, "\n     *** Skipping remaining PCIE tests *** \n", 0);
    return status;
  }

  status |= p002_entry(num_pe);
  status |= p003_entry(num_pe);

  if (status != AVS_STATUS_PASS) {
      val_print(AVS_PRINT_ERR, "\n     One or more PCIe tests have failed.... \n", status);
  }

  return status;
}


/**
  @brief   This API will call PAL layer to fill in the PCIe information
           into the g_pcie_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   pcie_info_table    Memory address where PCIe Information is populated

  @return  Error if Input param is NULL
**/
void
val_pcie_create_info_table(uint64_t *pcie_info_table)
{

  if (pcie_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "Input for Create Info table cannot be NULL \n", 0);
      return;
  }

  g_pcie_info_table = (PCIE_INFO_TABLE *)pcie_info_table;

  pal_pcie_create_info_table(g_pcie_info_table);

  val_print(AVS_PRINT_TEST, " PCIE_INFO: Number of ECAM regions    :    %lx \n", val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0));
}

/**
  @brief  Free the memory allocated for the pcie_info_table
**/
void
val_pcie_free_info_table()
{
  pal_mem_free((void *)g_pcie_info_table);
}


/**
  @brief   This API is the single entry point to return all PCIe related information
           1. Caller       -  Test Suite
           2. Prerequisite -  val_pcie_create_info_table
  @param   info_type  - Type of the information to be returned
  @param   index      - The index of the Ecam region whose information is returned

  @return  64-bit data pertaining to the requested input type
**/
uint64_t
val_pcie_get_info(PCIE_INFO_e type, uint32_t index)
{

  if (g_pcie_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "GET_PCIe_INFO: PCIE info table is not created \n", 0);
      return 0;
  }

  if (index >= g_pcie_info_table->num_entries) {
      if (g_pcie_info_table->num_entries != 0)
          val_print(AVS_PRINT_ERR, "Invalid index %d > num of entries \n", index);
      return 0;
  }

  switch (type) {
      case PCIE_INFO_NUM_ECAM:
          return g_pcie_info_table->num_entries;
      case PCIE_INFO_MCFG_ECAM:
          return pal_pcie_get_mcfg_ecam();
      case PCIE_INFO_ECAM:
          return g_pcie_info_table->block[index].ecam_base;
      case PCIE_INFO_START_BUS:
          return g_pcie_info_table->block[index].start_bus_num;
      case PCIE_INFO_END_BUS:
          return g_pcie_info_table->block[index].end_bus_num;
      case PCIE_INFO_SEGMENT:
          return g_pcie_info_table->block[index].segment_num;
      default:
          val_print(AVS_PRINT_ERR, "This PCIE info option not supported %d \n", type);
          break;
  }

  return 0;
}

