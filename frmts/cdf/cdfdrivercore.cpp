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

#include "cdfdrivercore.h"

void CDFDriverSetCommonMetadata(GDALDriver* poDriver)
{
    poDriver->SetDescription("CDF");
    poDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME,
                             "CDF (Common Data Format)");
    poDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC,
                             "drivers/raster/cdf.html");
    poDriver->SetMetadataItem(GDAL_DMD_EXTENSION, "cdf");
    poDriver->SetMetadataItem(GDAL_DMD_EXTENSIONS, "cdf");
    poDriver->SetMetadataItem(GDAL_DMD_MIMETYPE, "application/x-cdf");
    poDriver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");

    poDriver->pfnIdentify = nullptr;
    poDriver->SetMetadataItem(GDAL_DCAP_OPEN, "YES");
}