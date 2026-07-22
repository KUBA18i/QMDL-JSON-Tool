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

using namespace std;
using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

struct h2_fm_blockheader_t
{
	char ident[32];
	int version;
	int size;
};
//Block header values listed below:
// Initial Header
#define FM_HEADER_NAME	"header"
#define FM_HEADER_VER	2
// Skin header.
#define FM_SKIN_NAME		"skin"
#define FM_SKIN_VER			1
// ST coord header.
#define FM_ST_NAME			"st coord"
#define FM_ST_VER			1
// Tri header.
#define FM_TRI_NAME			"tris"
#define FM_TRI_VER			1
// Frame header.
#define FM_FRAME_NAME		"frames"
#define FM_FRAME_VER		1
// GLCmds header.
#define FM_GLCMDS_NAME		"glcmds"
#define FM_GLCMDS_VER		1
// Mesh nodes header.
#define FM_MESH_NAME		"mesh nodes"
#define FM_MESH_VER			3
// Skeleton header.
#define FM_SKELETON_NAME	"skeleton"
#define FM_SKELETON_VER		1
// References header.
#define FM_REFERENCES_NAME	"references"
#define FM_REFERENCES_VER	1

//FM_SHORT_FRAME, FM_NORMAL and FM_COMP blocks are unused.
/*
// Frame for compression, just the names.
#define FM_SHORT_FRAME_NAME	"short frames"
#define FM_SHORT_FRAME_VER	1
// Normals for compressed frames.
#define FM_NORMAL_NAME		"normals"
#define FM_NORMAL_VER		1
// Compressed frame data.
#define FM_COMP_NAME		"comp data"
#define FM_COMP_VER			1
*/

struct h2_fm_startheader_t
{
    int skinwidth;
    int skinheight;
    int framesize;

    int num_skins;
    int num_xyz;
    int num_st;
    int num_tris;
    int num_glcmds;
    int num_frames;
    int num_mesh_nodes;
};

struct h2_fm_Placement_t
{
    float origin[3];
    float direction[3];
    float up[3];
};

struct h2_fm_stvert_t {
    short s, t;
};

struct h2_fm_triangle_t {
    short vertexIndices[3];
    short textureIndices[3];
};

struct h2_fm_triangleVertex_t {
    uint8_t vertex[3];
    uint8_t lightNormalIndex;
};

struct h2_fm_meshnode_t {
    uint8_t unused[256];
    uint8_t verts[256];
    short start_glcmds;
    short num_glcmds;
};

struct h2_fm_frame_t {
    float scale[3];
    float translate[3];
    char name[16];
    vector<h2_fm_triangleVertex_t> vertices;
};

struct h2_fm_ReferenceBlock_t {
    bool isUsed = false;
    int referenceType = 0;
    int haveRefs = 0;
    vector<h2_fm_Placement_t> placements;
};

struct h2_fm_SkeletalCluster_t {
    vector<int> vertices;
};

struct h2_fm_SkeletonFrame_t {
    vector<h2_fm_Placement_t> joints;
};

struct h2_fm_SkeletonBlock_t {
    bool isUsed = false;
    int skeletalType = 0;
    int numClusters = 0;
    vector<h2_fm_SkeletalCluster_t> clusters;
    int haveSkeleton = 0;
    vector<h2_fm_SkeletonFrame_t> frames;
};

struct h2_fm_glCommandVertex_t {
    float s, t;
    int vertexIndex;
};

struct h2_fm_glCommand_t {
    int count; // positive for strip, negative for fan
    vector<h2_fm_glCommandVertex_t> vertices;
};

struct h2_fm_model_t {
    h2_fm_startheader_t header;
    vector<h2_fm_stvert_t> uv;
    vector<h2_fm_triangle_t> triangles;
    vector<h2_fm_frame_t> frames;
    vector<h2_fm_glCommand_t> glCommands;
    vector<string> skins;
    vector<h2_fm_meshnode_t> meshNodes;
    h2_fm_SkeletonBlock_t skeleton;
    h2_fm_ReferenceBlock_t references;
};

h2_fm_model_t ParseFM(fs::path filePath);
void FM2JSON(h2_fm_model_t model, fs::path outPath);
void JSON2H2FM(fs::path inpath, fs::path outpath, json jsonFM);