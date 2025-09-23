/******************************************************************************
 *
 * Project:  GDAL
 * Purpose:  BUFR (Binary Universal Form for the Representation of
 *           meteorological data) driver
 * Author:   Claude AI Assistant
 *
 ******************************************************************************
 * Copyright (c) 2025, Claude AI Assistant
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#ifndef BUFRDATASET_H
#define BUFRDATASET_H

#include "gdal_priv.h"
#include "gdal_pam.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include "cpl_error.h"
#include "ogr_spatialref.h"

#include <vector>
#include <map>
#include <string>

class BUFRRasterBand;

struct BUFRHeader {
    char identifier[4];  // "BUFR"
    uint32_t messageLength;
    uint8_t edition;
    uint8_t masterTable;
    uint16_t subcentre;
    uint16_t centre;
    uint8_t updateSequence;
    uint8_t category;
    uint8_t subcategory;
    uint8_t localSubcategory;
    uint8_t masterTableVersion;
    uint8_t localTableVersion;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

struct BUFRDescriptor {
    uint8_t f;  // Format (0-3)
    uint8_t x;  // Class (0-63)
    uint8_t y;  // Element (0-255)
};

class BUFRDataset final: public GDALPamDataset
{
    friend class BUFRRasterBand;

    private:
        VSILFILE* m_fp;
        BUFRHeader m_header;
        std::vector<BUFRDescriptor> m_descriptors;
        std::map<std::string, std::string> m_metadata;
        bool m_bHasGeoTransform;
        double m_adfGeoTransform[6];
        OGRSpatialReference m_oSRS;

        bool ReadHeader();
        bool ReadDescriptors();
        bool ParseMetadata();
        bool DetermineGeoreference();

    public:
        BUFRDataset();
        virtual ~BUFRDataset();

        static GDALDataset* Open(GDALOpenInfo* poOpenInfo);
        static int Identify(GDALOpenInfo* poOpenInfo);

        virtual CPLErr GetGeoTransform(GDALGeoTransform& gt) const override;
        virtual const OGRSpatialReference* GetSpatialRef() const override;
        virtual char** GetMetadata(const char* pszDomain = "") override;
        virtual const char* GetMetadataItem(const char* pszName,
                                          const char* pszDomain = "") override;
};

class BUFRRasterBand final: public GDALPamRasterBand
{
    private:
        BUFRDataset* m_poParent;

    public:
        BUFRRasterBand(BUFRDataset* poDS, int nBandNum);
        virtual ~BUFRRasterBand();

        virtual CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage) override;
        virtual double GetNoDataValue(int* pbSuccess = nullptr) override;
};

void GDALRegister_BUFR();

#endif // BUFRDATASET_H