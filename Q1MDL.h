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
#include <optional>
#include <variant>

using namespace std;
using json = nlohmann::ordered_json;
namespace fs = filesystem;

struct q1_mdl_header_t
{
    int ident; //should be "IDPO"
    int version; //should be 6

    float scale[3];
    float translate[3];
    float boundingRadius;
    float eyePosition[3];

    int numSkins;
    int skinWidth;
    int skinHeight;

    int numVerts;
    int numTris;
    int numFrames;

    int syncType; // 0 = synchronised, 1 = random
    int flags;
    float size;
};

struct q1_mdl_skin_t
{
    int group; // 0 = single, 1 = group
    vector<uint8_t> data; //texture data
};

struct q1_mdl_groupskin_t
{
    int group;
    int nb;
    vector<float> time;
    vector<vector<uint8_t>> frames;
};
using q1_SkinElement = variant<q1_mdl_skin_t, q1_mdl_groupskin_t>;

struct q1_mdl_texcoord_t
{
    int onseam;
    int s;
    int t;
};

struct q1_mdl_triangle_t
{
    int facesfront;  // 0 = backface, 1 = frontface
    int vertexIndices[3];
};

struct q1_mdl_vertex_t
{
    uint8_t v[3];
    uint8_t normalIndex;
};

struct q1_mdl_simpleframe_t
{
    struct q1_mdl_vertex_t bboxmin;
    struct q1_mdl_vertex_t bboxmax;
    char name[16];
    vector<q1_mdl_vertex_t> verts;
};

struct q1_mdl_frame_t
{
    int type; //0 = simple, 1 = group
    struct q1_mdl_simpleframe_t frame;
};

struct q1_mdl_groupframe_t
{
    int type; //0 = simple, 1 = group
    int nb; // number of frames in group
    struct q1_mdl_vertex_t min;
    struct q1_mdl_vertex_t max;
    vector<float> time; // time duration for each frame
    vector<q1_mdl_simpleframe_t> frames;
};
using q1_FrameElement = variant<q1_mdl_frame_t, q1_mdl_groupframe_t>;

struct Q1_MDL_file {
    q1_mdl_header_t header;
    vector<q1_SkinElement> allskins;
    vector<q1_mdl_texcoord_t> UV;
    vector<q1_mdl_triangle_t> triangles;
    vector<q1_FrameElement> allframes;
};

extern void JSON2Q1MDL(fs::path inpath, fs::path outpath, json jsonMDL);
extern Q1_MDL_file ParseQ1MDL(fs::path inpath);
extern void Q1MDL2JSON(const Q1_MDL_file& NewMDL, fs::path outpath);