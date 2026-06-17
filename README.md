# QMDL-JSON-Tool
Convert Quake family model formats to JSON and vice versa.

# Purpose
I made this mainly to better understand the model formats themselves, and by chance also write C++ parsers and loaders for them, so they can be used as a basis for other programs.

As for the practical uses, the user has the ability to directly edit the model without fear of damaging the integrity of file, like with a hex editor. The program is best suited for small edits like skin paths or flags.

The reason why the JSON format script is in Python instead of C++ is because it uses RegEx to accomplish its task, and Python is simply much more efficient at doing so.

# Supported formats:
- Quake 1's MDL files (IDPO ident)
- Hexen 2: Portal of Praevus's MDL files (RAPO ident)
- Quake 2's MD2 files (IDP2 ident)
- Kingpin: Life of Crime's MDX files (IPDX ident)
- Daikatana's DKM files, both versions (DKMD ident)
- Quake 3's MD3 files (IDP3 ident)

# Usage manual
Drag and drop a model file to create a JSON file, and vice versa. If using the command line, you can just pass the path of the file you want to convert as an argument.

After you have a JSON file, you can use `format_json.py` to remove the unnecessary spaces and newlines from the file; this decreases the file size significantly. Doing this is optional though.

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
MDX format is by Xatrix Entertainment, aka Gray Matter Interactive.  
DKM format is by Ion Storm.  

# References
The information on how to parse the model files was loosely based on the following sources:
## Q1 MDL
- http://tfc.duke.free.fr/coding/mdl-specs-en.html
- https://formats.kaitai.io/quake_mdl/
- https://github.com/amroibrahim/DIYQuake/blob/main/Quake/Notes005/notes/README.md
- The official Quake 1 source code release.

## H2PoP MDL
- The source code of Hammer of Thyrion, a Hexen 2 source port.

## Q2 MD2
- http://tfc.duke.free.fr/coding/md2-specs-en.html
- https://icculus.org/~phaethon/q3a/formats/md2-schoenblum.html
- The official Quake 2 source code release.

## KP MDX
- https://wiki.kingpin.info/index.php/MDX

## DKM
- https://github.com/cholleme/DaikatanaTools/blob/main/DaiMdlV2/models.pas
- https://www.moddb.com/downloads/daikatana-to-quake-2-model-converter
- https://github.com/TrenchBroom/TrenchBroom/blob/c2ae5a5b48c345d06e315571cd4da30ef04748e5/lib/TbMdlLib/src/LoadDkmModel.cpp
- Assistance from the Beta 1.3 patch team.

## Q3 MD3
- https://icculus.org/homepages/phaethon/q3a/formats/md3format.html
