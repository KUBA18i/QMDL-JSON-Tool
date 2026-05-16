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

/* Vector */
typedef float h2pop_vec3_t[3];

/* MDL header */
struct h2pop_mdl_header_t
{
    int ident;            /* magic number: "RAPO" */
    int version;          /* version: 50 */

    h2pop_vec3_t scale;         /* scale factor */
    h2pop_vec3_t translate;     /* translation vector */
    float boundingradius;
    h2pop_vec3_t eyeposition;   /* eyes' position */

    int num_skins;        /* number of textures */
    int skinwidth;        /* texture width */
    int skinheight;       /* texture height */

    int num_verts;        /* number of vertices */
    int num_tris;         /* number of triangles */
    int num_frames;       /* number of frames */

    int synctype;         /* 0 = synchron, 1 = random */
    int flags;            /* state flag */
    float size;

    int num_st_verts; //RAPO addition
};

/* Skin */
struct h2pop_mdl_skin_t
{
    int group;      /* 0 = single, 1 = group */
    vector<uint8_t> data;  /* texture data */
};
/* Group of pictures */
struct h2pop_mdl_groupskin_t
{
    int group;
    int nb;
    vector<float> time;
    vector<vector<uint8_t>> frames;
};
using h2pop_SkinElement = variant<h2pop_mdl_skin_t, h2pop_mdl_groupskin_t>;

/* Texture coords */
struct h2pop_mdl_texcoord_t
{
    int onseam;
    int s;
    int t;
};

/* Triangle info */
struct h2pop_mdl_triangle_t
{
    int	facesfront;
    unsigned short	vertindex[3];
    unsigned short	stindex[3];
}; //this is different in RAPO

/* Compressed vertex */
struct h2pop_mdl_vertex_t
{
    unsigned char v[3];
    unsigned char normalIndex;
};

/* Simple frame */
struct h2pop_mdl_simpleframe_t
{
    struct h2pop_mdl_vertex_t bboxmin; /* bouding box min */
    struct h2pop_mdl_vertex_t bboxmax; /* bouding box max */
    char name[16];
    vector<h2pop_mdl_vertex_t> verts;  /* vertex list of the frame */
};
/* Model frame */
struct h2pop_mdl_frame_t
{
    int type;                        /* 0 = simple, !0 = group */
    struct h2pop_mdl_simpleframe_t frame;
};
/* Group of simple frames */
struct h2pop_mdl_groupframe_t
{
    int type;                         /* !0 = group */
    int nb;                           /* number of frames in group */
    struct h2pop_mdl_vertex_t min;          /* min pos in all simple frames */
    struct h2pop_mdl_vertex_t max;          /* max pos in all simple frames */
    vector<float> time;                      /* time duration for each frame */
    vector<h2pop_mdl_simpleframe_t> frames; /* simple frame list */
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