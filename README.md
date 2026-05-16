# QMDL-JSON-Tool
Convert Quake family model formats to JSON and vice versa.

# Supported formats:
- [Drawing images with sectors is a popular practice in the Doom scene](https://doomwiki.org/wiki/Automap_art), so this software should definitely appeal to those that do it.
- If anyone wishes to create a Doom map based on a real location, then using an actual top down map, like provided by OpenStreetMaps, together with this software will make it easier to ensure all the objects are up to scale.
- It is likel

# Usage manual:
Drag and drop a model file to create a JSON file, and vice versa. If using the command line, you can just pass the path of the file you want to convert as an argument.


# Compile
If using MSVS, open the SLN. If on Linux, you can use the included shell script. Otherwise:
```
g++ main.cpp Q1MDL.cpp H2PoPMDL.cpp Q2MD2.cpp DKM.cpp Q3MD3.cpp -o QMDL-JSON-Tool_linux -I.
```
# Copyright
This program was made by Jakub Majewski.
JSON parsing library was made by Niels Lohmann.
Quake was originally made by iD Software; its engine and model format was mostly devised by John Carmack. Most formats here are derivatives of it.
RAPO format is by Raven Software.
DKM format is by Ion Storm.

# References
## Quake MDL

## Hexen ][: Portals of Praevus