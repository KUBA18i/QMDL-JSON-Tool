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

#include "Q3MD3.h"

using namespace std;
using json = nlohmann::ordered_json;
namespace fs = filesystem;

void JSON2Q3MD3(fs::path inpath, fs::path outpath, json jsonMD3) {
    try {
        ofstream outFile(outpath, ios::binary);
        if (!outFile.is_open()) {
            cout << "Error: Could not open output file." << endl;
            return;
        }

        q3_md3_header_t new_header;
        new_header.version = 15;

        new_header.numFrames = jsonMD3["frames"].size();
        
        if (new_header.numFrames != jsonMD3["tags"].size())
            cout << "Error: Frame count mismatch with tag frames " << new_header.numFrames << " vs " << jsonMD3["tags"].size() << endl;
        
        new_header.numTags = -1;
        for (const auto& tf : jsonMD3["tags"]) {
            if (new_header.numTags == -1)
                new_header.numTags = tf.size();
            else if (new_header.numTags != tf.size()) {
                cout << "Error: Tag frame has " << tf.size() << " tags, but expected " << new_header.numTags << endl;
                return;
            }
        }

        new_header.numSurfaces = jsonMD3["surfaces"].size();
                
        auto jheader = jsonMD3.at("header");
        new_header.modelname = jheader["name"];
        new_header.numSkins = jheader["numSkins"];
        new_header.flags = jheader["flags"];
        
        outFile.write("IDP3", 4);
        outFile.write(reinterpret_cast<char*>(&new_header.version), sizeof(int));
        char modelname[64] = { 0 };
        strncpy(modelname, new_header.modelname.c_str(), 63);
        outFile.write(modelname, 64);
        outFile.write(reinterpret_cast<char*>(&new_header.flags), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numFrames), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numTags), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numSurfaces), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numSkins), sizeof(int));
        for (int i = 0; i < 16; i++) outFile.write("X", 1);
        
        new_header.offsetFrames = outFile.tellp();
        for (const auto& f : jsonMD3["frames"]) {
            float minBounds[3] = { f["minBounds"][0], f["minBounds"][1], f["minBounds"][2] };
            float maxBounds[3] = { f["maxBounds"][0], f["maxBounds"][1], f["maxBounds"][2] };
            float localOrigin[3] = { f["localOrigin"][0], f["localOrigin"][1], f["localOrigin"][2] };
            float radius = f["radius"];
            string fname = f["name"];
            char name[16] = { 0 };
            strncpy(name, fname.c_str(), 15);
            
            outFile.write(reinterpret_cast<char*>(&minBounds), sizeof(minBounds));
            outFile.write(reinterpret_cast<char*>(&maxBounds), sizeof(maxBounds));
            outFile.write(reinterpret_cast<char*>(&localOrigin), sizeof(localOrigin));
            outFile.write(reinterpret_cast<char*>(&radius), sizeof(radius));
            outFile.write(name, 16);
        }
        
        new_header.offsetTags = outFile.tellp();
        for (const auto& tf : jsonMD3["tags"]) {
            for (const auto& t : tf) {
                string fname = t["name"];
                char name[64] = { 0 };
                strncpy(name, fname.c_str(), 63);
                float origin[3] = { t["origin"][0], t["origin"][1], t["origin"][2] };
                float axis[9] = { t["axis"][0], t["axis"][1], t["axis"][2], t["axis"][3], t["axis"][4], t["axis"][5], t["axis"][6], t["axis"][7], t["axis"][8] };

                outFile.write(name, 64);
                outFile.write(reinterpret_cast<char*>(&origin), sizeof(origin));
                outFile.write(reinterpret_cast<char*>(&axis), sizeof(axis));
            }
        }
        
        new_header.offsetSurfaces = outFile.tellp();
        for (const auto& surface : jsonMD3["surfaces"]) {
            int offsetSurfaceStart = outFile.tellp();
            outFile.write("IDP3", 4);
            q3_md3_surface_t tempsurf;
            
            char CSurfaceName[64] = { 0 };
            tempsurf.surfacename = surface["name"];
            strncpy(CSurfaceName, tempsurf.surfacename.c_str(), 63);
            outFile.write(CSurfaceName, 64);
            tempsurf.flags = surface["flags"];
            tempsurf.numFrames = surface["numFrames"];
            if (tempsurf.numFrames != new_header.numFrames) {
                cout << "Error: Frame count mismatch in surface:" << tempsurf.surfacename << endl;
                return;
            }
            tempsurf.numShaders = surface["shaders"].size();
            tempsurf.numVerts = -1;
            for (const auto& vf : surface["vertexFrames"]) {
                if (tempsurf.numVerts == -1)
                    tempsurf.numVerts = vf.size();
                else if (tempsurf.numVerts != vf.size()) {
                    cout << "Error: Vertex count mismatch in surface:" << tempsurf.surfacename << endl;
                    cout << "Vertex frame has " << vf.size() << " tags, but expected " << tempsurf.numVerts << endl;
                    return;
                }
            }
            if (tempsurf.numVerts != surface["UV"].size()) {
                cout << "Error: Vertex count mismatch in surface:" << tempsurf.surfacename << endl;
                cout << "UV mismatch between frame vertices." << endl;
                return;
            }
            tempsurf.numTriangles = surface["triangles"].size();

            outFile.write(reinterpret_cast<char*>(&tempsurf.flags), sizeof(tempsurf.flags));
            outFile.write(reinterpret_cast<char*>(&tempsurf.numFrames), sizeof(tempsurf.numFrames));
            outFile.write(reinterpret_cast<char*>(&tempsurf.numShaders), sizeof(tempsurf.numShaders));
            outFile.write(reinterpret_cast<char*>(&tempsurf.numVerts), sizeof(tempsurf.numVerts));
            outFile.write(reinterpret_cast<char*>(&tempsurf.numTriangles), sizeof(tempsurf.numTriangles));
            for (int i = 0; i < 20; i++) outFile.write("X", 1);
            
            tempsurf.offsetShaders = (int)outFile.tellp() - offsetSurfaceStart;
            for (const auto& s : surface["shaders"]) {
                char cShaderName[64] = { 0 };
                string sShaderName = s[0];
                strncpy(cShaderName, sShaderName.c_str(), 63);
                outFile.write(cShaderName, 64);
                int shaderIndex = s[1];
                outFile.write(reinterpret_cast<char*>(&shaderIndex), sizeof(shaderIndex));
            }
            
            tempsurf.offsetTriangles = (int)outFile.tellp() - offsetSurfaceStart;
            for (const auto& t : surface["triangles"]) {
                int indices[3] = { t[0], t[1], t[2] };
                outFile.write(reinterpret_cast<char*>(&indices), sizeof(indices));
            }

            tempsurf.offsetUV = (int)outFile.tellp() - offsetSurfaceStart;
            for (const auto& tc : surface["UV"]) {
                float s = tc[0];
                float t = tc[1];
                outFile.write(reinterpret_cast<char*>(&s), sizeof(s));
                outFile.write(reinterpret_cast<char*>(&t), sizeof(t));
            }

            tempsurf.offsetVertices = (int)outFile.tellp() - offsetSurfaceStart;
            for (const auto& vf : surface["vertexFrames"]) {
                for (const auto& v : vf) {
                    short vert[4] = { v[0], v[1], v[2], v[3] };
                    outFile.write(reinterpret_cast<char*>(&vert), sizeof(vert));
                }
            }
            tempsurf.offsetEnd = (int)outFile.tellp() - offsetSurfaceStart;

            outFile.seekp(offsetSurfaceStart + 88);
            outFile.write(reinterpret_cast<char*>(&tempsurf.offsetTriangles), sizeof(tempsurf.offsetTriangles));
            outFile.write(reinterpret_cast<char*>(&tempsurf.offsetShaders), sizeof(tempsurf.offsetShaders));
            outFile.write(reinterpret_cast<char*>(&tempsurf.offsetUV), sizeof(tempsurf.offsetUV));
            outFile.write(reinterpret_cast<char*>(&tempsurf.offsetVertices), sizeof(tempsurf.offsetVertices));
            outFile.write(reinterpret_cast<char*>(&tempsurf.offsetEnd), sizeof(tempsurf.offsetEnd));
            outFile.seekp(offsetSurfaceStart + tempsurf.offsetEnd);
        }
        new_header.offsetEnd = outFile.tellp();

        outFile.seekp(92);
        outFile.write(reinterpret_cast<char*>(&new_header.offsetFrames), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetTags), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetSurfaces), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetEnd), sizeof(int));

        outFile.close();
        cout << "MD3 constructed successfully: " << outpath << endl;

    }
    catch (exception& e) {
        cout << "JSON Parsing Error: " << e.what() << endl;
    }
}

