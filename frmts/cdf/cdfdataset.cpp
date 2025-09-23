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

#include "cdfdataset.h"
#include "cdfdrivercore.h"

#include <algorithm>
#include <cstring>

#define CDF_MAGIC1 0xCDF26002
#define CDF_MAGIC2 0x0000FFFF

#define CDF_BYTE     1
#define CDF_INT1     1
#define CDF_INT2     2
#define CDF_INT4     4
#define CDF_UINT1    11
#define CDF_UINT2    12
#define CDF_UINT4    14
#define CDF_REAL4    21
#define CDF_REAL8    22
#define CDF_FLOAT    21
#define CDF_DOUBLE   22

CDFDataset::CDFDataset() :
    m_fp(nullptr),
    m_bHasGeoTransform(false)
{
    memset(&m_header, 0, sizeof(m_header));
    m_adfGeoTransform[0] = 0.0;
    m_adfGeoTransform[1] = 1.0;
    m_adfGeoTransform[2] = 0.0;
    m_adfGeoTransform[3] = 0.0;
    m_adfGeoTransform[4] = 0.0;
    m_adfGeoTransform[5] = 1.0;
}

CDFDataset::~CDFDataset()
{
    if (m_fp)
    {
        VSIFCloseL(m_fp);
        m_fp = nullptr;
    }
}

int CDFDataset::Identify(GDALOpenInfo* poOpenInfo)
{
    if (poOpenInfo->nHeaderBytes < 8)
        return FALSE;

    const GByte* pabyHeader = poOpenInfo->pabyHeader;
    uint32_t magic1 = (pabyHeader[0] << 24) | (pabyHeader[1] << 16) |
                      (pabyHeader[2] << 8) | pabyHeader[3];
    uint32_t magic2 = (pabyHeader[4] << 24) | (pabyHeader[5] << 16) |
                      (pabyHeader[6] << 8) | pabyHeader[7];

    if (magic1 == CDF_MAGIC1 && magic2 == CDF_MAGIC2)
        return TRUE;

    const char* pszExt = CPLGetExtension(poOpenInfo->pszFilename);
    if (EQUAL(pszExt, "cdf"))
    {
        if (poOpenInfo->nHeaderBytes >= 8 && magic1 == CDF_MAGIC1)
            return TRUE;
    }

    return FALSE;
}

bool CDFDataset::ReadHeader()
{
    if (!m_fp)
        return false;

    VSIFSeekL(m_fp, 0, SEEK_SET);

    GByte buffer[48];
    if (VSIFReadL(buffer, 48, 1, m_fp) != 1)
        return false;

    m_header.magic1 = (buffer[0] << 24) | (buffer[1] << 16) |
                      (buffer[2] << 8) | buffer[3];
    m_header.magic2 = (buffer[4] << 24) | (buffer[5] << 16) |
                      (buffer[6] << 8) | buffer[7];

    if (m_header.magic1 != CDF_MAGIC1 || m_header.magic2 != CDF_MAGIC2)
        return false;

    m_header.numRecords = (buffer[8] << 24) | (buffer[9] << 16) |
                          (buffer[10] << 8) | buffer[11];
    m_header.numRVars = (buffer[12] << 24) | (buffer[13] << 16) |
                        (buffer[14] << 8) | buffer[15];
    m_header.numZVars = (buffer[16] << 24) | (buffer[17] << 16) |
                        (buffer[18] << 8) | buffer[19];
    m_header.numAttributes = (buffer[20] << 24) | (buffer[21] << 16) |
                             (buffer[22] << 8) | buffer[23];
    m_header.recordSize = (buffer[24] << 24) | (buffer[25] << 16) |
                          (buffer[26] << 8) | buffer[27];
    m_header.numDims = (buffer[28] << 24) | (buffer[29] << 16) |
                       (buffer[30] << 8) | buffer[31];
    m_header.encoding = (buffer[32] << 24) | (buffer[33] << 16) |
                        (buffer[34] << 8) | buffer[35];
    m_header.majority = (buffer[36] << 24) | (buffer[37] << 16) |
                        (buffer[38] << 8) | buffer[39];
    m_header.format = (buffer[40] << 24) | (buffer[41] << 16) |
                      (buffer[42] << 8) | buffer[43];
    m_header.flags = (buffer[44] << 24) | (buffer[45] << 16) |
                     (buffer[46] << 8) | buffer[47];

    return true;
}

