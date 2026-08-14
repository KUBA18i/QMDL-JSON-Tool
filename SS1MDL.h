#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include "json.hpp"
#include <vector>
#include <filesystem>
#include <cstdint>
#include <cstring>
#include <array>
#include <optional>

using namespace std;
using json = nlohmann::ordered_json;
namespace fs = filesystem;

struct ss1_mdl_Vertex8 {
    int8_t x, y, z;
    uint8_t normalIndex;
};

struct ss1_mdl_Vertex16 {
    int16_t x, y, z;
    uint8_t normH, normP;
};

struct ss1_mdl_Vertex16_old {
    int16_t x, y, z;
    uint8_t normalIndex;
};

struct ss1_mdl_ModelFrameInfo {
    float MinX, MinY, MinZ;
    float MaxX, MaxY, MaxZ;
};

struct ss1_mdl_MipVertex {
    float x, y, z;
};

struct ss1_mdl_TextureVertex {
    bool newformat = false;
    float uvwX, uvwY, uvwZ;
    float u, v;
    //present if bHasPolygonsPerSurface is false, or the old format is used
    bool done;
    //present if bHasPolygonsPerSurface is true
    uint32_t SurfaceIndex;
    uint32_t transformedVertexIndex;
    float bumpU[3];
    float bumpV[3];
};

struct ss1_mdl_PolygonVertex {
    uint32_t transformedVertexIndex;
    uint32_t textureVertexIndex;
};

struct ss1_mdl_Polygon {
    bool newformat;
    vector<ss1_mdl_PolygonVertex> vertices;
    uint32_t renderFlags;
    uint32_t colorAndAlpha;
    uint32_t surfaceIndex;
    //below is stored in MDPL format and not actually used.
    uint32_t exONcolor;
    uint32_t exOFFcolor;
};

struct ss1_mdl_MappingSurface {
    string name;
    float surfaceOffsetX;
    float surfaceOffsetY;
    float surfaceOffsetZ;
    float h, p, b;
    float zoom;

    //loaded if bHasPolygonsPerSurface is true
    int32_t shadingType;
    int32_t translucencyType;
    uint32_t renderingFlags;
    vector<uint32_t> polygonIndices;
    vector<uint32_t> textureVertexIndices;
    uint32_t color;

    //loaded if bHasDiffuseColor is true
    uint32_t diffuseColor;
    uint32_t reflectionColor;
    uint32_t specularColor;
    uint32_t bumpColor;
    uint32_t onColorMask;
    uint32_t offColorMask;
};

struct ss1_mdl_PolygonsPerPatch {
    int32_t ctOccupied;
    vector<int32_t> PolygonIndices;
};

struct ss1_mdl_Mip {
    vector<ss1_mdl_Polygon> polygons;
    vector<ss1_mdl_TextureVertex> textureVertices;
    vector<ss1_mdl_MappingSurface> mappingSurfaces;
    //loaded if bHasPolygonalPatches is true
    uint32_t flags;
    vector<ss1_mdl_PolygonsPerPatch> patches;
};

struct ss1_mdl_Patch {
    string name; //present in new format
    string textureName; //sometimes present in old format
    int32_t posU;
    int32_t posV;
    float stretch; //present in new format
};

struct ss1_mdl_CollisionBox {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
    string name;
};

struct ss1_mdl_AttachedPosition {
    int32_t centerVertex;
    int32_t frontVertex;
    int32_t upVertex;
    float Position[3];
    float Angle[3];
};

struct ss1_mdl_AnimData {
    char name[32];
    float SecondsPerFrame;
    vector<int32_t> FrameIndices;
};


struct SS1_MDL_file {
    string version;
    uint32_t flags;

    uint32_t verticesCount;
    uint32_t framesCount;

    vector<ss1_mdl_Vertex8> frameVertices8;
    vector<ss1_mdl_Vertex16> frameVertices16;
    vector<ss1_mdl_Vertex16_old> frameVertices16_old;

    vector<ss1_mdl_ModelFrameInfo> frameInfos;
    vector<ss1_mdl_MipVertex> mainMipVertices;

    vector<uint32_t> vertexMipMask;

    uint32_t mipCount;
    float mipSwitchFactors[32];
    vector<ss1_mdl_Mip> mips;

    bool NewPatchFormat;
    ss1_mdl_Patch patches[32];
    int32_t texWidth;
    int32_t texHeight;

    int32_t shadowQuality;
    float stretch[3];
    float center[3];

    vector<ss1_mdl_CollisionBox> collisionBoxes;
    bool collideAsCube;
    vector<ss1_mdl_AttachedPosition> attachedPositions;
    string colorNames[32];
    
    vector<ss1_mdl_AnimData> animations;

    //loaded if bHasDiffuseColor is true
    uint32_t colorDiffuse;
    //loaded if bHasColorForReflectionAndSpecularity is true
    uint32_t colorReflections;
    uint32_t colorSpecular;
    uint32_t colorBump;
};

extern void JSON2SS1MDL(const fs::path& inpath, const fs::path& outpath, const json& jsonMDL);
extern SS1_MDL_file ParseSS1MDL(const fs::path& filePath);
extern void SS1MDL2JSON(const SS1_MDL_file& MDL, const fs::path& outPath);