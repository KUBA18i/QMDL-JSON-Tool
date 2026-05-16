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

struct q3_md3_header_t
{
    int ident; //should be 1367369843,aka "IDP3"
    int version; //should be 15
    
    string modelname; //64 character limit
    
    int flags;
    int numFrames;
    int numTags;
    int numSurfaces;
    int numSkins;

    int offsetFrames;
    int offsetTags;
    int offsetSurfaces;
    int offsetEnd;
};

typedef float q3_vec3_t[3];

struct q3_md3_frame_t
{
    q3_vec3_t minBounds;
    q3_vec3_t maxBounds;
    q3_vec3_t localOrigin;
    float radius;
    string name; //16 character limit
};

struct q3_md3_tag_t
{
    string name; //64 character limit
    q3_vec3_t origin;
    float axis[9];
};

struct q3_md3_tagCollection_t
{
    vector<q3_md3_tag_t> tags;
}; //should be equal to the amount of frames

struct q3_md3_shader_t {
    string name; //64 character limit
    int shaderIndex;
};

struct q3_md3_triangle_t
{
    int indices[3];
};

struct q3_md3_texCoord_t
{
    float s, t;
};

struct q3_md3_vertex_t
{
    short x;
    short y;
    short z;
    short lightNormalIndex;
};

struct q3_md3_vertexFrame_t
{
    vector<q3_md3_vertex_t> vertices;
}; //should be equal to the amount of frames

struct q3_md3_surface_t
{
    int ident; //should be 1367369843,aka "IDP3"
    string surfacename; //64 character limit
    int flags;

    int numFrames;
    int numShaders;
    int numVerts;
    int numTriangles;

    int offsetTriangles;
    int offsetShaders;
    int offsetUV;
    int offsetVertices;
    int offsetEnd;

    vector<q3_md3_shader_t> shaders;
    vector<q3_md3_triangle_t> triangles;
    vector<q3_md3_texCoord_t> UV;
    vector<q3_md3_vertexFrame_t> vertexFrames;
};

struct Q3_MD3_file {
    q3_md3_header_t header;
    vector<q3_md3_frame_t> frames;
    vector<q3_md3_tagCollection_t> tagframes;
    vector<q3_md3_surface_t> surfaces;
};

extern void JSON2Q3MD3(fs::path inpath, fs::path outpath, json jsonMD2);
extern Q3_MD3_file ParseQ3MD3(fs::path inpath);
extern void Q3MD32JSON(const Q3_MD3_file& NewMD2, fs::path outpath);