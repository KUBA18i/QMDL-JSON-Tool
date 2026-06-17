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

struct kp_mdx_header_t
{
    int magic; //should be 1481655369,aka "IDPX"
    int version; //should be 4
    
    int skinWidth;
    int skinHeight;

    int frameSize;
    
    int numSkins;
    int numVertices;
    int numTriangles;
    int numGlCommands;
    int numFrames;
    int numSfxDefines;
    int numSfxEntries;
    int numSubObjects;

    int offsetSkins;
    int offsetTriangles;
    int offsetFrames;
    int offsetGlCommands;
    int offsetVertexInfo;
    int offsetSfxDefines;
    int offsetSfxEntries;
    int offsetBBoxFrames;
    int offsetDummyEnd; //same as offsetEnd
    int offsetEnd;
};

struct kp_mdx_triangleVertex_t
{
    byte vertex[3];
    byte lightNormalIndex;
};

struct kp_mdx_frame_t
{
    float scale[3];
    float translate[3];
    char name[16];
    vector <kp_mdx_triangleVertex_t> vertices;
};

struct kp_mdx_triangle_t
{
    short vertexIndices[3];
    short textureIndices[3];
};

struct kp_mdx_glCommandVertex_t {
    float s, t;
    int vertexIndex;
};

struct kp_mdx_glCommand_t {
    int count; // positive for strip, negative for fan
    int SubObjectID;
    vector<kp_mdx_glCommandVertex_t> vertices;
};

struct kp_mdx_sfxDefine_t
{
    int type;
    int flags;
    int velocity_type;
    int velocity_speed_up;
    float gravity;
    int spawn_interval;
    float random_spawn_interval;
    float start_alpha;
    float end_alpha;
    float fadein_time;
    float lifetime;
    float random_time_scale;
    float start_width;
    float end_width;
    float start_height;
    float end_height;
    float random_size_scale;
};

struct kp_mdx_sfxEntry_t
{
    int index;
    int define_no;
    int vertexindex;
    uint8_t SfxFrames[128];
};

struct kp_mdx_BBox_t
{
    float MinX;
    float MinY;
    float MinZ;
    float MaxX;
    float MaxY;
    float MaxZ;
};

struct kp_mdx_BFrames_t
{
    vector<kp_mdx_BBox_t> BoxFrames;
};

struct kp_mdx_file {
    kp_mdx_header_t header;
    vector<string> skinpaths;
    vector<kp_mdx_triangle_t> triangles;
    vector<kp_mdx_frame_t> frames;
    vector<kp_mdx_glCommand_t> GLCommands;
    vector<int> VertexInfo;
    vector<kp_mdx_sfxDefine_t> SfxDefintions;
    vector<kp_mdx_sfxEntry_t> SfxEntries;
    vector<kp_mdx_BFrames_t> BBoxFrames;
};

extern void JSON2KPMDX(fs::path inpath, fs::path outpath, json jsonMDX);
extern kp_mdx_file ParseKPMDX(fs::path inpath);
extern void KPMDX2JSON(const kp_mdx_file& NewMDX, fs::path outpath);