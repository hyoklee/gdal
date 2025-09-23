/******************************************************************************
 *
 * Project:  GDAL
 * Purpose:  BUFR driver
 * Author:   Claude AI Assistant
 *
 ******************************************************************************
 * Copyright (c) 2025, Claude AI Assistant
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#ifndef BUFRDRIVERCORE_H
#define BUFRDRIVERCORE_H

#include "gdal_priv.h"

#ifdef __cplusplus
extern "C"
{
#endif

void BUFRDriverSetCommonMetadata(GDALDriver* poDriver);

#ifdef __cplusplus
}
#endif

#endif // BUFRDRIVERCORE_H