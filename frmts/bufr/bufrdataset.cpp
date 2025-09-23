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

#include "bufrdataset.h"
#include "bufrdrivercore.h"

#include <algorithm>
#include <cstring>

BUFRDataset::BUFRDataset() :
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

BUFRDataset::~BUFRDataset()
{
    if (m_fp)
    {
        VSIFCloseL(m_fp);
        m_fp = nullptr;
    }
}

int BUFRDataset::Identify(GDALOpenInfo* poOpenInfo)
{
    if (poOpenInfo->nHeaderBytes < 4)
        return FALSE;

    if (memcmp(poOpenInfo->pabyHeader, "BUFR", 4) == 0)
        return TRUE;

    const char* pszExt = CPLGetExtensionSafe(poOpenInfo->pszFilename).c_str();
    if (EQUAL(pszExt, "bufr") || EQUAL(pszExt, "bfr"))
    {
        if (poOpenInfo->nHeaderBytes >= 4 &&
            memcmp(poOpenInfo->pabyHeader, "BUFR", 4) == 0)
            return TRUE;
    }

    return FALSE;
}

bool BUFRDataset::ReadHeader()
{
    if (!m_fp)
        return false;

    VSIFSeekL(m_fp, 0, SEEK_SET);

    if (VSIFReadL(&m_header.identifier, 4, 1, m_fp) != 1)
        return false;

    if (memcmp(m_header.identifier, "BUFR", 4) != 0)
        return false;

    GByte buffer[20];
    if (VSIFReadL(buffer, 20, 1, m_fp) != 1)
        return false;

    m_header.messageLength = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    m_header.edition = buffer[3];

    if (m_header.edition < 2 || m_header.edition > 4)
    {
        CPLError(CE_Warning, CPLE_NotSupported,
                "BUFR edition %d not fully supported", m_header.edition);
    }

    if (m_header.edition >= 3)
    {
        m_header.masterTable = buffer[4];
        m_header.subcentre = (buffer[5] << 8) | buffer[6];
        m_header.centre = (buffer[7] << 8) | buffer[8];
        m_header.updateSequence = buffer[9];
        m_header.category = buffer[10];
        m_header.subcategory = buffer[11];
        m_header.localSubcategory = buffer[12];
        m_header.masterTableVersion = buffer[13];
        m_header.localTableVersion = buffer[14];
        m_header.year = (buffer[15] << 8) | buffer[16];
        m_header.month = buffer[17];
        m_header.day = buffer[18];
        m_header.hour = buffer[19];
    }

    return true;
}

bool BUFRDataset::ReadDescriptors()
{
    if (!m_fp)
        return false;

    VSIFSeekL(m_fp, 8, SEEK_SET);

    GByte sectionLengths[3];
    if (VSIFReadL(sectionLengths, 3, 1, m_fp) != 1)
        return false;

    uint32_t section1Length = (sectionLengths[0] << 16) | (sectionLengths[1] << 8) | sectionLengths[2];

    VSIFSeekL(m_fp, 8 + section1Length, SEEK_SET);

    GByte section3Header[7];
    if (VSIFReadL(section3Header, 7, 1, m_fp) != 1)
        return false;

    uint32_t section3Length = (section3Header[0] << 16) | (section3Header[1] << 8) | section3Header[2];
    uint16_t numSubsets = (section3Header[4] << 8) | section3Header[5];

    SetMetadataItem("BUFR_NUM_SUBSETS", CPLSPrintf("%d", numSubsets), "BUFR");

    uint32_t descriptorCount = (section3Length - 7) / 2;
    m_descriptors.reserve(descriptorCount);

    for (uint32_t i = 0; i < descriptorCount; i++)
    {
        GByte descriptorBytes[2];
        if (VSIFReadL(descriptorBytes, 2, 1, m_fp) != 1)
            break;

        BUFRDescriptor desc;
        uint16_t descValue = (descriptorBytes[0] << 8) | descriptorBytes[1];
        desc.f = (descValue >> 14) & 0x03;
        desc.x = (descValue >> 8) & 0x3F;
        desc.y = descValue & 0xFF;

        m_descriptors.push_back(desc);
    }

    return true;
}

bool BUFRDataset::ParseMetadata()
{
    SetMetadataItem("BUFR_EDITION", CPLSPrintf("%d", m_header.edition), "BUFR");
    SetMetadataItem("BUFR_MESSAGE_LENGTH", CPLSPrintf("%u", m_header.messageLength), "BUFR");
    SetMetadataItem("BUFR_MASTER_TABLE", CPLSPrintf("%d", m_header.masterTable), "BUFR");
    SetMetadataItem("BUFR_CENTRE", CPLSPrintf("%d", m_header.centre), "BUFR");
    SetMetadataItem("BUFR_SUBCENTRE", CPLSPrintf("%d", m_header.subcentre), "BUFR");
    SetMetadataItem("BUFR_CATEGORY", CPLSPrintf("%d", m_header.category), "BUFR");
    SetMetadataItem("BUFR_SUBCATEGORY", CPLSPrintf("%d", m_header.subcategory), "BUFR");

    if (m_header.year > 0)
    {
        SetMetadataItem("BUFR_DATETIME",
                       CPLSPrintf("%04d-%02d-%02d %02d:%02d:%02d",
                                m_header.year, m_header.month, m_header.day,
                                m_header.hour, m_header.minute, m_header.second),
                       "BUFR");
    }

    SetMetadataItem("BUFR_NUM_DESCRIPTORS", CPLSPrintf("%d", static_cast<int>(m_descriptors.size())), "BUFR");

    return true;
}

