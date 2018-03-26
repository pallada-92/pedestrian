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

## Zones list

| Zone id |  Price  |      Name       |
| ------- | ------- | --------------- |
|    0    |   MAX   |     barriers    |
|   20    | MAX / 2 |  private gates  |
|   40    | MAX / 4 |    entrance     |
|   60    | MAX / 8 | special landuse |
|   80    |   40    |      grass      |
|   100   |   10    |     default     |
|   120   |   1     |     footway     |
|   128   | MAX / 16 | buildings of unknown number of floors |
|   129   | MAX / 16 | buildings with one floor |
|   130   | MAX / 16 | buildings with two floors |
|   ...   |   ...    |    ...         |

## Calculating paths

### Invocation

Invocation:

```
gcc calc_paths.c
echo "2700 2100 1107 1021" | ./a.out
```

The program expects input in the following format:
`width height cx cy`, where `width` and `height` are
width and height of the raster image `map.pgm`, `cx` and `cy` are
the (pixel) coordinates of the target point for path calculation.

Point `(1107, 1021)` corresponds to the Airport subway station.

### Data structure

The main data structure is a binary prefix tree where keys are binary representations of distances. All distances are considered to be integer numbers.
