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

struct q2_md2_header_t
{
    int magic; //should be 844121161,aka "IDP2"
    int version; //should be 8
    
    int skinWidth;
    int skinHeight;

    int frameSize;
    
    int numSkins;
    int numVertices;
    int numTexCoords;
    int numTriangles;
    int numGlCommands;
    int numFrames;

    int offsetSkins;
    int offsetTexCoords;
    int offsetTriangles;
    int offsetFrames;
    int offsetGlCommands;
    int offsetEnd;
};

struct q2_md2_triangleVertex_t
{
    byte vertex[3];
    byte lightNormalIndex;
};

struct q2_md2_frame_t
{
    float scale[3];
    float translate[3];
    char name[16];
    vector <q2_md2_triangleVertex_t> vertices;
};

struct q2_md2_triangle_t
{
    short vertexIndices[3];
    short textureIndices[3];
};

struct q2_md2_textureCoordinate_t
{
    short s, t;
};

struct q2_md2_glCommandVertex_t {
    float s, t;
    int vertexIndex;
};

struct q2_md2_glCommand_t {
    int count; // positive for strip, negative for fan
    vector<q2_md2_glCommandVertex_t> vertices;
};

struct Q2_MD2_file {
    q2_md2_header_t header;
    vector<string> skinpaths;
    vector<q2_md2_textureCoordinate_t> UV;
    vector<q2_md2_triangle_t> triangles;
    vector<q2_md2_frame_t> frames;
    vector<q2_md2_glCommand_t> GLCommands;
};

extern void JSON2Q2MD2(fs::path inpath, fs::path outpath, json jsonMD2);
extern Q2_MD2_file ParseQ2MD2(fs::path inpath);
extern void Q2MD22JSON(const Q2_MD2_file& NewMD2, fs::path outpath);