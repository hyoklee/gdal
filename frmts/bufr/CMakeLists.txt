add_gdal_driver(TARGET gdal_BUFR
  SOURCES bufrdataset.cpp
  CORE_SOURCES bufrdrivercore.cpp
  PLUGIN_CAPABLE
  NO_SHARED_SYMBOL_WITH_CORE
)
gdal_standard_includes(gdal_BUFR)