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

#include "bufrdrivercore.h"

void BUFRDriverSetCommonMetadata(GDALDriver* poDriver)
{
    poDriver->SetDescription("BUFR");
    poDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME,
                             "BUFR (Binary Universal Form for the "
                             "Representation of meteorological data)");
    poDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC,
                             "drivers/raster/bufr.html");
    poDriver->SetMetadataItem(GDAL_DMD_EXTENSION, "bufr");
    poDriver->SetMetadataItem(GDAL_DMD_EXTENSIONS, "bufr bfr");
    poDriver->SetMetadataItem(GDAL_DMD_MIMETYPE, "application/x-bufr");
    poDriver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");

    poDriver->pfnIdentify = nullptr;
    poDriver->SetMetadataItem(GDAL_DCAP_OPEN, "YES");
}