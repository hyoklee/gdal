#!/usr/bin/env python3
# -*- coding: utf-8 -*-
###############################################################################
# Project:  GDAL/OGR Test Suite
# Purpose:  Test CDF driver
# Author:   Claude AI Assistant
#
###############################################################################
# Copyright (c) 2025, Claude AI Assistant
#
# SPDX-License-Identifier: MIT
###############################################################################

import struct
import tempfile
import os

import gdaltest
import pytest

from osgeo import gdal


def test_cdf_identify():
    """Test CDF format identification"""

    # Create a minimal CDF file for testing (CDF magic numbers)
    cdf_data = struct.pack('>II', 0xCDF26002, 0x0000FFFF) + b'\x00' * 40

    with tempfile.NamedTemporaryFile(suffix='.cdf', delete=False) as f:
        f.write(cdf_data)
        f.flush()
        temp_path = f.name

    try:
        # Test driver identification
        driver = gdal.IdentifyDriver(temp_path)
        assert driver is not None, "Driver should be identified"
        assert driver.GetDescription() == "CDF", f"Expected CDF driver, got {driver.GetDescription()}"
    finally:
        os.unlink(temp_path)


def test_cdf_open():
    """Test opening a CDF file"""

    # Create a more complete CDF file for testing
    cdf_header = struct.pack('>II', 0xCDF26002, 0x0000FFFF)  # Magic numbers
    cdf_header += struct.pack('>IIIIIIIIII', 0, 0, 2, 0, 0, 0, 0, 0, 0, 0)  # Rest of header
    cdf_data = cdf_header + b'\x00' * 8  # Additional padding

    with tempfile.NamedTemporaryFile(suffix='.cdf', delete=False) as f:
        f.write(cdf_data)
        f.flush()
        temp_path = f.name

    try:
        # Test opening the file
        ds = gdal.Open(temp_path)
        assert ds is not None, "Should be able to open CDF file"

        # Check basic properties
        assert ds.RasterXSize > 0, "Should have valid X size"
        assert ds.RasterYSize > 0, "Should have valid Y size"
        assert ds.RasterCount >= 1, "Should have at least one band"

        # Check metadata
        metadata = ds.GetMetadata("CDF")
        assert metadata is not None, "Should have CDF metadata"
        assert "CDF_NUM_ZVARS" in metadata, "Should have ZVars metadata"

        # Test first band
        band = ds.GetRasterBand(1)
        assert band is not None, "Should have first band"
        assert band.DataType == gdal.GDT_Float32, "Should be Float32 data type"

        ds = None
    finally:
        os.unlink(temp_path)


def test_cdf_read_data():
    """Test reading data from a CDF file"""

    # Create test CDF file with variables
    cdf_header = struct.pack('>II', 0xCDF26002, 0x0000FFFF)
    cdf_header += struct.pack('>IIIIIIIIII', 100, 0, 3, 5, 100, 2, 1, 0, 0, 0)
    cdf_data = cdf_header + b'\x00' * 8

    with tempfile.NamedTemporaryFile(suffix='.cdf', delete=False) as f:
        f.write(cdf_data)
        f.flush()
        temp_path = f.name

    try:
        ds = gdal.Open(temp_path)
        assert ds is not None

        # Should have multiple bands for multiple variables
        assert ds.RasterCount >= 1, "Should have at least one band"

        band = ds.GetRasterBand(1)
        data = band.ReadAsArray()
        assert data is not None, "Should be able to read data"
        assert data.shape[0] > 0, "Should have valid data shape"

        # Check NoData value
        nodata = band.GetNoDataValue()
        assert nodata == -999.0, f"Expected NoData -999.0, got {nodata}"

        # Check band description
        description = band.GetDescription()
        assert description is not None, "Should have band description"

        ds = None
    finally:
        os.unlink(temp_path)


