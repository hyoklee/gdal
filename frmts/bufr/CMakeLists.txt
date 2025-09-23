add_gdal_driver(TARGET gdal_BUFR
  SOURCES bufrdataset.cpp
  CORE_SOURCES bufrdrivercore.cpp
  PLUGIN_CAPABLE
)
gdal_standard_includes(gdal_BUFR)