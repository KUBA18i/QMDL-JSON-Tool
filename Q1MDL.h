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

/* Vector */
typedef float q1_vec3_t[3];

/* MDL header */
struct q1_mdl_header_t
{
    int ident;            /* magic number: "IDPO" */
    int version;          /* version: 6 */

    q1_vec3_t scale;         /* scale factor */
    q1_vec3_t translate;     /* translation vector */
    float boundingradius;
    q1_vec3_t eyeposition;   /* eyes' position */

    int num_skins;        /* number of textures */
    int skinwidth;        /* texture width */
    int skinheight;       /* texture height */

    int num_verts;        /* number of vertices */
    int num_tris;         /* number of triangles */
    int num_frames;       /* number of frames */

    int synctype;         /* 0 = synchron, 1 = random */
    int flags;            /* state flag */
    float size;
};

/* Skin */
struct q1_mdl_skin_t
{
    int group;      /* 0 = single, 1 = group */
    vector<uint8_t> data;  /* texture data */
};
/* Group of pictures */
struct q1_mdl_groupskin_t
{
    int group;
    int nb;
    vector<float> time;
    vector<vector<uint8_t>> frames;
};
using q1_SkinElement = variant<q1_mdl_skin_t, q1_mdl_groupskin_t>;

/* Texture coords */
struct q1_mdl_texcoord_t
{
    int onseam;
    int s;
    int t;
};

/* Triangle info */
struct q1_mdl_triangle_t
{
    int facesfront;  /* 0 = backface, 1 = frontface */
    int vertex[3];   /* vertex indices */
};

/* Compressed vertex */
struct q1_mdl_vertex_t
{
    unsigned char v[3];
    unsigned char normalIndex;
};

/* Simple frame */
struct q1_mdl_simpleframe_t
{
    struct q1_mdl_vertex_t bboxmin; /* bouding box min */
    struct q1_mdl_vertex_t bboxmax; /* bouding box max */
    char name[16];
    vector<q1_mdl_vertex_t> verts;  /* vertex list of the frame */
};
/* Model frame */
struct q1_mdl_frame_t
{
    int type;                        /* 0 = simple, !0 = group */
    struct q1_mdl_simpleframe_t frame;
};
/* Group of simple frames */
struct q1_mdl_groupframe_t
{
    int type;                         /* !0 = group */
    int nb;                           /* number of frames in group */
    struct q1_mdl_vertex_t min;          /* min pos in all simple frames */
    struct q1_mdl_vertex_t max;          /* max pos in all simple frames */
    vector<float> time;                      /* time duration for each frame */
    vector<q1_mdl_simpleframe_t> frames; /* simple frame list */
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