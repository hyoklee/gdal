add_gdal_driver(TARGET gdal_CDF
  SOURCES cdfdataset.cpp
  CORE_SOURCES cdfdrivercore.cpp
  PLUGIN_CAPABLE
  NO_SHARED_SYMBOL_WITH_CORE
)
gdal_standard_includes(gdal_CDF)