#!/usr/bin/env python3
# -*- coding: utf-8 -*-
###############################################################################
# Project:  GDAL/OGR Test Suite
# Purpose:  Test BUFR driver
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


def test_bufr_identify():
    """Test BUFR format identification"""

    # Create a minimal BUFR file for testing
    bufr_data = b'BUFR' + b'\x00\x00\x00\x1A' + b'\x03' + b'\x00' * 17

    with tempfile.NamedTemporaryFile(suffix='.bufr', delete=False) as f:
        f.write(bufr_data)
        f.flush()
        temp_path = f.name

    try:
        # Test driver identification
        driver = gdal.IdentifyDriver(temp_path)
        assert driver is not None, "Driver should be identified"
        assert driver.GetDescription() == "BUFR", f"Expected BUFR driver, got {driver.GetDescription()}"
    finally:
        os.unlink(temp_path)


def test_bufr_open():
    """Test opening a BUFR file"""

    # Create a more complete BUFR file for testing
    bufr_header = struct.pack('>4sI', b'BUFR', 26)  # Header + message length
    bufr_edition = struct.pack('B', 3)  # Edition 3
    bufr_rest = b'\x00' * 17  # Rest of header
    bufr_data = bufr_header + bufr_edition + bufr_rest

    with tempfile.NamedTemporaryFile(suffix='.bufr', delete=False) as f:
        f.write(bufr_data)
        f.flush()
        temp_path = f.name

    try:
        # Test opening the file
        ds = gdal.Open(temp_path)
        assert ds is not None, "Should be able to open BUFR file"

        # Check basic properties
        assert ds.RasterXSize > 0, "Should have valid X size"
        assert ds.RasterYSize > 0, "Should have valid Y size"
        assert ds.RasterCount >= 1, "Should have at least one band"

        # Check metadata
        metadata = ds.GetMetadata("BUFR")
        assert metadata is not None, "Should have BUFR metadata"
        assert "BUFR_EDITION" in metadata, "Should have edition metadata"

        # Test first band
        band = ds.GetRasterBand(1)
        assert band is not None, "Should have first band"
        assert band.DataType == gdal.GDT_Float32, "Should be Float32 data type"

        ds = None
    finally:
        os.unlink(temp_path)


def test_bufr_read_data():
    """Test reading data from a BUFR file"""

    # Create test BUFR file
    bufr_header = struct.pack('>4sI', b'BUFR', 50)
    bufr_edition = struct.pack('B', 4)  # Edition 4
    bufr_rest = b'\x00' * 41  # Rest of data
    bufr_data = bufr_header + bufr_edition + bufr_rest

    with tempfile.NamedTemporaryFile(suffix='.bufr', delete=False) as f:
        f.write(bufr_data)
        f.flush()
        temp_path = f.name

    try:
        ds = gdal.Open(temp_path)
        assert ds is not None

        band = ds.GetRasterBand(1)
        data = band.ReadAsArray()
        assert data is not None, "Should be able to read data"
        assert data.shape[0] > 0, "Should have valid data shape"

        # Check NoData value
        nodata = band.GetNoDataValue()
        assert nodata == -9999.0, f"Expected NoData -9999.0, got {nodata}"

        ds = None
    finally:
        os.unlink(temp_path)


def test_bufr_invalid_file():
    """Test handling of invalid BUFR files"""

    # Create invalid file (not starting with BUFR)
    with tempfile.NamedTemporaryFile(suffix='.bufr', delete=False) as f:
        f.write(b'INVALID_DATA')
        f.flush()
        temp_path = f.name

    try:
        # Should not be identified as BUFR
        driver = gdal.IdentifyDriver(temp_path)
        if driver is not None:
            assert driver.GetDescription() != "BUFR", "Invalid file should not be identified as BUFR"

        # Should fail to open
        ds = gdal.Open(temp_path)
        if ds is not None:
            # If it opens, it shouldn't be with BUFR driver
            assert ds.GetDriver().GetDescription() != "BUFR"

    finally:
        os.unlink(temp_path)


def test_bufr_extensions():
    """Test BUFR file extension handling"""

    bufr_data = b'BUFR' + b'\x00\x00\x00\x1A' + b'\x03' + b'\x00' * 17

    # Test .bufr extension
    with tempfile.NamedTemporaryFile(suffix='.bufr', delete=False) as f:
        f.write(bufr_data)
        f.flush()
        temp_path = f.name

    try:
        driver = gdal.IdentifyDriver(temp_path)
        assert driver is not None
        assert driver.GetDescription() == "BUFR"
    finally:
        os.unlink(temp_path)

    # Test .bfr extension
    with tempfile.NamedTemporaryFile(suffix='.bfr', delete=False) as f:
        f.write(bufr_data)
        f.flush()
        temp_path = f.name

    try:
        driver = gdal.IdentifyDriver(temp_path)
        assert driver is not None
        assert driver.GetDescription() == "BUFR"
    finally:
        os.unlink(temp_path)


if __name__ == "__main__":
    pytest.main([__file__])