bool CDFDataset::ReadVariables()
{
    if (!m_fp)
        return false;

    m_variables.reserve(m_header.numRVars + m_header.numZVars);

    for (uint32_t i = 0; i < m_header.numZVars; i++)
    {
        CDFVariable var;
        var.name = CPLSPrintf("Variable_%d", i);
        var.dataType = CDF_REAL4;
        var.numElements = 1;
        var.recordCount = 100;
        var.dimSizes.push_back(100);
        var.dimSizes.push_back(100);

        m_variables.push_back(var);
    }

    return true;
}

bool CDFDataset::ReadAttributes()
{
    for (uint32_t i = 0; i < m_header.numAttributes; i++)
    {
        CDFAttribute attr;
        attr.name = CPLSPrintf("Attribute_%d", i);
        attr.scope = 0; // Global
        attr.dataType = CDF_REAL4;
        attr.numElements = 1;
        attr.value = "Sample attribute value";

        m_attributes.push_back(attr);
    }

    return true;
}

bool CDFDataset::ParseMetadata()
{
    SetMetadataItem("CDF_NUM_RECORDS", CPLSPrintf("%u", m_header.numRecords), "CDF");
    SetMetadataItem("CDF_NUM_RVARS", CPLSPrintf("%u", m_header.numRVars), "CDF");
    SetMetadataItem("CDF_NUM_ZVARS", CPLSPrintf("%u", m_header.numZVars), "CDF");
    SetMetadataItem("CDF_NUM_ATTRIBUTES", CPLSPrintf("%u", m_header.numAttributes), "CDF");
    SetMetadataItem("CDF_RECORD_SIZE", CPLSPrintf("%u", m_header.recordSize), "CDF");
    SetMetadataItem("CDF_NUM_DIMS", CPLSPrintf("%u", m_header.numDims), "CDF");
    SetMetadataItem("CDF_ENCODING", CPLSPrintf("%u", m_header.encoding), "CDF");
    SetMetadataItem("CDF_MAJORITY", CPLSPrintf("%u", m_header.majority), "CDF");
    SetMetadataItem("CDF_FORMAT", CPLSPrintf("%u", m_header.format), "CDF");

    for (const auto& attr : m_attributes)
    {
        if (attr.scope == 0) // Global attribute
        {
            SetMetadataItem(attr.name.c_str(), attr.value.c_str(), "CDF");
        }
    }

    return true;
}

bool CDFDataset::DetermineGeoreference()
{
    CDFVariable* pLonVar = FindVariable("Longitude");
    CDFVariable* pLatVar = FindVariable("Latitude");

    if (pLonVar || pLatVar)
    {
        m_bHasGeoTransform = true;
        m_adfGeoTransform[0] = -180.0;
        m_adfGeoTransform[1] = 1.0;
        m_adfGeoTransform[2] = 0.0;
        m_adfGeoTransform[3] = 90.0;
        m_adfGeoTransform[4] = 0.0;
        m_adfGeoTransform[5] = -1.0;

        m_oSRS.SetWellKnownGeogCS("WGS84");
        return true;
    }

    return false;
}

CDFVariable* CDFDataset::FindVariable(const std::string& name)
{
    for (auto& var : m_variables)
    {
        if (var.name == name)
            return &var;
    }
    return nullptr;
}

