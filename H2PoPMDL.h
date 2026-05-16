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

struct h2pop_mdl_header_t
{
    int ident; //should be "RAPO"
    int version; //should be 50

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

    int syncType; //0 = synchronised, 1 = random
    int flags;
    float size;

    int numStVerts; //RAPO addition
};

struct h2pop_mdl_skin_t
{
    int group; //0 = single, 1 = group
    vector<uint8_t> data;  /* texture data */
};

struct h2pop_mdl_groupskin_t
{
    int group;
    int nb;
    vector<float> time;
    vector<vector<uint8_t>> frames;
};
using h2pop_SkinElement = variant<h2pop_mdl_skin_t, h2pop_mdl_groupskin_t>;

struct h2pop_mdl_texcoord_t
{
    int onseam;
    int s;
    int t;
};

struct h2pop_mdl_triangle_t
{
    int	facesfront;
    uint16_t vertindex[3];
    uint16_t stindex[3];
}; //this is different in RAPO

struct h2pop_mdl_vertex_t
{
    uint8_t v[3];
    uint8_t normalIndex;
};

struct h2pop_mdl_simpleframe_t
{
    struct h2pop_mdl_vertex_t bboxmin;
    struct h2pop_mdl_vertex_t bboxmax;
    char name[16];
    vector<h2pop_mdl_vertex_t> verts;
};

struct h2pop_mdl_frame_t
{
    int type; //0 = simple, 1 = group
    struct h2pop_mdl_simpleframe_t frame;
};

struct h2pop_mdl_groupframe_t
{
    int type; //1 = group
    int nb;
    struct h2pop_mdl_vertex_t min;
    struct h2pop_mdl_vertex_t max;
    vector<float> time; //time duration for each frame
    vector<h2pop_mdl_simpleframe_t> frames;
};
using h2pop_FrameElement = variant<h2pop_mdl_frame_t, h2pop_mdl_groupframe_t>;

struct H2PoP_MDL_file {
    h2pop_mdl_header_t header;
    vector<h2pop_SkinElement> allskins;
    vector<h2pop_mdl_texcoord_t> UV;
    vector<h2pop_mdl_triangle_t> triangles;
    vector<h2pop_FrameElement> allframes;
};

extern void JSON2H2PoPMDL(fs::path inpath, fs::path outpath, json jsonMDL);
extern H2PoP_MDL_file ParseH2PoPMDL(fs::path inpath);
extern void H2PoPMDL2JSON(const H2PoP_MDL_file& NewMDL, fs::path outpath);