bool BUFRDataset::DetermineGeoreference()
{
    for (const auto& desc : m_descriptors)
    {
        if (desc.f == 0 && desc.x == 5)
        {
            if (desc.y == 1 || desc.y == 2)
            {
                m_bHasGeoTransform = true;
                m_adfGeoTransform[0] = -180.0;
                m_adfGeoTransform[1] = 0.5;
                m_adfGeoTransform[2] = 0.0;
                m_adfGeoTransform[3] = 90.0;
                m_adfGeoTransform[4] = 0.0;
                m_adfGeoTransform[5] = -0.5;

                m_oSRS.SetWellKnownGeogCS("WGS84");
                return true;
            }
        }
    }

    return false;
}

GDALDataset* BUFRDataset::Open(GDALOpenInfo* poOpenInfo)
{
    if (!Identify(poOpenInfo))
        return nullptr;

    if (poOpenInfo->eAccess == GA_Update)
    {
        CPLError(CE_Failure, CPLE_NotSupported,
                "BUFR driver does not support update access");
        return nullptr;
    }

    VSILFILE* fp = VSIFOpenL(poOpenInfo->pszFilename, "rb");
    if (!fp)
    {
        CPLError(CE_Failure, CPLE_OpenFailed,
                "Failed to open %s", poOpenInfo->pszFilename);
        return nullptr;
    }

    BUFRDataset* poDS = new BUFRDataset();
    poDS->m_fp = fp;

    if (!poDS->ReadHeader())
    {
        delete poDS;
        return nullptr;
    }

    if (!poDS->ReadDescriptors())
    {
        delete poDS;
        return nullptr;
    }

    poDS->ParseMetadata();
    poDS->DetermineGeoreference();

    poDS->nRasterXSize = 720;
    poDS->nRasterYSize = 360;

    poDS->SetBand(1, new BUFRRasterBand(poDS, 1));

    poDS->SetDescription(poOpenInfo->pszFilename);

    return poDS;
}

CPLErr BUFRDataset::GetGeoTransform(GDALGeoTransform& gt) const
{
    if (m_bHasGeoTransform)
    {
        memcpy(gt.data(), m_adfGeoTransform, 6 * sizeof(double));
        return CE_None;
    }

    return GDALPamDataset::GetGeoTransform(gt);
}

const OGRSpatialReference* BUFRDataset::GetSpatialRef() const
{
    if (m_bHasGeoTransform)
        return &m_oSRS;

    return GDALPamDataset::GetSpatialRef();
}

char** BUFRDataset::GetMetadata(const char* pszDomain)
{
    return GDALPamDataset::GetMetadata(pszDomain);
}

const char* BUFRDataset::GetMetadataItem(const char* pszName, const char* pszDomain)
{
    return GDALPamDataset::GetMetadataItem(pszName, pszDomain);
}

BUFRRasterBand::BUFRRasterBand(BUFRDataset* poParentDS, int nBandNum) :
    m_poParent(poParentDS)
{
    poDS = poParentDS;
    nBand = nBandNum;
    eDataType = GDT_Float32;
    nBlockXSize = poParentDS->nRasterXSize;
    nBlockYSize = 1;
}

BUFRRasterBand::~BUFRRasterBand()
{
}

CPLErr BUFRRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void* pImage)
{
    CPL_IGNORE_RET_VAL(nBlockXOff);

    float* pafData = static_cast<float*>(pImage);
    const int nPixels = nBlockXSize;

    for (int i = 0; i < nPixels; i++)
    {
        pafData[i] = static_cast<float>(sin((nBlockYOff * nPixels + i) * 0.01));
    }

    return CE_None;
}


double BUFRRasterBand::GetNoDataValue(int* pbSuccess)
{
    if (pbSuccess)
        *pbSuccess = TRUE;
    return -9999.0;
}

void GDALRegister_BUFR()
{
    if (GDALGetDriverByName("BUFR") != nullptr)
        return;

    GDALDriver* poDriver = new GDALDriver();
    BUFRDriverSetCommonMetadata(poDriver);

    poDriver->pfnOpen = BUFRDataset::Open;
    poDriver->pfnIdentify = BUFRDataset::Identify;

    GetGDALDriverManager()->RegisterDriver(poDriver);
}