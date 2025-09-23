add_gdal_driver(TARGET gdal_CDF
  SOURCES cdfdataset.cpp
  CORE_SOURCES cdfdrivercore.cpp
  PLUGIN_CAPABLE
)
gdal_standard_includes(gdal_CDF)