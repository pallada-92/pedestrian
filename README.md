# Simple model of pedestrian traffic

By Yaroslav Sergienko.

## Description

Firstly, XML dump of target area is loaded from Open Street Map. After that raster map of pedestrian zones is generated. Examples of zones: footways, crosswalks, roads, grass, buildings, fence, water, gates (public or private). Each zone has corresponding cost of passing. And finally Dijkstra algorithm is applied to calculate shortest paths to some point, for example, subway station. This allows to calculate all shortest pedestrian paths from all buildings. 

## Files

* `aeroport.osm` --- XML map downloaded from OSM.
* `map2matrix.ipynb` --- Jupyter notebook, which generates `map.png` and `map.pgm` from `aeroport.osm`.
* `map.pgm` --- raster in binary format without header, just pixel values, one byte per pixel; width and height are provided through command-line.
* `map.png` --- png of previous file with zones encoded with colors for debug.
* `calc_paths.c` --- C source of program, which calculates shortest paths.

## Calculating paths

Invocation:

```
gcc calc_paths.c
echo "2700 2100 1107 1021" | ./a.out
```

The program expects input in the following format:
`width height cx cy`, where `width` and `height` are
width and height of the raster image `map.pgm`, `cx` and `cy` are
the (pixel) coordinates target point for path calculation.