Q3_MD3_file ParseQ3MD3(fs::path inpath) {
    ifstream inFile(inpath, ios::binary);
    Q3_MD3_file NewMD3;

    inFile.read(reinterpret_cast<char*>(&NewMD3.header.ident), sizeof(NewMD3.header.ident));
    inFile.read(reinterpret_cast<char*>(&NewMD3.header.version), sizeof(NewMD3.header.version));
    char newmodelname[64];
    inFile.read(newmodelname, 64);
    NewMD3.header.modelname = newmodelname;
    inFile.read(reinterpret_cast<char*>(&NewMD3.header.flags), sizeof(NewMD3.header.flags));
    inFile.read(reinterpret_cast<char*>(&NewMD3.header.numFrames), sizeof(NewMD3.header.numFrames));
    inFile.read(reinterpret_cast<char*>(&NewMD3.header.numTags), sizeof(NewMD3.header.numTags));
    inFile.read(reinterpret_cast<char*>(&NewMD3.header.numSurfaces), sizeof(NewMD3.header.numSurfaces));
    inFile.read(reinterpret_cast<char*>(&NewMD3.header.numSkins), sizeof(NewMD3.header.numSkins));
    inFile.read(reinterpret_cast<char*>(&NewMD3.header.offsetFrames), sizeof(NewMD3.header.offsetFrames));
    inFile.read(reinterpret_cast<char*>(&NewMD3.header.offsetTags), sizeof(NewMD3.header.offsetTags));
    inFile.read(reinterpret_cast<char*>(&NewMD3.header.offsetSurfaces), sizeof(NewMD3.header.offsetSurfaces));
    inFile.read(reinterpret_cast<char*>(&NewMD3.header.offsetEnd), sizeof(NewMD3.header.offsetEnd));

    if (NewMD3.header.offsetFrames <= NewMD3.header.offsetTags && NewMD3.header.offsetTags <= NewMD3.header.offsetSurfaces && NewMD3.header.offsetSurfaces <= NewMD3.header.offsetEnd) {
        cout << "Offsets are in ascending order." << endl;
    }
    else {
        cout << "WARNING: Offsets are NOT in ascending order." << endl;
        cout << "header.offsetFrames: " << NewMD3.header.offsetFrames << endl;
        cout << "header.offsetTags: " << NewMD3.header.offsetTags << endl;
        cout << "header.offsetSurfaces: " << NewMD3.header.offsetSurfaces << endl;
        cout << "header.offsetEnd: " << NewMD3.header.offsetEnd << endl;
    }

    inFile.seekg(NewMD3.header.offsetFrames);
    for (int i = 0; i < NewMD3.header.numFrames; i++) {
        q3_md3_frame_t newframe;
        inFile.read(reinterpret_cast<char*>(&newframe.minBounds), sizeof(newframe.minBounds));
        inFile.read(reinterpret_cast<char*>(&newframe.maxBounds), sizeof(newframe.maxBounds));
        inFile.read(reinterpret_cast<char*>(&newframe.localOrigin), sizeof(newframe.localOrigin));
        inFile.read(reinterpret_cast<char*>(&newframe.radius), sizeof(newframe.radius));
        char name[16];
        inFile.read(name, 16);
        newframe.name = name;
        NewMD3.frames.emplace_back(newframe);
    }

    inFile.seekg(NewMD3.header.offsetTags);
    for (int i = 0; i < NewMD3.header.numFrames; i++) {
        q3_md3_tagCollection_t newtagframe;
        for (int j = 0; j < NewMD3.header.numTags; j++) {
            q3_md3_tag_t newtag;
            char name[64];
            inFile.read(name, 64);
            newtag.name = name;
            inFile.read(reinterpret_cast<char*>(&newtag.origin), sizeof(newtag.origin));
            inFile.read(reinterpret_cast<char*>(&newtag.axis), sizeof(newtag.axis));
            newtagframe.tags.emplace_back(newtag);
        }
        NewMD3.tagframes.emplace_back(newtagframe);
    }

    inFile.seekg(NewMD3.header.offsetSurfaces);
    int surface_offset = inFile.tellg();
    for (int i = 0; i < NewMD3.header.numSurfaces; i++) {
        q3_md3_surface_t newsurface;
        inFile.read(reinterpret_cast<char*>(&newsurface.ident), sizeof(newsurface.ident));
        char surfacename[64];
        inFile.read(surfacename, 64);
        newsurface.surfacename = surfacename;
        inFile.read(reinterpret_cast<char*>(&newsurface.flags), sizeof(newsurface.flags));
        inFile.read(reinterpret_cast<char*>(&newsurface.numFrames), sizeof(newsurface.numFrames));
        inFile.read(reinterpret_cast<char*>(&newsurface.numShaders), sizeof(newsurface.numShaders));
        inFile.read(reinterpret_cast<char*>(&newsurface.numVerts), sizeof(newsurface.numVerts));
        inFile.read(reinterpret_cast<char*>(&newsurface.numTriangles), sizeof(newsurface.numTriangles));
        inFile.read(reinterpret_cast<char*>(&newsurface.offsetTriangles), sizeof(newsurface.offsetTriangles));
        inFile.read(reinterpret_cast<char*>(&newsurface.offsetShaders), sizeof(newsurface.offsetShaders));
        inFile.read(reinterpret_cast<char*>(&newsurface.offsetUV), sizeof(newsurface.offsetUV));
        inFile.read(reinterpret_cast<char*>(&newsurface.offsetVertices), sizeof(newsurface.offsetVertices));
        inFile.read(reinterpret_cast<char*>(&newsurface.offsetEnd), sizeof(newsurface.offsetEnd));

        inFile.seekg(surface_offset + newsurface.offsetShaders);
        for (int j = 0; j < newsurface.numShaders; j++) {
            q3_md3_shader_t newshader;
            char shadername[64];
            inFile.read(shadername, 64);
            newshader.name = shadername;
            inFile.read(reinterpret_cast<char*>(&newshader.shaderIndex), sizeof(newshader.shaderIndex));
            newsurface.shaders.push_back(newshader);

            if (newshader.name == "")
                cout << "Empty shader detected." << endl;
            else
                cout << "Shader name: "<< newshader.name << endl;
        }

        inFile.seekg(surface_offset + newsurface.offsetTriangles);
        for (int j = 0; j < newsurface.numTriangles; j++) {
            q3_md3_triangle_t newtriangle;
            inFile.read(reinterpret_cast<char*>(&newtriangle.indices), sizeof(newtriangle.indices));
            newsurface.triangles.push_back(newtriangle);
        }

        inFile.seekg(surface_offset + newsurface.offsetUV);
        for (int j = 0; j < newsurface.numVerts; j++) {
            q3_md3_texCoord_t newTC;
            inFile.read(reinterpret_cast<char*>(&newTC.s), sizeof(newTC.s));
            inFile.read(reinterpret_cast<char*>(&newTC.t), sizeof(newTC.t));
            newsurface.UV.push_back(newTC);
        }

        inFile.seekg(surface_offset + newsurface.offsetVertices);
        for (int j = 0; j < newsurface.numFrames; j++) {
            q3_md3_vertexFrame_t newVF;
            for (int k = 0; k < newsurface.numVerts; k++) {
                q3_md3_vertex_t newvert;
                inFile.read(reinterpret_cast<char*>(&newvert.x), sizeof(newvert.x));
                inFile.read(reinterpret_cast<char*>(&newvert.y), sizeof(newvert.y));
                inFile.read(reinterpret_cast<char*>(&newvert.z), sizeof(newvert.z));
                inFile.read(reinterpret_cast<char*>(&newvert.lightNormalIndex), sizeof(newvert.lightNormalIndex));
                newVF.vertices.push_back(newvert);
            }
            newsurface.vertexFrames.push_back(newVF);
        }
        NewMD3.surfaces.emplace_back(newsurface);
        inFile.seekg(surface_offset + newsurface.offsetEnd);
        surface_offset = inFile.tellg();
    }

    if (NewMD3.header.offsetEnd == inFile.tellg()) {
        cout << "End of file matches header." << endl;
    }
    else {
        cout << "Warning: End of file mismatch with header: " << inFile.tellg() << " vs " << NewMD3.header.offsetEnd << endl;
    }

    inFile.close();

    return NewMD3;
}