def test_cdf_multiple_bands():
    """Test CDF files with multiple variables/bands"""

    # Create CDF file with multiple variables
    cdf_header = struct.pack('>II', 0xCDF26002, 0x0000FFFF)
    cdf_header += struct.pack('>IIIIIIIIII', 50, 0, 5, 3, 200, 3, 1, 0, 0, 0)
    cdf_data = cdf_header + b'\x00' * 8

    with tempfile.NamedTemporaryFile(suffix='.cdf', delete=False) as f:
        f.write(cdf_data)
        f.flush()
        temp_path = f.name

    try:
        ds = gdal.Open(temp_path)
        assert ds is not None

        # Should have multiple bands
        assert ds.RasterCount > 1, "Should have multiple bands for multiple variables"

        # Test each band
        for i in range(1, min(ds.RasterCount + 1, 4)):  # Test up to 3 bands
            band = ds.GetRasterBand(i)
            assert band is not None, f"Should have band {i}"

            # Each band should have different data patterns
            data = band.ReadAsArray()
            assert data is not None, f"Should be able to read data from band {i}"

        ds = None
    finally:
        os.unlink(temp_path)


def test_cdf_invalid_file():
    """Test handling of invalid CDF files"""

    # Create invalid file (wrong magic numbers)
    with tempfile.NamedTemporaryFile(suffix='.cdf', delete=False) as f:
        f.write(b'INVALID_MAGIC_NUMBERS_AND_DATA')
        f.flush()
        temp_path = f.name

    try:
        # Should not be identified as CDF
        driver = gdal.IdentifyDriver(temp_path)
        if driver is not None:
            assert driver.GetDescription() != "CDF", "Invalid file should not be identified as CDF"

        # Should fail to open
        ds = gdal.Open(temp_path)
        if ds is not None:
            # If it opens, it shouldn't be with CDF driver
            assert ds.GetDriver().GetDescription() != "CDF"

    finally:
        os.unlink(temp_path)


def test_cdf_georeference():
    """Test CDF georeference detection"""

    # Create CDF file (basic structure)
    cdf_header = struct.pack('>II', 0xCDF26002, 0x0000FFFF)
    cdf_header += struct.pack('>IIIIIIIIII', 0, 0, 1, 0, 0, 0, 0, 0, 0, 0)
    cdf_data = cdf_header + b'\x00' * 8

    with tempfile.NamedTemporaryFile(suffix='.cdf', delete=False) as f:
        f.write(cdf_data)
        f.flush()
        temp_path = f.name

    try:
        ds = gdal.Open(temp_path)
        assert ds is not None

        # Check for geotransform
        gt = ds.GetGeoTransform()
        assert gt is not None, "Should have geotransform"

        # Check for spatial reference
        srs = ds.GetSpatialRef()
        if srs is not None:
            # If we have SRS, it should be valid
            assert srs.IsGeographic() or srs.IsProjected(), "SRS should be geographic or projected"

        ds = None
    finally:
        os.unlink(temp_path)


def test_cdf_metadata():
    """Test CDF metadata extraction"""

    # Create CDF with attributes
    cdf_header = struct.pack('>II', 0xCDF26002, 0x0000FFFF)
    cdf_header += struct.pack('>IIIIIIIIII', 10, 0, 2, 3, 50, 2, 1, 0, 0, 0)
    cdf_data = cdf_header + b'\x00' * 8

    with tempfile.NamedTemporaryFile(suffix='.cdf', delete=False) as f:
        f.write(cdf_data)
        f.flush()
        temp_path = f.name

    try:
        ds = gdal.Open(temp_path)
        assert ds is not None

        # Check CDF-specific metadata
        metadata = ds.GetMetadata("CDF")
        assert metadata is not None, "Should have CDF metadata domain"

        # Should have format-specific metadata
        assert "CDF_NUM_RECORDS" in metadata, "Should have record count"
        assert "CDF_NUM_ZVARS" in metadata, "Should have variable count"
        assert "CDF_NUM_ATTRIBUTES" in metadata, "Should have attribute count"

        ds = None
    finally:
        os.unlink(temp_path)


if __name__ == "__main__":
    pytest.main([__file__])