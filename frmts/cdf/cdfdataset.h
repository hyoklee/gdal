/******************************************************************************
 *
 * Project:  GDAL
 * Purpose:  CDF (Common Data Format) driver
 * Author:   Claude AI Assistant
 *
 ******************************************************************************
 * Copyright (c) 2025, Claude AI Assistant
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#ifndef CDFDATASET_H
#define CDFDATASET_H

#include "gdal_priv.h"
#include "gdal_pam.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include "cpl_error.h"
#include "ogr_spatialref.h"

#include <vector>
#include <map>
#include <string>

class CDFRasterBand;

struct CDFHeader {
    uint32_t magic1;     // 0xCDF26002 for CDF v2.6
    uint32_t magic2;     // 0x0000FFFF
    uint32_t numRecords;
    uint32_t numRVars;
    uint32_t numZVars;
    uint32_t numAttributes;
    uint32_t recordSize;
    uint32_t numDims;
    uint32_t encoding;
    uint32_t majority;
    uint32_t format;
    uint32_t flags;
};

struct CDFVariable {
    std::string name;
    uint32_t dataType;
    uint32_t numElements;
    uint32_t recVariance;
    std::vector<uint32_t> dimVariances;
    std::vector<uint32_t> dimSizes;
    uint32_t recordCount;
    std::map<std::string, std::string> attributes;
};

struct CDFAttribute {
    std::string name;
    uint32_t scope;  // Global or Variable
    uint32_t dataType;
    uint32_t numElements;
    std::string value;
};

class CDFDataset final: public GDALPamDataset
{
    friend class CDFRasterBand;

    private:
        VSILFILE* m_fp;
        CDFHeader m_header;
        std::vector<CDFVariable> m_variables;
        std::vector<CDFAttribute> m_attributes;
        std::map<std::string, std::string> m_metadata;
        bool m_bHasGeoTransform;
        double m_adfGeoTransform[6];
        OGRSpatialReference m_oSRS;

        bool ReadHeader();
        bool ReadVariables();
        bool ReadAttributes();
        bool ParseMetadata();
        bool DetermineGeoreference();
        CDFVariable* FindVariable(const std::string& name);

    public:
        CDFDataset();
        virtual ~CDFDataset();

        static GDALDataset* Open(GDALOpenInfo* poOpenInfo);
        static int Identify(GDALOpenInfo* poOpenInfo);

        virtual CPLErr GetGeoTransform(double* padfTransform) override;
        virtual const OGRSpatialReference* GetSpatialRef() const override;
        virtual char** GetMetadata(const char* pszDomain = "") override;
        virtual const char* GetMetadataItem(const char* pszName,
                                          const char* pszDomain = "") override;
};

class CDFRasterBand final: public GDALPamRasterBand
{
    private:
        CDFDataset* m_poParent;
        CDFVariable* m_poVariable;

    public:
        CDFRasterBand(CDFDataset* poDS, int nBand, CDFVariable* poVar);
        virtual ~CDFRasterBand();

        virtual CPLErr IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage) override;
        virtual GDALDataType GetRasterDataType() override;
        virtual double GetNoDataValue(int* pbSuccess = nullptr) override;
        virtual const char* GetDescription() override;
};

void GDALRegister_CDF();

#endif // CDFDATASET_H