void Q3MD32JSON(const Q3_MD3_file& NewMD3, fs::path outpath) {
    json jsonMD3;
    json jheader;
    jheader["ident"] = "IDP3";
    jheader["version"] = NewMD3.header.version;
    jheader["name"] = NewMD3.header.modelname;
    jheader["flags"] = NewMD3.header.flags;
    jheader["numFrames"] = NewMD3.header.numFrames;
    jheader["numTags"] = NewMD3.header.numTags;
    jheader["numSurfaces"] = NewMD3.header.numSurfaces;
    jheader["numSkins"] = NewMD3.header.numSkins;
    jheader["offsetFrames"] = NewMD3.header.offsetFrames;
    jheader["offsetTags"] = NewMD3.header.offsetTags;
    jheader["offsetSurfaces"] = NewMD3.header.offsetSurfaces;
    jheader["offsetEnd"] = NewMD3.header.offsetEnd;
    jsonMD3["header"] = jheader;

    jsonMD3["frames"] = json::array();
    for (const auto& frame : NewMD3.frames) {
        json jframe;
        jframe["minBounds"] = { frame.minBounds[0],frame.minBounds[1],frame.minBounds[2] };
        jframe["maxBounds"] = { frame.maxBounds[0], frame.maxBounds[1], frame.maxBounds[2] };
        jframe["localOrigin"] = { frame.localOrigin[0], frame.localOrigin[1], frame.localOrigin[2] };
        jframe["radius"] = frame.radius;
        jframe["name"] = frame.name;
        jsonMD3["frames"].push_back(jframe);
    }
    
    jsonMD3["tags"] = json::array();
    for (const auto& tagframe : NewMD3.tagframes) {
        json jtagframe;
        for (const auto& tag : tagframe.tags) {
            json jtag;
            jtag["name"] = tag.name;
            jtag["origin"] = { tag.origin[0],tag.origin[1],tag.origin[2] };
            jtag["axis"] = { tag.axis[0],tag.axis[1],tag.axis[2],tag.axis[3],tag.axis[4],tag.axis[5],tag.axis[6],tag.axis[7],tag.axis[8] };
            jtagframe.push_back(jtag);
        }
        jsonMD3["tags"].push_back(jtagframe);
    }
    
    jsonMD3["surfaces"] = json::array();
    for (const auto& surface : NewMD3.surfaces) {
        json jsurface;
        jsurface["version"] = "IDP3";
        jsurface["name"] = surface.surfacename;
        jsurface["flags"] = surface.flags;
        jsurface["numFrames"] = surface.numFrames;
        jsurface["numShaders"] = surface.numShaders;
        jsurface["numVerts"] = surface.numVerts;
        jsurface["numTriangles"] = surface.numTriangles;
        jsurface["offsetTriangles"] = surface.offsetTriangles;
        jsurface["offsetShaders"] = surface.offsetShaders;
        jsurface["offsetUV"] = surface.offsetUV;
        jsurface["offsetVertices"] = surface.offsetVertices;
        jsurface["offsetEnd"] = surface.offsetEnd;

        jsurface["shaders"] = json::array();
        for (int i = 0; i < surface.numShaders; i++) {
            json jshader;
            jshader = { surface.shaders[i].name, surface.shaders[i].shaderIndex };
            jsurface["shaders"].push_back(jshader);
        }

        jsurface["triangles"] = json::array();
        for (int i = 0; i < surface.numTriangles; i++) {
            json jtriangle;
            jtriangle = { surface.triangles[i].indices[0], surface.triangles[i].indices[1], surface.triangles[i].indices[2] };
            jsurface["triangles"].push_back(jtriangle);
        }

        jsurface["UV"] = json::array();
        for (int i = 0; i < surface.numVerts; i++) {
            json jtc;
            jtc = { surface.UV[i].s,surface.UV[i].t };
            jsurface["UV"].push_back(jtc);
        }

        jsurface["vertexFrames"] = json::array();
        for (int i = 0; i < surface.numFrames; i++) {
            json jvf;
            //jvf["name"] = NewMD3.frames[i].name;
            for (int j = 0; j < surface.numVerts; j++) {
                json jvert;
                jvert = { surface.vertexFrames[i].vertices[j].x,surface.vertexFrames[i].vertices[j].y, surface.vertexFrames[i].vertices[j].z, surface.vertexFrames[i].vertices[j].lightNormalIndex};
                jvf.push_back(jvert);
            }
            jsurface["vertexFrames"].push_back(jvf);
        }
        
        jsonMD3["surfaces"].push_back(jsurface);

    }

    cout << "JSON prepped, time to export." << endl;
    ofstream outFile(outpath);
    if (outFile.is_open()) {
        outFile << jsonMD3.dump(2);
        outFile.close();
        cout << "JSON created successfully: " << outpath << endl;
    }
    else {
        cout << "Error: Could not write output file." << endl;
    }
}