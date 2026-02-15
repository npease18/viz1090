#!/bin/bash
# Generate map data and move to project root

python3 src/tools/mapconverter.py \
    --mapfile src/tools/mapdata/ne_10m_admin_1_states_provinces.shp \
    --mapnames src/tools/mapdata/ne_10m_populated_places.shp \
    --airportfile src/tools/mapdata/Runways.shp \
    --icao-airportnames src/tools/mapdata/airports.csv \
    --landfile src/tools/mapdata/ne_10m_land.shp \
    --minpop 10000 \
    --output-dir .

echo "Map data generation complete - files moved to project root"

    # --countryfile src/tools/mapdata/ne_10m_admin_0_boundary_lines_land.shp \
    # --disputedfile src/tools/mapdata/ne_10m_admin_0_boundary_lines_disputed_areas.shp \
    # --coastlinefile src/tools/mapdata/ne_10m_coastline.shp \
    # --riverfile src/tools/mapdata/ne_10m_rivers_lake_centerlines.shp \
    # --lakefile src/tools/mapdata/ne_10m_rivers_lake_centerlines.shp \