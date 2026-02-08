#!/bin/bash
# Generate map data and move to project root

python3 src/tools/mapconverter.py \
    --mapfile mapdata/ne_10m_admin_1_states_provinces.shp \
    --mapnames mapdata/ne_10m_populated_places.shp \
    --airportfile mapdata/Runways.shp \
    --icao-airportnames src/tools/mapdata/airports.csv \
    --minpop 10000 \
    --output-dir .

echo "Map data generation complete - files moved to project root"