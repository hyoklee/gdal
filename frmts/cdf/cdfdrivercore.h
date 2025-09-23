/******************************************************************************
 *
 * Project:  GDAL
 * Purpose:  CDF driver
 * Author:   Claude AI Assistant
 *
 ******************************************************************************
 * Copyright (c) 2025, Claude AI Assistant
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#ifndef CDFDRIVERCORE_H
#define CDFDRIVERCORE_H

#include "gdal_priv.h"

#ifdef __cplusplus
extern "C"
{
#endif

void CDFDriverSetCommonMetadata(GDALDriver* poDriver);

#ifdef __cplusplus
}
#endif

#endif // CDFDRIVERCORE_H