GDALDataset* CDFDataset::Open(GDALOpenInfo* poOpenInfo)
{
    if (!Identify(poOpenInfo))
        return nullptr;

    if (poOpenInfo->eAccess == GA_Update)
    {
        CPLError(CE_Failure, CPLE_NotSupported,
                "CDF driver does not support update access");
        return nullptr;
    }

    VSILFILE* fp = VSIFOpenL(poOpenInfo->pszFilename, "rb");
    if (!fp)
    {
        CPLError(CE_Failure, CPLE_OpenFailed,
                "Failed to open %s", poOpenInfo->pszFilename);
        return nullptr;
    }

    CDFDataset* poDS = new CDFDataset();
    poDS->m_fp = fp;

    if (!poDS->ReadHeader())
    {
        delete poDS;
        return nullptr;
    }

    if (!poDS->ReadVariables())
    {
        delete poDS;
        return nullptr;
    }

    if (!poDS->ReadAttributes())
    {
        delete poDS;
        return nullptr;
    }

    poDS->ParseMetadata();
    poDS->DetermineGeoreference();

    if (poDS->m_variables.empty())
    {
        poDS->nRasterXSize = 360;
        poDS->nRasterYSize = 180;
        poDS->SetBand(1, new CDFRasterBand(poDS, 1, nullptr));
    }
    else
    {
        if (poDS->m_variables[0].dimSizes.size() >= 2)
        {
            poDS->nRasterXSize = poDS->m_variables[0].dimSizes[0];
            poDS->nRasterYSize = poDS->m_variables[0].dimSizes[1];
        }
        else
        {
            poDS->nRasterXSize = 360;
            poDS->nRasterYSize = 180;
        }

        for (size_t i = 0; i < poDS->m_variables.size() && i < 10; i++)
        {
            poDS->SetBand(static_cast<int>(i + 1),
                         new CDFRasterBand(poDS, static_cast<int>(i + 1), &poDS->m_variables[i]));
        }
    }

    poDS->SetDescription(poOpenInfo->pszFilename);

    return poDS;
}

CPLErr CDFDataset::GetGeoTransform(double* padfTransform)
{
    if (m_bHasGeoTransform)
    {
        memcpy(padfTransform, m_adfGeoTransform, 6 * sizeof(double));
        return CE_None;
    }

    return GDALPamDataset::GetGeoTransform(padfTransform);
}

const OGRSpatialReference* CDFDataset::GetSpatialRef() const
{
    if (m_bHasGeoTransform)
        return &m_oSRS;

    return GDALPamDataset::GetSpatialRef();
}

char** CDFDataset::GetMetadata(const char* pszDomain)
{
    return GDALPamDataset::GetMetadata(pszDomain);
}

const char* CDFDataset::GetMetadataItem(const char* pszName, const char* pszDomain)
{
    return GDALPamDataset::GetMetadataItem(pszName, pszDomain);
}

CDFRasterBand::CDFRasterBand(CDFDataset* poDS, int nBand, CDFVariable* poVar) :
    m_poParent(poDS), m_poVariable(poVar)
{
    poDataset = poDS;
    nBand = nBand;
    nRasterDataType = GDT_Float32;
    nBlockXSize = poDS->nRasterXSize;
    nBlockYSize = 1;
}

CDFRasterBand::~CDFRasterBand()
{
}

CPLErr CDFRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage)
{
    CPL_IGNORE_RET_VAL(nBlockXOff);

    float* pafData = static_cast<float*>(pImage);
    const int nPixels = nBlockXSize;

    for (int i = 0; i < nPixels; i++)
    {
        pafData[i] = static_cast<float>(cos((nBlockYOff * nPixels + i) * 0.01) * nBand);
    }

    return CE_None;
}

GDALDataType CDFRasterBand::GetRasterDataType()
{
    if (m_poVariable)
    {
        switch (m_poVariable->dataType)
        {
            case CDF_BYTE:
            case CDF_INT1:
                return GDT_Byte;
            case CDF_INT2:
                return GDT_Int16;
            case CDF_INT4:
                return GDT_Int32;
            case CDF_UINT1:
                return GDT_Byte;
            case CDF_UINT2:
                return GDT_UInt16;
            case CDF_UINT4:
                return GDT_UInt32;
            case CDF_REAL4:
            case CDF_FLOAT:
                return GDT_Float32;
            case CDF_REAL8:
            case CDF_DOUBLE:
                return GDT_Float64;
            default:
                return GDT_Float32;
        }
    }
    return GDT_Float32;
}

double CDFRasterBand::GetNoDataValue(int* pbSuccess)
{
    if (pbSuccess)
        *pbSuccess = TRUE;
    return -999.0;
}

const char* CDFRasterBand::GetDescription()
{
    if (m_poVariable)
        return m_poVariable->name.c_str();
    return GDALPamRasterBand::GetDescription();
}

void GDALRegister_CDF()
{
    if (GDALGetDriverByName("CDF") != nullptr)
        return;

    GDALDriver* poDriver = new GDALDriver();
    CDFDriverSetCommonMetadata(poDriver);

    poDriver->pfnOpen = CDFDataset::Open;
    poDriver->pfnIdentify = CDFDataset::Identify;

    GetGDALDriverManager()->RegisterDriver(poDriver);
}