#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include "json.hpp"
#include <vector>
#include <cstdio>
#include <filesystem>
#include <cstdint>
#include <variant>

using namespace std;
using json = nlohmann::ordered_json;
namespace fs = filesystem;

struct dkm_header_t
{
    int ident;			//should be DKMD
    int version;		//should be 1 or 2
    float origin[3];
    int frameSize;
    int numSkins;
    int numVertices;
    int numTexCoords;
    int numTriangles;
    int numGlCommands;
    int numFrames;
    int numSurfaces;

    int offsetSkins; // each skin is char[64]
    int offsetTexCoords;
    int offsetTriangles;
    int offsetFrames;
    int offsetGlCommands;
    int offsetSurfaces;
    int offsetEnd;

    int numSequences;
    int offsetSequences;
};

struct dkm_textureCoordinate_t
{
    short s, t;
};

struct dkm_triangle_t
{
    short surfaceIndex;
    short num_uvframes;
    short vertexIndices[3];
    short textureIndices[3];
};

struct dkm_triangleVertex_t
{
    uint16_t vertex[3]; // v1 uses bytes, v2 compresses them all into one big uint32_t (11 bits - 10 bits - 11 bits
    uint8_t lightNormalIndex;
};

struct dkm_frame_t
{
    float scale[3];
    float translate[3];
    string name; //16 character limit
    vector <dkm_triangleVertex_t> vertices;
    uint8_t unknown[3];
};

struct dkm_glCommandVertex_t {
    int vertexIndex;
    float s, t;
};

struct dkm_glCommand_t {
    int count; //positive for strip, negative for fan
    int skinIndex;
    int surfIndex;
    vector<dkm_glCommandVertex_t> vertices;
};

struct dkm_surface_t
{
    string name; //32 character limit
    int flags;
    int skinIndex;
    int skinWidth;
    int skinHeight;
    int numUVframes;
};

struct dkm_animSeq_t
{
    string name; //16 character limit
    int startFrame;
    int endFrame;
};

struct DKM_file {
    dkm_header_t header;
    vector<string> skinpaths;
    vector<dkm_textureCoordinate_t> UV;
    vector<dkm_triangle_t> triangles;
    vector<dkm_frame_t> frames;
    vector<dkm_glCommand_t> GLCommands;
    vector<dkm_surface_t> surfaces;
    vector<dkm_animSeq_t> sequences;
};

extern void JSON2DKM(fs::path inpath, fs::path outpath, json jsonDKM);
extern DKM_file ParseDKM(fs::path inpath);
extern void DKM2JSON(const DKM_file& NewDKM, fs::path outpath);