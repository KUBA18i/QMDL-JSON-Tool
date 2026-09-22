#include "SS1MDL.h"

bool SS1_MDL_bHasSavedCenter = false;
bool SS1_MDL_bHasMultipleCollisionBoxes = false;
bool SS1_MDL_bHasAttachedPositions = false;
bool SS1_MDL_bHasPolygonalPatches = false;
bool SS1_MDL_bHasPolygonsPerSurface = false;
bool SS1_MDL_bHasSavedFlagsOnStart = false;
bool SS1_MDL_bHasColorForReflectionAndSpecularity = false;
bool SS1_MDL_bHasDiffuseColor = false;
bool SS1_MDL_bHasCOLIChunk = false;//not determined from model version
void SS1_MDL_SetModelVersionFlags(char version[4]) {
    // if this is version without stretch center then it doesn't contain multiple collision boxes also
    if (memcmp(version, "V002", 4) == 0)
    {
        //it's free real estate~
    }
    // if model has stretch center but does not have multiple collision boxes
    else if (memcmp(version, "V003", 4) == 0)
    {
        SS1_MDL_bHasSavedCenter = true;
    }
    else if (memcmp(version, "V004", 4) == 0)
    {
        SS1_MDL_bHasSavedCenter = true;
        SS1_MDL_bHasMultipleCollisionBoxes = true;
    }
    else if (memcmp(version, "V005", 4) == 0)
    {
        SS1_MDL_bHasSavedCenter = true;
        SS1_MDL_bHasMultipleCollisionBoxes = true;
        SS1_MDL_bHasAttachedPositions = true;
    }
    else if (memcmp(version, "V006", 4) == 0)
    {
        SS1_MDL_bHasSavedCenter = true;
        SS1_MDL_bHasMultipleCollisionBoxes = true;
        SS1_MDL_bHasAttachedPositions = true;
        SS1_MDL_bHasPolygonalPatches = true;
    }
    else if (memcmp(version, "V007", 4) == 0)
    {
        SS1_MDL_bHasSavedCenter = true;
        SS1_MDL_bHasMultipleCollisionBoxes = true;
        SS1_MDL_bHasAttachedPositions = true;
        SS1_MDL_bHasPolygonalPatches = true;
        SS1_MDL_bHasPolygonsPerSurface = true;
    }
    // if has saved flags on start - because 16-bit compression
    else if (memcmp(version, "V008", 4) == 0)
    {
        SS1_MDL_bHasSavedCenter = true;
        SS1_MDL_bHasMultipleCollisionBoxes = true;
        SS1_MDL_bHasAttachedPositions = true;
        SS1_MDL_bHasPolygonalPatches = true;
        SS1_MDL_bHasPolygonsPerSurface = true;
        SS1_MDL_bHasSavedFlagsOnStart = true;
    }
    // has saved color for reflection and specularity
    else if (memcmp(version, "V009", 4) == 0)
    {
        SS1_MDL_bHasSavedCenter = true;
        SS1_MDL_bHasMultipleCollisionBoxes = true;
        SS1_MDL_bHasAttachedPositions = true;
        SS1_MDL_bHasPolygonalPatches = true;
        SS1_MDL_bHasPolygonsPerSurface = true;
        SS1_MDL_bHasSavedFlagsOnStart = true;
        SS1_MDL_bHasColorForReflectionAndSpecularity = true;
    }
    // has saved diffuse color
    else if (memcmp(version, "V010", 4) == 0)
    {
        SS1_MDL_bHasSavedCenter = true;
        SS1_MDL_bHasMultipleCollisionBoxes = true;
        SS1_MDL_bHasAttachedPositions = true;
        SS1_MDL_bHasPolygonalPatches = true;
        SS1_MDL_bHasPolygonsPerSurface = true;
        SS1_MDL_bHasSavedFlagsOnStart = true;
        SS1_MDL_bHasColorForReflectionAndSpecularity = true;
        SS1_MDL_bHasDiffuseColor = true;
    }
    else
    {
        cout << "Invalid model version: " << string(version, 4) << endl;
        exit(1);
    }
    cout << "Model version: " << string(version, 4) << endl;
}

char ChunkBuffer[4];
uint32_t ChunkSize;
void WriteChunkHeader(ofstream& outFile, const char* id, uint32_t size) {
    outFile.write(id, 4);
    outFile.write(reinterpret_cast<char*>(&size), sizeof(size));
    cout << "Writing Chunk ID: " << string(id, 4) << " | size: " << size << endl;
}
void ReadChunkHeader(ifstream& inFile) {
    inFile.read(ChunkBuffer, 4);
    inFile.read(reinterpret_cast<char*>(&ChunkSize), sizeof(ChunkSize));
    cout << "Parsing Chunk ID: " << string(ChunkBuffer, 4) << " | size: " << ChunkSize << endl;
}
void ReadChunkSubHeader(ifstream& inFile) {
    inFile.read(ChunkBuffer, 4);
    cout << "Parsing Sub-Chunk ID: " << string(ChunkBuffer, 4) << endl;
}
string ReadCString(ifstream& inFile) {
    uint32_t CharCount;
    string newstring;
    inFile.read(reinterpret_cast<char*>(&CharCount), sizeof(CharCount));
    for (int i = 0; i < CharCount; i++) {
        char charbuffer;
        inFile.read(&charbuffer, 1);
        newstring.push_back(charbuffer);
    }
    return newstring;
}
bool Read4ByteBool(ifstream& inFile) {
    uint32_t buffer;
    inFile.read(reinterpret_cast<char*>(&buffer), sizeof(buffer));
    bool result = (buffer != 0);
    return result;
}
void WriteCString(ofstream& outFile, string cstring) {
    uint32_t CharCount = cstring.size();
    outFile.write(reinterpret_cast<char*>(&CharCount), sizeof(CharCount));
    outFile.write(cstring.c_str(), CharCount);
}
void Write4ByteBool(ofstream& outFile, bool input) {
    uint32_t buffer = input ? 1 : 0;
    outFile.write(reinterpret_cast<char*>(&buffer), sizeof(buffer));
}
uint32_t NullsFound = 0;
float JSONFloatCheck(const nlohmann::json& j) {
    NullsFound++;
    return (j.is_number()) ? j.get<float>() : numeric_limits<float>::quiet_NaN();
}

SS1_MDL_file JSON2SS1MDL(const fs::path& inpath, json& jsonMDL) {
    try {
        SS1_MDL_file newMDL;
        auto jmodelinfo = jsonMDL.at("ModelInfo");
        newMDL.version = jmodelinfo["version"];
        char MDLVersion[4];
        newMDL.version.copy(MDLVersion, 4);
        SS1_MDL_SetModelVersionFlags(MDLVersion);

        newMDL.flags = jmodelinfo["flags"];
        newMDL.verticesCount = jsonMDL["mainMipVertices"].size();
        newMDL.framesCount = jsonMDL["frameInfos"].size();

        if (jsonMDL["AV16"].is_array() && !jsonMDL["AV16"].empty()) {
            for (const auto& vert : jsonMDL["AV16"]) {
                ss1_mdl_Vertex16_old newVert;
                newVert.x = vert[0];
                newVert.y = vert[1];
                newVert.z = vert[2];
                newVert.normalIndex = vert[3];
                newMDL.frameVertices16_old.push_back(newVert);
            }
        }
        else if (jsonMDL["AV17"].is_array() && !jsonMDL["AV17"].empty()) {
            for (const auto& vert : jsonMDL["AV17"]) {
                ss1_mdl_Vertex16 newVert;
                newVert.x = vert[0];
                newVert.y = vert[1];
                newVert.z = vert[2];
                newVert.normH = vert[3];
                newVert.normP = vert[4];
                newMDL.frameVertices16.push_back(newVert);
            }
        }
        else if (jsonMDL["AFVX"].is_array() && !jsonMDL["AFVX"].empty()) {
            for (const auto& vert : jsonMDL["AFVX"]) {
                ss1_mdl_Vertex8 newVert;
                newVert.x = vert[0];
                newVert.y = vert[1];
                newVert.z = vert[2];
                newVert.normalIndex = vert[3];
                newMDL.frameVertices8.push_back(newVert);
            }
        }
        else {
            cout << "Error: missing vertices chunk." << endl;
            exit(1);
        }
        
        for (const auto& fi : jsonMDL["frameInfos"]) {
            ss1_mdl_ModelFrameInfo newfi;
            newfi.MinX = JSONFloatCheck(fi[0]);
            newfi.MinY = JSONFloatCheck(fi[1]);
            newfi.MinZ = JSONFloatCheck(fi[2]);
            newfi.MaxX = JSONFloatCheck(fi[3]);
            newfi.MaxY = JSONFloatCheck(fi[4]);
            newfi.MaxZ = JSONFloatCheck(fi[5]);
            newMDL.frameInfos.push_back(newfi);
        }

        for (const auto& mv : jsonMDL["mainMipVertices"]) {
            ss1_mdl_MipVertex newmv;
            newmv.x = JSONFloatCheck(mv[0]);
            newmv.y = JSONFloatCheck(mv[1]);
            newmv.z = JSONFloatCheck(mv[2]);
            newMDL.mainMipVertices.push_back(newmv);
        }

        for (const auto& mm : jsonMDL["vertexMipMask"])
            newMDL.vertexMipMask.push_back(mm);
        newMDL.mipCount = jsonMDL["mips"].size();
        for (int i = 0; i < 32; i++)
            newMDL.mipSwitchFactors[i] = JSONFloatCheck(jsonMDL["mipSwitchFactors"][i]);
        for (const auto& mip : jsonMDL["mips"]) {
            ss1_mdl_Mip newMIP;
            for (const auto& poly : mip["polygons"]) {
                ss1_mdl_Polygon newPoly;
                newPoly.newformat = poly["newformat"];
                for (const auto& vert : poly["vertices"]) {
                    ss1_mdl_PolygonVertex newvert;
                    newvert.transformedVertexIndex = vert[0];
                    newvert.textureVertexIndex = vert[1];
                    newPoly.vertices.push_back(newvert);
                }
                newPoly.renderFlags = poly["renderFlags"];
                newPoly.colorAndAlpha = poly["colorAndAlpha"];
                newPoly.surfaceIndex = poly["surfaceIndex"];
                if (!newPoly.newformat) {
                    newPoly.exONcolor = poly["exONcolor"];
                    newPoly.exOFFcolor = poly["exOFFcolor"];
                }
                newMIP.polygons.push_back(newPoly);
            }
            if (SS1_MDL_bHasPolygonsPerSurface) {
                bool newformat = mip["textureVertices"][0]["newformat"];
                if (!newformat) {
                    for (const auto& tv : mip["textureVertices"]) {
                        ss1_mdl_TextureVertex newTV;
                        newTV.uvwX = JSONFloatCheck(tv["UVW"][0]);
                        newTV.uvwY = JSONFloatCheck(tv["UVW"][1]);
                        newTV.uvwZ = JSONFloatCheck(tv["UVW"][2]);
                        newTV.u = JSONFloatCheck(tv["UV"][0]);
                        newTV.v = JSONFloatCheck(tv["UV"][1]);
                        newTV.done = tv["done"];
                        newTV.transformedVertexIndex = tv["transformedVertexIndex"];
                        newMIP.textureVertices.push_back(newTV);
                    }
                }
                else {
                    for (const auto& tv : mip["textureVertices"]) {
                        ss1_mdl_TextureVertex newTV;
                        newTV.newformat = true;
                        newTV.uvwX = JSONFloatCheck(tv["UVW"][0]);
                        newTV.uvwY = JSONFloatCheck(tv["UVW"][1]);
                        newTV.uvwZ = JSONFloatCheck(tv["UVW"][2]);
                        newTV.u = JSONFloatCheck(tv["UV"][0]);
                        newTV.v = JSONFloatCheck(tv["UV"][1]);
                        newTV.SurfaceIndex = tv["SurfaceIndex"];
                        newTV.transformedVertexIndex = tv["transformedVertexIndex"];
                        newTV.bumpU[0] = JSONFloatCheck(tv["bumpU"][0]);
                        newTV.bumpU[1] = JSONFloatCheck(tv["bumpU"][1]);
                        newTV.bumpU[2] = JSONFloatCheck(tv["bumpU"][2]);
                        newTV.bumpV[0] = JSONFloatCheck(tv["bumpV"][0]);
                        newTV.bumpV[1] = JSONFloatCheck(tv["bumpV"][1]);
                        newTV.bumpV[2] = JSONFloatCheck(tv["bumpV"][2]);
                        newMIP.textureVertices.push_back(newTV);
                    }
                }
            }
            else {
                for (const auto& tv : mip["textureVertices"]) {
                    ss1_mdl_TextureVertex newTV;
                    newTV.uvwX = JSONFloatCheck(tv["UVW"][0]);
                    newTV.uvwY = JSONFloatCheck(tv["UVW"][1]);
                    newTV.uvwZ = JSONFloatCheck(tv["UVW"][2]);
                    newTV.u = JSONFloatCheck(tv["UV"][0]);
                    newTV.v = JSONFloatCheck(tv["UV"][1]);
                    newTV.done = tv["done"];
                    newMIP.textureVertices.push_back(newTV);
                }
            }
            for (const auto& ms : mip["mappingSurfaces"]) {
                ss1_mdl_MappingSurface newMS;
                newMS.name = ms["name"];
                newMS.surfaceOffsetX = JSONFloatCheck(ms["surfaceOffset"][0]);
                newMS.surfaceOffsetY = JSONFloatCheck(ms["surfaceOffset"][1]);
                newMS.surfaceOffsetZ = JSONFloatCheck(ms["surfaceOffset"][2]);
                newMS.h = JSONFloatCheck(ms["hpb"][0]);
                newMS.p = JSONFloatCheck(ms["hpb"][1]);
                newMS.b = JSONFloatCheck(ms["hpb"][2]);
                newMS.zoom = JSONFloatCheck(ms["zoom"]);
                if (SS1_MDL_bHasPolygonsPerSurface) {
                    newMS.shadingType = ms["shadingType"];
                    newMS.translucencyType = ms["translucencyType"];
                    newMS.renderingFlags = ms["renderingFlags"];
                    for (const auto& pi : ms["polygonIndices"])
                        newMS.polygonIndices.push_back(pi);
                    for (const auto& vi : ms["textureVertexIndices"])
                        newMS.textureVertexIndices.push_back(vi);
                    newMS.color = ms["color"];
                }
                if (SS1_MDL_bHasDiffuseColor) {
                    newMS.diffuseColor = ms["diffuseColor"];
                    newMS.reflectionColor = ms["reflectionColor"];
                    newMS.specularColor = ms["specularColor"];
                    newMS.bumpColor = ms["bumpColor"];
                    newMS.onColorMask = ms["onColorMask"];
                    newMS.offColorMask = ms["offColorMask"];
                }
                newMIP.mappingSurfaces.push_back(newMS);
            }
            if (SS1_MDL_bHasPolygonalPatches) {
                newMIP.flags = mip["flags"];
                for (const auto& patch : mip["patches"]) {
                    ss1_mdl_PolygonsPerPatch newPatch;
                    newPatch.ctOccupied = patch["ctOccupied"];
                    if (newPatch.ctOccupied != 0)
                        for (const auto& pi : patch["PolygonIndices"])
                            newPatch.PolygonIndices.push_back(pi);
                    newMIP.patches.push_back(newPatch);
                }
            }
            newMDL.mips.push_back(newMIP);
        }

        newMDL.NewPatchFormat = jsonMDL["Patches"]["NewFormat"];
        if (!newMDL.NewPatchFormat)
            for (int i = 0; i < 32; i++)
                newMDL.patches[i].textureName = jsonMDL["Patches"]["Entries"][i]["textureName"];
        else {
            for (int i = 0; i < 32; i++) {
                newMDL.patches[i].name = jsonMDL["Patches"]["Entries"][i]["name"];
                newMDL.patches[i].textureName = jsonMDL["Patches"]["Entries"][i]["textureName"];
                newMDL.patches[i].posU = jsonMDL["Patches"]["Entries"][i]["position"][0];
                newMDL.patches[i].posV = jsonMDL["Patches"]["Entries"][i]["position"][1];
                newMDL.patches[i].stretch = JSONFloatCheck(jsonMDL["Patches"]["Entries"][i]["stretch"]);
            }
        }
        newMDL.texWidth = jmodelinfo["texWidth"];
        newMDL.texHeight = jmodelinfo["texHeight"];
        if (!newMDL.NewPatchFormat) {
            for (int i = 0; i < 32; i++)
            {
                newMDL.patches[i].posU = jsonMDL["Patches"]["Entries"][i]["position"][0];
                newMDL.patches[i].posV = jsonMDL["Patches"]["Entries"][i]["position"][1];
            }
        }
        newMDL.shadowQuality = jmodelinfo["shadowQuality"];
        newMDL.stretch[0] = JSONFloatCheck(jmodelinfo["stretch"][0]);
        newMDL.stretch[1] = JSONFloatCheck(jmodelinfo["stretch"][1]);
        newMDL.stretch[2] = JSONFloatCheck(jmodelinfo["stretch"][2]);
        if (SS1_MDL_bHasSavedCenter) {
            newMDL.center[0] = JSONFloatCheck(jmodelinfo["center"][0]);
            newMDL.center[1] = JSONFloatCheck(jmodelinfo["center"][1]);
            newMDL.center[2] = JSONFloatCheck(jmodelinfo["center"][2]);
        }
        if (SS1_MDL_bHasMultipleCollisionBoxes) {
            for (const auto& cb : jsonMDL["CollisionBoxes"]["Entries"]) {
                ss1_mdl_CollisionBox newCB;
                newCB.minX = JSONFloatCheck(cb["min"][0]);
                newCB.minY = JSONFloatCheck(cb["min"][1]);
                newCB.minZ = JSONFloatCheck(cb["min"][2]);
                newCB.maxX = JSONFloatCheck(cb["max"][0]);
                newCB.maxY = JSONFloatCheck(cb["max"][1]);
                newCB.maxZ = JSONFloatCheck(cb["max"][2]);
                newCB.name = cb["name"];
                newMDL.collisionBoxes.push_back(newCB);
            }
        }
        else {
            ss1_mdl_CollisionBox newCB;
            newCB.minX = JSONFloatCheck(jsonMDL["CollisionBoxes"]["Entries"][0]["min"][0]);
            newCB.minY = JSONFloatCheck(jsonMDL["CollisionBoxes"]["Entries"][0]["min"][1]);
            newCB.minZ = JSONFloatCheck(jsonMDL["CollisionBoxes"]["Entries"][0]["min"][2]);
            newCB.maxX = JSONFloatCheck(jsonMDL["CollisionBoxes"]["Entries"][0]["max"][0]);
            newCB.maxY = JSONFloatCheck(jsonMDL["CollisionBoxes"]["Entries"][0]["max"][1]);
            newCB.maxZ = JSONFloatCheck(jsonMDL["CollisionBoxes"]["Entries"][0]["max"][2]);
            newMDL.collisionBoxes.push_back(newCB);
        }

        if (jsonMDL["CollisionBoxes"]["collideAsCube"].is_null())
            newMDL.collideAsCube = false;
        else
        {
            newMDL.collideAsCube = jsonMDL["CollisionBoxes"]["collideAsCube"];
            SS1_MDL_bHasCOLIChunk = true;
        }

        if (SS1_MDL_bHasAttachedPositions) {
            for (const auto& ap : jsonMDL["attachedPositions"]) {
                ss1_mdl_AttachedPosition newap;
                newap.centerVertex = ap["centerVertex"];
                newap.frontVertex= ap["frontVertex"];
                newap.upVertex = ap["upVertex"];
                newap.Position[0] = ap["Position"][0];
                newap.Position[1] = ap["Position"][1];
                newap.Position[2] = ap["Position"][2];
                newap.Angle[0] = ap["Angle"][0];
                newap.Angle[1] = ap["Angle"][1];
                newap.Angle[2] = ap["Angle"][2];
                newMDL.attachedPositions.push_back(newap);
            }
        }
        
        for (int i = 0; i < 32; i++)
            newMDL.colorNames[i] = jsonMDL["colorNames"][i];

        for (const auto& anm : jsonMDL["animations"]) {
            ss1_mdl_AnimData newAnm;
            string sName = anm["name"];
            memset(newAnm.name, 0, sizeof(newAnm.name));
            strncpy(newAnm.name, sName.c_str(), sizeof(newAnm.name) - 1);
            newAnm.SecondsPerFrame = JSONFloatCheck(anm["SecondsPerFrame"]);
            for (const auto& fi : anm["FrameIndices"])
                newAnm.FrameIndices.push_back(fi);
            newMDL.animations.push_back(newAnm);
        }
        if (SS1_MDL_bHasDiffuseColor)
            newMDL.colorDiffuse = jsonMDL["Colors"]["Diffuse"];
        if (SS1_MDL_bHasColorForReflectionAndSpecularity) {
            newMDL.colorReflections = jsonMDL["Colors"]["Reflections"];
            newMDL.colorSpecular = jsonMDL["Colors"]["Specular"];
            newMDL.colorBump = jsonMDL["Colors"]["Bump"];
        }

        if (NullsFound)
            cout << "Null values to NaN floats converted: " << NullsFound << endl;
        cout << "JSON parsed, creating MDL..." << endl;
        return newMDL;
    }
    catch (exception& e) {
        cout << "JSON Parsing Error: " << e.what() << endl;
        exit(1);
    }
}

void WriteSS1MDL(const fs::path& outpath, SS1_MDL_file newMDL) {
    try {
        ofstream outFile(outpath, ios::binary);
        if (!outFile.is_open()) {
            cout << "Error: Could not open output file." << endl;
            return;
        }

        uint32_t MemberCount;

        outFile.write("MDAT", 4);
        outFile.write(newMDL.version.c_str(), 4);
        if (SS1_MDL_bHasSavedFlagsOnStart)
            outFile.write(reinterpret_cast<char*>(&newMDL.flags), sizeof(newMDL.flags));
        WriteChunkHeader(outFile, "IVTX", 4);
        outFile.write(reinterpret_cast<char*>(&newMDL.verticesCount), sizeof(newMDL.verticesCount));
        WriteChunkHeader(outFile, "IFRM", 4);
        outFile.write(reinterpret_cast<char*>(&newMDL.framesCount), sizeof(newMDL.framesCount));

        if (!newMDL.frameVertices16_old.empty()) {
            WriteChunkHeader(outFile, "AV16", newMDL.verticesCount * newMDL.framesCount * 8);
            for (auto& vert : newMDL.frameVertices16_old) {
                outFile.write(reinterpret_cast<char*>(&vert.x), sizeof(vert.x));
                outFile.write(reinterpret_cast<char*>(&vert.y), sizeof(vert.y));
                outFile.write(reinterpret_cast<char*>(&vert.z), sizeof(vert.z));
                outFile.write(reinterpret_cast<char*>(&vert.normalIndex), sizeof(vert.normalIndex));
                outFile.write("\0", 1);//padding
            }
        }
        else if (!newMDL.frameVertices16.empty()) {
            WriteChunkHeader(outFile, "AV17", newMDL.verticesCount * newMDL.framesCount * 8);
            for (auto& vert : newMDL.frameVertices16) {
                outFile.write(reinterpret_cast<char*>(&vert.x), sizeof(vert.x));
                outFile.write(reinterpret_cast<char*>(&vert.y), sizeof(vert.y));
                outFile.write(reinterpret_cast<char*>(&vert.z), sizeof(vert.z));
                outFile.write(reinterpret_cast<char*>(&vert.normH), sizeof(vert.normH));
                outFile.write(reinterpret_cast<char*>(&vert.normP), sizeof(vert.normP));
            }
        }
        else if (!newMDL.frameVertices8.empty()) {
            WriteChunkHeader(outFile, "AFVX", newMDL.verticesCount * newMDL.framesCount * 4);
            for (auto& vert : newMDL.frameVertices8) {
                outFile.write(reinterpret_cast<char*>(&vert.x), sizeof(vert.x));
                outFile.write(reinterpret_cast<char*>(&vert.y), sizeof(vert.y));
                outFile.write(reinterpret_cast<char*>(&vert.z), sizeof(vert.z));
                outFile.write(reinterpret_cast<char*>(&vert.normalIndex), sizeof(vert.normalIndex));
            }
        }
        else {
            cout << "Error: missing vertices." << endl;
            exit(1);
        }

        WriteChunkHeader(outFile, "AFIN", newMDL.framesCount * 24);
        for (auto& newfi : newMDL.frameInfos) {
            outFile.write(reinterpret_cast<char*>(&newfi.MinX), sizeof(newfi.MinX));
            outFile.write(reinterpret_cast<char*>(&newfi.MinY), sizeof(newfi.MinX));
            outFile.write(reinterpret_cast<char*>(&newfi.MinZ), sizeof(newfi.MinX));
            outFile.write(reinterpret_cast<char*>(&newfi.MaxX), sizeof(newfi.MaxX));
            outFile.write(reinterpret_cast<char*>(&newfi.MaxY), sizeof(newfi.MaxY));
            outFile.write(reinterpret_cast<char*>(&newfi.MaxZ), sizeof(newfi.MaxZ));
        }
        WriteChunkHeader(outFile, "AMMV", newMDL.verticesCount * 12);
        for (auto& newmv : newMDL.mainMipVertices) {
            outFile.write(reinterpret_cast<char*>(&newmv.x), sizeof(newmv.x));
            outFile.write(reinterpret_cast<char*>(&newmv.y), sizeof(newmv.y));
            outFile.write(reinterpret_cast<char*>(&newmv.z), sizeof(newmv.z));
        }
        WriteChunkHeader(outFile, "AVMK", newMDL.verticesCount * 4);
        for (auto& newmm : newMDL.vertexMipMask)
            outFile.write(reinterpret_cast<char*>(&newmm), sizeof(newmm));
        WriteChunkHeader(outFile, "IMIP", 4);
        MemberCount = newMDL.mips.size();
        outFile.write(reinterpret_cast<char*>(&MemberCount), sizeof(MemberCount));
        WriteChunkHeader(outFile, "FMIP", 128);
        for (auto& newvmm : newMDL.mipSwitchFactors)
            outFile.write(reinterpret_cast<char*>(&newvmm), sizeof(newvmm));
        for (auto& mip : newMDL.mips) {
            WriteChunkHeader(outFile, "IPOL", 4);
            MemberCount = mip.polygons.size();
            outFile.write(reinterpret_cast<char*>(&MemberCount), sizeof(MemberCount));
            for (auto& poly : mip.polygons) {
                if (!poly.newformat) {
                    outFile.write("MDPLIMPV", 8);
                    ChunkSize = 4;
                    outFile.write(reinterpret_cast<char*>(&ChunkSize), sizeof(ChunkSize));
                    MemberCount = poly.vertices.size();
                    outFile.write(reinterpret_cast<char*>(&MemberCount), sizeof(MemberCount));
                    for (auto& vert : poly.vertices) {
                        outFile.write(reinterpret_cast<char*>(&vert.transformedVertexIndex), sizeof(vert.transformedVertexIndex));
                        outFile.write(reinterpret_cast<char*>(&vert.textureVertexIndex), sizeof(vert.textureVertexIndex));
                    }
                    outFile.write(reinterpret_cast<char*>(&poly.renderFlags), sizeof(poly.renderFlags));
                    outFile.write(reinterpret_cast<char*>(&poly.colorAndAlpha), sizeof(poly.colorAndAlpha));
                    outFile.write(reinterpret_cast<char*>(&poly.surfaceIndex), sizeof(poly.surfaceIndex));
                    outFile.write(reinterpret_cast<char*>(&poly.exONcolor), sizeof(poly.exONcolor));
                    outFile.write(reinterpret_cast<char*>(&poly.exOFFcolor), sizeof(poly.exOFFcolor));
                }
                else {
                    outFile.write("MDP2", 4);
                    MemberCount = poly.vertices.size();
                    outFile.write(reinterpret_cast<char*>(&MemberCount), sizeof(MemberCount));
                    for (auto& vert : poly.vertices) {
                        outFile.write(reinterpret_cast<char*>(&vert.transformedVertexIndex), sizeof(vert.transformedVertexIndex));
                        outFile.write(reinterpret_cast<char*>(&vert.textureVertexIndex), sizeof(vert.textureVertexIndex));
                    }
                    outFile.write(reinterpret_cast<char*>(&poly.renderFlags), sizeof(poly.renderFlags));
                    outFile.write(reinterpret_cast<char*>(&poly.colorAndAlpha), sizeof(poly.colorAndAlpha));
                    outFile.write(reinterpret_cast<char*>(&poly.surfaceIndex), sizeof(poly.surfaceIndex));
                }
            }
            MemberCount = mip.textureVertices.size();
            outFile.write(reinterpret_cast<char*>(&MemberCount), sizeof(MemberCount));
            if (SS1_MDL_bHasPolygonsPerSurface) {
                if (!mip.textureVertices[0].newformat) {
                    WriteChunkHeader(outFile, "TXVT", 28 * MemberCount);
                    for (auto& newTV : mip.textureVertices) {
                        outFile.write(reinterpret_cast<char*>(&newTV.uvwX), sizeof(newTV.uvwX));
                        outFile.write(reinterpret_cast<char*>(&newTV.uvwY), sizeof(newTV.uvwY));
                        outFile.write(reinterpret_cast<char*>(&newTV.uvwZ), sizeof(newTV.uvwZ));
                        outFile.write(reinterpret_cast<char*>(&newTV.u), sizeof(newTV.u));
                        outFile.write(reinterpret_cast<char*>(&newTV.v), sizeof(newTV.v));
                        Write4ByteBool(outFile, newTV.done);
                        outFile.write(reinterpret_cast<char*>(&newTV.transformedVertexIndex), sizeof(newTV.transformedVertexIndex));
                    }
                }
                else {
                    WriteChunkHeader(outFile, "TXV2", 52 * MemberCount);
                    for (auto& newTV : mip.textureVertices) {
                        outFile.write(reinterpret_cast<char*>(&newTV.uvwX), sizeof(newTV.uvwX));
                        outFile.write(reinterpret_cast<char*>(&newTV.uvwY), sizeof(newTV.uvwY));
                        outFile.write(reinterpret_cast<char*>(&newTV.uvwZ), sizeof(newTV.uvwZ));
                        outFile.write(reinterpret_cast<char*>(&newTV.u), sizeof(newTV.u));
                        outFile.write(reinterpret_cast<char*>(&newTV.v), sizeof(newTV.v));
                        outFile.write(reinterpret_cast<char*>(&newTV.SurfaceIndex), sizeof(newTV.SurfaceIndex));
                        outFile.write(reinterpret_cast<char*>(&newTV.transformedVertexIndex), sizeof(newTV.transformedVertexIndex));
                        outFile.write(reinterpret_cast<char*>(&newTV.bumpU[0]), sizeof(newTV.bumpU[0]));
                        outFile.write(reinterpret_cast<char*>(&newTV.bumpU[1]), sizeof(newTV.bumpU[1]));
                        outFile.write(reinterpret_cast<char*>(&newTV.bumpU[2]), sizeof(newTV.bumpU[2]));
                        outFile.write(reinterpret_cast<char*>(&newTV.bumpV[0]), sizeof(newTV.bumpV[0]));
                        outFile.write(reinterpret_cast<char*>(&newTV.bumpV[1]), sizeof(newTV.bumpV[1]));
                        outFile.write(reinterpret_cast<char*>(&newTV.bumpV[2]), sizeof(newTV.bumpV[2]));
                    }
                }
            }
            else {
                WriteChunkHeader(outFile, "TXVT", 24 * MemberCount);
                for (auto& newTV : mip.textureVertices) {
                    outFile.write(reinterpret_cast<char*>(&newTV.uvwX), sizeof(newTV.uvwX));
                    outFile.write(reinterpret_cast<char*>(&newTV.uvwY), sizeof(newTV.uvwY));
                    outFile.write(reinterpret_cast<char*>(&newTV.uvwZ), sizeof(newTV.uvwZ));
                    outFile.write(reinterpret_cast<char*>(&newTV.u), sizeof(newTV.u));
                    outFile.write(reinterpret_cast<char*>(&newTV.v), sizeof(newTV.v));
                    Write4ByteBool(outFile, newTV.done);
                }
            }
            MemberCount = mip.mappingSurfaces.size();
            outFile.write(reinterpret_cast<char*>(&MemberCount), sizeof(MemberCount));
            for (auto& newMS : mip.mappingSurfaces) {
                WriteCString(outFile, newMS.name);
                outFile.write(reinterpret_cast<char*>(&newMS.surfaceOffsetX), sizeof(newMS.surfaceOffsetX));
                outFile.write(reinterpret_cast<char*>(&newMS.surfaceOffsetY), sizeof(newMS.surfaceOffsetY));
                outFile.write(reinterpret_cast<char*>(&newMS.surfaceOffsetZ), sizeof(newMS.surfaceOffsetZ));
                outFile.write(reinterpret_cast<char*>(&newMS.h), sizeof(newMS.h));
                outFile.write(reinterpret_cast<char*>(&newMS.p), sizeof(newMS.p));
                outFile.write(reinterpret_cast<char*>(&newMS.b), sizeof(newMS.b));
                outFile.write(reinterpret_cast<char*>(&newMS.zoom), sizeof(newMS.zoom));
                if (SS1_MDL_bHasPolygonsPerSurface) {
                    outFile.write(reinterpret_cast<char*>(&newMS.shadingType), sizeof(newMS.shadingType));
                    outFile.write(reinterpret_cast<char*>(&newMS.translucencyType), sizeof(newMS.translucencyType));
                    outFile.write(reinterpret_cast<char*>(&newMS.renderingFlags), sizeof(newMS.renderingFlags));
                    uint32_t ItemCount = newMS.polygonIndices.size();
                    outFile.write(reinterpret_cast<char*>(&ItemCount), sizeof(ItemCount));
                    for (auto& pi : newMS.polygonIndices)
                        outFile.write(reinterpret_cast<char*>(&pi), sizeof(pi));
                    ItemCount = newMS.textureVertexIndices.size();
                    outFile.write(reinterpret_cast<char*>(&ItemCount), sizeof(ItemCount));
                    for (auto& vi : newMS.textureVertexIndices)
                        outFile.write(reinterpret_cast<char*>(&vi), sizeof(vi));
                    outFile.write(reinterpret_cast<char*>(&newMS.color), sizeof(newMS.color));
                }
                if (SS1_MDL_bHasDiffuseColor) {
                    outFile.write(reinterpret_cast<char*>(&newMS.diffuseColor), sizeof(newMS.diffuseColor));
                    outFile.write(reinterpret_cast<char*>(&newMS.reflectionColor), sizeof(newMS.reflectionColor));
                    outFile.write(reinterpret_cast<char*>(&newMS.specularColor), sizeof(newMS.specularColor));
                    outFile.write(reinterpret_cast<char*>(&newMS.bumpColor), sizeof(newMS.bumpColor));
                    outFile.write(reinterpret_cast<char*>(&newMS.onColorMask), sizeof(newMS.onColorMask));
                    outFile.write(reinterpret_cast<char*>(&newMS.offColorMask), sizeof(newMS.offColorMask));
                }
            }
            if (SS1_MDL_bHasPolygonalPatches) {
                outFile.write(reinterpret_cast<char*>(&mip.flags), sizeof(mip.flags));
                MemberCount = mip.patches.size();
                outFile.write(reinterpret_cast<char*>(&MemberCount), sizeof(MemberCount));
                for (auto& patch : mip.patches) {
                    outFile.write(reinterpret_cast<char*>(&patch.ctOccupied), sizeof(patch.ctOccupied));
                    if (patch.ctOccupied != 0) {
                        WriteChunkHeader(outFile, "OCPL", 4 * patch.ctOccupied);
                        for (auto& pi : patch.PolygonIndices)
                            outFile.write(reinterpret_cast<char*>(&pi), sizeof(pi));
                    }
                }
            }
        }

        if (!newMDL.NewPatchFormat) {
            uint32_t ulOldExistingPatches = 0;
            uint32_t PatchChunkSize = 4;
            for (int i = 0; i < 32; i++) {
                if (newMDL.patches[i].textureName.size() > 0) {
                    ulOldExistingPatches |= (1UL << i);
                    PatchChunkSize += newMDL.patches[i].textureName.size() + 8;
                }
            }
            WriteChunkHeader(outFile, "STMK", PatchChunkSize);
            outFile.write(reinterpret_cast<char*>(&ulOldExistingPatches), sizeof(ulOldExistingPatches));
            for (int i = 0; i < 32; i++)
            {
                if (((1UL << i) & ulOldExistingPatches) != 0)
                {
                    outFile.write("DFNM", 4);
                    WriteCString(outFile, newMDL.patches[i].textureName);
                }
            }
        }
        else {
            outFile.write("PTC2", 4);
            for (int i = 0; i < 32; i++) {
                WriteCString(outFile, newMDL.patches[i].name);
                outFile.write("DFNM", 4);
                WriteCString(outFile, newMDL.patches[i].textureName);
                outFile.write(reinterpret_cast<char*>(&newMDL.patches[i].posU), sizeof(newMDL.patches[i].posU));
                outFile.write(reinterpret_cast<char*>(&newMDL.patches[i].posV), sizeof(newMDL.patches[i].posV));
                outFile.write(reinterpret_cast<char*>(&newMDL.patches[i].stretch), sizeof(newMDL.patches[i].stretch));
            }
        }
        WriteChunkHeader(outFile, "STXW", 4);
        outFile.write(reinterpret_cast<char*>(&newMDL.texWidth), sizeof(newMDL.texWidth));
        WriteChunkHeader(outFile, "STXH", 4);
        outFile.write(reinterpret_cast<char*>(&newMDL.texHeight), sizeof(newMDL.texHeight));
        if (!newMDL.NewPatchFormat) {
            WriteChunkHeader(outFile, "POSS", 256);
            for (int i = 0; i < 32; i++)
            {
                outFile.write(reinterpret_cast<char*>(&newMDL.patches[i].posU), sizeof(newMDL.patches[i].posU));
                outFile.write(reinterpret_cast<char*>(&newMDL.patches[i].posV), sizeof(newMDL.patches[i].posV));
            }
        }
        if (!SS1_MDL_bHasSavedFlagsOnStart)
            outFile.write(reinterpret_cast<char*>(&newMDL.flags), sizeof(newMDL.flags));
        outFile.write(reinterpret_cast<char*>(&newMDL.shadowQuality), sizeof(newMDL.shadowQuality));
        outFile.write(reinterpret_cast<char*>(&newMDL.stretch[0]), sizeof(newMDL.stretch[0]));
        outFile.write(reinterpret_cast<char*>(&newMDL.stretch[1]), sizeof(newMDL.stretch[1]));
        outFile.write(reinterpret_cast<char*>(&newMDL.stretch[2]), sizeof(newMDL.stretch[2]));
        if (SS1_MDL_bHasSavedCenter) {
            outFile.write(reinterpret_cast<char*>(&newMDL.center[0]), sizeof(newMDL.center[0]));
            outFile.write(reinterpret_cast<char*>(&newMDL.center[1]), sizeof(newMDL.center[1]));
            outFile.write(reinterpret_cast<char*>(&newMDL.center[2]), sizeof(newMDL.center[2]));
        }
        if (SS1_MDL_bHasMultipleCollisionBoxes) {
            uint32_t ctCollisionBoxes = newMDL.collisionBoxes.size();
            outFile.write(reinterpret_cast<char*>(&ctCollisionBoxes), sizeof(ctCollisionBoxes));
            for (auto& newCB : newMDL.collisionBoxes) {
                outFile.write(reinterpret_cast<char*>(&newCB.minX), sizeof(newCB.minX));
                outFile.write(reinterpret_cast<char*>(&newCB.minY), sizeof(newCB.minY));
                outFile.write(reinterpret_cast<char*>(&newCB.minZ), sizeof(newCB.minZ));
                outFile.write(reinterpret_cast<char*>(&newCB.maxX), sizeof(newCB.maxX));
                outFile.write(reinterpret_cast<char*>(&newCB.maxY), sizeof(newCB.maxY));
                outFile.write(reinterpret_cast<char*>(&newCB.maxZ), sizeof(newCB.maxZ));
                WriteCString(outFile, newCB.name);
            }
        }
        else {
            outFile.write(reinterpret_cast<char*>(&newMDL.collisionBoxes[0].minX), sizeof(newMDL.collisionBoxes[0].minX));
            outFile.write(reinterpret_cast<char*>(&newMDL.collisionBoxes[0].minY), sizeof(newMDL.collisionBoxes[0].minY));
            outFile.write(reinterpret_cast<char*>(&newMDL.collisionBoxes[0].minZ), sizeof(newMDL.collisionBoxes[0].minZ));
            outFile.write(reinterpret_cast<char*>(&newMDL.collisionBoxes[0].maxX), sizeof(newMDL.collisionBoxes[0].maxX));
            outFile.write(reinterpret_cast<char*>(&newMDL.collisionBoxes[0].maxY), sizeof(newMDL.collisionBoxes[0].maxY));
            outFile.write(reinterpret_cast<char*>(&newMDL.collisionBoxes[0].maxZ), sizeof(newMDL.collisionBoxes[0].maxZ));
        }
        
        if (SS1_MDL_bHasCOLIChunk) {
            outFile.write("COLI", 4);
            Write4ByteBool(outFile, newMDL.collideAsCube);
        }
        
        if (SS1_MDL_bHasAttachedPositions) {
            uint32_t ctAttachedPositions = newMDL.attachedPositions.size();
            outFile.write(reinterpret_cast<char*>(&ctAttachedPositions), sizeof(ctAttachedPositions));
            for (auto& newap : newMDL.attachedPositions) {
                outFile.write(reinterpret_cast<char*>(&newap.centerVertex), sizeof(newap.centerVertex));
                outFile.write(reinterpret_cast<char*>(&newap.frontVertex), sizeof(newap.frontVertex));
                outFile.write(reinterpret_cast<char*>(&newap.upVertex), sizeof(newap.upVertex));
                outFile.write(reinterpret_cast<char*>(&newap.Position[0]), sizeof(newap.Position[0]));
                outFile.write(reinterpret_cast<char*>(&newap.Position[1]), sizeof(newap.Position[1]));
                outFile.write(reinterpret_cast<char*>(&newap.Position[2]), sizeof(newap.Position[2]));
                outFile.write(reinterpret_cast<char*>(&newap.Angle[0]), sizeof(newap.Angle[0]));
                outFile.write(reinterpret_cast<char*>(&newap.Angle[1]), sizeof(newap.Angle[1]));
                outFile.write(reinterpret_cast<char*>(&newap.Angle[2]), sizeof(newap.Angle[2]));
            }
        }
        WriteChunkHeader(outFile, "ICLN", 4);
        uint32_t iValidColorsCt = 0;
        for (const auto& cn : newMDL.colorNames)
            if (!cn.empty())
                iValidColorsCt++;
        outFile.write(reinterpret_cast<char*>(&iValidColorsCt), sizeof(iValidColorsCt));
        for (int i = 0; i < 32; i++) {
            if (newMDL.colorNames[i].size() != 0) {
                outFile.write(reinterpret_cast<char*>(&i), sizeof(i));
                WriteCString(outFile, newMDL.colorNames[i]);
            }
        }
        outFile.write("ADAT", 4);
        int ad_NumberOfAnims = newMDL.animations.size();
        outFile.write(reinterpret_cast<char*>(&ad_NumberOfAnims), sizeof(ad_NumberOfAnims));
        for (auto& anm : newMDL.animations) {
            char name[32] = { 0 };
            strncpy(name, anm.name, 31);
            outFile.write(name, 32);
            outFile.write(reinterpret_cast<char*>(&anm.SecondsPerFrame), sizeof(anm.SecondsPerFrame));
            int32_t numFrames = anm.FrameIndices.size();
            outFile.write(reinterpret_cast<char*>(&numFrames), sizeof(numFrames));
            for (auto& fi : anm.FrameIndices)
                outFile.write(reinterpret_cast<char*>(&fi), sizeof(fi));
        }
        if (SS1_MDL_bHasDiffuseColor)
            outFile.write(reinterpret_cast<char*>(&newMDL.colorDiffuse), sizeof(newMDL.colorDiffuse));
        if (SS1_MDL_bHasColorForReflectionAndSpecularity) {
            outFile.write(reinterpret_cast<char*>(&newMDL.colorReflections), sizeof(newMDL.colorReflections));
            outFile.write(reinterpret_cast<char*>(&newMDL.colorSpecular), sizeof(newMDL.colorSpecular));
            outFile.write(reinterpret_cast<char*>(&newMDL.colorBump), sizeof(newMDL.colorBump));
        }

        outFile.close();
        cout << "MDL constructed successfully: " << outpath << endl;
    }
    catch (exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}

SS1_MDL_file ReadSS1MDL(const fs::path& filePath) {
    ifstream inFile(filePath, ios::binary);
    SS1_MDL_file newMDL;
    
    inFile.read(ChunkBuffer, 4);//get past the header
    inFile.read(ChunkBuffer, 4);//get the version
    SS1_MDL_SetModelVersionFlags(ChunkBuffer);
    newMDL.version = string(ChunkBuffer, 4);
    if (SS1_MDL_bHasSavedFlagsOnStart)
        inFile.read(reinterpret_cast<char*>(&newMDL.flags), sizeof(newMDL.flags));
    else
        newMDL.flags = 0;
    
    ReadChunkHeader(inFile); //IVTX
    inFile.read(reinterpret_cast<char*>(&newMDL.verticesCount), sizeof(newMDL.verticesCount));
    ReadChunkHeader(inFile); //IFRM
    inFile.read(reinterpret_cast<char*>(&newMDL.framesCount), sizeof(newMDL.framesCount));
    if (newMDL.flags & 16) {
        ReadChunkHeader(inFile); //AV16 or 17
        if (memcmp(ChunkBuffer, "AV16", 4) == 0) {
            for (int i = 0; i < newMDL.verticesCount * newMDL.framesCount; i++) {
                ss1_mdl_Vertex16_old newVert16old;
                inFile.read(reinterpret_cast<char*>(&newVert16old.x), sizeof(newVert16old.x));
                inFile.read(reinterpret_cast<char*>(&newVert16old.y), sizeof(newVert16old.y));
                inFile.read(reinterpret_cast<char*>(&newVert16old.z), sizeof(newVert16old.z));
                inFile.read(reinterpret_cast<char*>(&newVert16old.normalIndex), sizeof(newVert16old.normalIndex));
                uint8_t padding;
                inFile.read(reinterpret_cast<char*>(&padding), sizeof(padding));//this is to be discarded
                newMDL.frameVertices16_old.push_back(newVert16old);
            }
        }
        else if (memcmp(ChunkBuffer, "AV17", 4) == 0) {
            for (int i = 0; i < newMDL.verticesCount * newMDL.framesCount; i++) {
                ss1_mdl_Vertex16 newVert16;
                inFile.read(reinterpret_cast<char*>(&newVert16.x), sizeof(newVert16.x));
                inFile.read(reinterpret_cast<char*>(&newVert16.y), sizeof(newVert16.y));
                inFile.read(reinterpret_cast<char*>(&newVert16.z), sizeof(newVert16.z));
                inFile.read(reinterpret_cast<char*>(&newVert16.normH), sizeof(newVert16.normH));
                inFile.read(reinterpret_cast<char*>(&newVert16.normP), sizeof(newVert16.normP));
                newMDL.frameVertices16.push_back(newVert16);
            }
        }
        else
        {
            cout << "Invalid Chunk ID: " << ChunkBuffer << endl;
            exit(1);
        }
    }
    else {
        ReadChunkHeader(inFile); //AFVX
        for (int i = 0; i < newMDL.verticesCount * newMDL.framesCount; i++) {
            ss1_mdl_Vertex8 newVert8;
            inFile.read(reinterpret_cast<char*>(&newVert8.x), sizeof(newVert8.x));
            inFile.read(reinterpret_cast<char*>(&newVert8.y), sizeof(newVert8.y));
            inFile.read(reinterpret_cast<char*>(&newVert8.z), sizeof(newVert8.z));
            inFile.read(reinterpret_cast<char*>(&newVert8.normalIndex), sizeof(newVert8.normalIndex));
            newMDL.frameVertices8.push_back(newVert8);
        }
    }

    ReadChunkHeader(inFile); //AFIN
    for (int i = 0; i < newMDL.framesCount; i++) {
        ss1_mdl_ModelFrameInfo newfi;
        inFile.read(reinterpret_cast<char*>(&newfi.MinX), sizeof(newfi.MinX));
        inFile.read(reinterpret_cast<char*>(&newfi.MinY), sizeof(newfi.MinY));
        inFile.read(reinterpret_cast<char*>(&newfi.MinZ), sizeof(newfi.MinZ));
        inFile.read(reinterpret_cast<char*>(&newfi.MaxX), sizeof(newfi.MaxX));
        inFile.read(reinterpret_cast<char*>(&newfi.MaxY), sizeof(newfi.MaxY));
        inFile.read(reinterpret_cast<char*>(&newfi.MaxZ), sizeof(newfi.MaxZ));
        newMDL.frameInfos.push_back(newfi);
    }
    ReadChunkHeader(inFile); //AMMV
    for (int i = 0; i < newMDL.verticesCount; i++) {
        ss1_mdl_MipVertex newmv;
        inFile.read(reinterpret_cast<char*>(&newmv.x), sizeof(newmv.x));
        inFile.read(reinterpret_cast<char*>(&newmv.y), sizeof(newmv.y));
        inFile.read(reinterpret_cast<char*>(&newmv.z), sizeof(newmv.z));
        newMDL.mainMipVertices.push_back(newmv);
    }
    ReadChunkHeader(inFile); //AVMK
    for (int i = 0; i < newMDL.verticesCount; i++) {
        uint32_t newmm;
        inFile.read(reinterpret_cast<char*>(&newmm), sizeof(newmm));
        newMDL.vertexMipMask.push_back(newmm);
    }
    ReadChunkHeader(inFile); //IMIP
    inFile.read(reinterpret_cast<char*>(&newMDL.mipCount), sizeof(newMDL.mipCount));
    ReadChunkHeader(inFile); //FMIP
    for (int i = 0; i < 32; i++)
        inFile.read(reinterpret_cast<char*>(&newMDL.mipSwitchFactors[i]), sizeof(newMDL.mipSwitchFactors[i]));
    for (int i = 0; i < newMDL.mipCount; i++) {
        ss1_mdl_Mip newmip;
        ReadChunkHeader(inFile);//IPOL
        uint32_t MemberCount;
        inFile.read(reinterpret_cast<char*>(&MemberCount), sizeof(MemberCount));
        for (int j = 0; j < MemberCount; j++) {
            ss1_mdl_Polygon newPoly;
            uint32_t VertexCount;
            inFile.read(ChunkBuffer, 4);//get polygon format
            if (memcmp(ChunkBuffer, "MDPL", 4) == 0)
            {
                newPoly.newformat = false;
                inFile.read(ChunkBuffer, 4);
                inFile.read(ChunkBuffer, 4);//skip the old sub-chunk header
                inFile.read(reinterpret_cast<char*>(&VertexCount), sizeof(VertexCount));
                for (int k = 0; k < VertexCount; k++) {
                    ss1_mdl_PolygonVertex newVert;
                    inFile.read(reinterpret_cast<char*>(&newVert.transformedVertexIndex), sizeof(newVert.transformedVertexIndex));
                    inFile.read(reinterpret_cast<char*>(&newVert.textureVertexIndex), sizeof(newVert.textureVertexIndex));
                    newPoly.vertices.push_back(newVert);
                }
                inFile.read(reinterpret_cast<char*>(&newPoly.renderFlags), sizeof(newPoly.renderFlags));
                inFile.read(reinterpret_cast<char*>(&newPoly.colorAndAlpha), sizeof(newPoly.colorAndAlpha));
                inFile.read(reinterpret_cast<char*>(&newPoly.surfaceIndex), sizeof(newPoly.surfaceIndex));
                inFile.read(reinterpret_cast<char*>(&newPoly.exONcolor), sizeof(newPoly.exONcolor));
                inFile.read(reinterpret_cast<char*>(&newPoly.exOFFcolor), sizeof(newPoly.exOFFcolor));
                newmip.polygons.push_back(newPoly);
            }
            else if (memcmp(ChunkBuffer, "MDP2", 4) == 0)
            {
                newPoly.newformat = true;
                inFile.read(reinterpret_cast<char*>(&VertexCount), sizeof(VertexCount));
                for (int k = 0; k < VertexCount; k++) {
                    ss1_mdl_PolygonVertex newVert;
                    inFile.read(reinterpret_cast<char*>(&newVert.transformedVertexIndex), sizeof(newVert.transformedVertexIndex));
                    inFile.read(reinterpret_cast<char*>(&newVert.textureVertexIndex), sizeof(newVert.textureVertexIndex));
                    newPoly.vertices.push_back(newVert);
                }
                inFile.read(reinterpret_cast<char*>(&newPoly.renderFlags), sizeof(newPoly.renderFlags));
                inFile.read(reinterpret_cast<char*>(&newPoly.colorAndAlpha), sizeof(newPoly.colorAndAlpha));
                inFile.read(reinterpret_cast<char*>(&newPoly.surfaceIndex), sizeof(newPoly.surfaceIndex));
                newmip.polygons.push_back(newPoly);
            }
        }
        inFile.read(reinterpret_cast<char*>(&MemberCount), sizeof(MemberCount));
        ReadChunkHeader(inFile); //TXVT or TXV2
        for (int j = 0; j < MemberCount; j++) {
            ss1_mdl_TextureVertex newTV;
            inFile.read(reinterpret_cast<char*>(&newTV.uvwX), sizeof(newTV.uvwX));
            inFile.read(reinterpret_cast<char*>(&newTV.uvwY), sizeof(newTV.uvwY));
            inFile.read(reinterpret_cast<char*>(&newTV.uvwZ), sizeof(newTV.uvwZ));
            inFile.read(reinterpret_cast<char*>(&newTV.u), sizeof(newTV.u));
            inFile.read(reinterpret_cast<char*>(&newTV.v), sizeof(newTV.v));
            if (SS1_MDL_bHasPolygonsPerSurface) {
                if (memcmp(ChunkBuffer, "TXV2", 4) == 0) {
                    newTV.newformat = true;
                    inFile.read(reinterpret_cast<char*>(&newTV.SurfaceIndex), sizeof(newTV.SurfaceIndex));
                    inFile.read(reinterpret_cast<char*>(&newTV.transformedVertexIndex), sizeof(newTV.transformedVertexIndex));
                    inFile.read(reinterpret_cast<char*>(&newTV.bumpU[0]), sizeof(newTV.bumpU[0]));
                    inFile.read(reinterpret_cast<char*>(&newTV.bumpU[1]), sizeof(newTV.bumpU[1]));
                    inFile.read(reinterpret_cast<char*>(&newTV.bumpU[2]), sizeof(newTV.bumpU[2]));
                    inFile.read(reinterpret_cast<char*>(&newTV.bumpV[0]), sizeof(newTV.bumpV[0]));
                    inFile.read(reinterpret_cast<char*>(&newTV.bumpV[1]), sizeof(newTV.bumpV[1]));
                    inFile.read(reinterpret_cast<char*>(&newTV.bumpV[2]), sizeof(newTV.bumpV[2]));
                }
                else {
                    newTV.done = Read4ByteBool(inFile);
                    inFile.read(reinterpret_cast<char*>(&newTV.transformedVertexIndex), sizeof(newTV.transformedVertexIndex));
                }
            }
            else
                newTV.done = Read4ByteBool(inFile);
            newmip.textureVertices.push_back(newTV);
        }
        inFile.read(reinterpret_cast<char*>(&MemberCount), sizeof(MemberCount));
        for (int j = 0; j < MemberCount; j++) {
            ss1_mdl_MappingSurface newMS;
            newMS.name = ReadCString(inFile);
            inFile.read(reinterpret_cast<char*>(&newMS.surfaceOffsetX), sizeof(newMS.surfaceOffsetX));
            inFile.read(reinterpret_cast<char*>(&newMS.surfaceOffsetY), sizeof(newMS.surfaceOffsetY));
            inFile.read(reinterpret_cast<char*>(&newMS.surfaceOffsetZ), sizeof(newMS.surfaceOffsetZ));
            inFile.read(reinterpret_cast<char*>(&newMS.h), sizeof(newMS.h));
            inFile.read(reinterpret_cast<char*>(&newMS.p), sizeof(newMS.p));
            inFile.read(reinterpret_cast<char*>(&newMS.b), sizeof(newMS.b));
            inFile.read(reinterpret_cast<char*>(&newMS.zoom), sizeof(newMS.zoom));
            if (SS1_MDL_bHasPolygonsPerSurface) {
                inFile.read(reinterpret_cast<char*>(&newMS.shadingType), sizeof(newMS.shadingType));
                inFile.read(reinterpret_cast<char*>(&newMS.translucencyType), sizeof(newMS.translucencyType));
                inFile.read(reinterpret_cast<char*>(&newMS.renderingFlags), sizeof(newMS.renderingFlags));
                uint32_t ItemCount;
                inFile.read(reinterpret_cast<char*>(&ItemCount), sizeof(ItemCount));
                for (int k = 0; k < ItemCount; k++) {
                    uint32_t npi;
                    inFile.read(reinterpret_cast<char*>(&npi), sizeof(npi));
                    newMS.polygonIndices.push_back(npi);
                }
                inFile.read(reinterpret_cast<char*>(&ItemCount), sizeof(ItemCount));
                for (int k = 0; k < ItemCount; k++) {
                    uint32_t tvi;
                    inFile.read(reinterpret_cast<char*>(&tvi), sizeof(tvi));
                    newMS.textureVertexIndices.push_back(tvi);
                }
                inFile.read(reinterpret_cast<char*>(&newMS.color), sizeof(newMS.color));
            }
            if (SS1_MDL_bHasDiffuseColor) {
                inFile.read(reinterpret_cast<char*>(&newMS.diffuseColor), sizeof(newMS.diffuseColor));
                inFile.read(reinterpret_cast<char*>(&newMS.reflectionColor), sizeof(newMS.reflectionColor));
                inFile.read(reinterpret_cast<char*>(&newMS.specularColor), sizeof(newMS.specularColor));
                inFile.read(reinterpret_cast<char*>(&newMS.bumpColor), sizeof(newMS.bumpColor));
                inFile.read(reinterpret_cast<char*>(&newMS.onColorMask), sizeof(newMS.onColorMask));
                inFile.read(reinterpret_cast<char*>(&newMS.offColorMask), sizeof(newMS.offColorMask));
            }
            newmip.mappingSurfaces.push_back(newMS);
        }
        if (SS1_MDL_bHasPolygonalPatches) {
            inFile.read(reinterpret_cast<char*>(&newmip.flags), sizeof(newmip.flags));
            inFile.read(reinterpret_cast<char*>(&MemberCount), sizeof(MemberCount));
            for (int j = 0; j < MemberCount; j++) {
                ss1_mdl_PolygonsPerPatch newppp;
                inFile.read(reinterpret_cast<char*>(&newppp.ctOccupied), sizeof(newppp.ctOccupied));
                if (newppp.ctOccupied != 0) {
                    ReadChunkHeader(inFile); //OCPL
                    for (int k = 0; k < newppp.ctOccupied; k++) {
                        int32_t newpi;
                        inFile.read(reinterpret_cast<char*>(&newpi), sizeof(newpi));
                        newppp.PolygonIndices.push_back(newpi);
                    }
                }
                newmip.patches.push_back(newppp);
            }
        }
        newMDL.mips.push_back(newmip);
    }
    inFile.read(ChunkBuffer, 4); //STMK or PTC2
    inFile.seekg(-4, ios_base::cur);
    if (memcmp(ChunkBuffer, "STMK", 4) == 0)
    {
        ReadChunkHeader(inFile);
        newMDL.NewPatchFormat = false;
        uint32_t ulOldExistingPatches;
        inFile.read(reinterpret_cast<char*>(&ulOldExistingPatches), sizeof(ulOldExistingPatches));
        for (int i = 0; i < 32; i++)
        {
            if (((1UL << i) & ulOldExistingPatches) != 0)
            {
                inFile.read(ChunkBuffer, 4); //DFNM
                newMDL.patches[i].textureName = ReadCString(inFile);
            }
        }
    }
    else if (memcmp(ChunkBuffer, "PTC2", 4) == 0)
    {
        ReadChunkSubHeader(inFile);
        newMDL.NewPatchFormat = true;
        for (int i = 0; i < 32; i++)
        {
            newMDL.patches[i].name = ReadCString(inFile);
            inFile.read(ChunkBuffer, 4); //DFNM
            newMDL.patches[i].textureName = ReadCString(inFile);
            inFile.read(reinterpret_cast<char*>(&newMDL.patches[i].posU), sizeof(newMDL.patches[i].posU));
            inFile.read(reinterpret_cast<char*>(&newMDL.patches[i].posV), sizeof(newMDL.patches[i].posV));
            inFile.read(reinterpret_cast<char*>(&newMDL.patches[i].stretch), sizeof(newMDL.patches[i].stretch));
        }
    }
    else
    {
        cout << "Expecting chunk containing patch data but found unrecognisable chunk ID: "<< ChunkBuffer << endl;
        exit(1);
    }
    ReadChunkHeader(inFile);//STXW
    inFile.read(reinterpret_cast<char*>(&newMDL.texWidth), sizeof(newMDL.texWidth));
    ReadChunkHeader(inFile);//STXH
    inFile.read(reinterpret_cast<char*>(&newMDL.texHeight), sizeof(newMDL.texHeight));
    if (!newMDL.NewPatchFormat) {
        ReadChunkHeader(inFile);//POSS
        for (int i = 0; i < 32; i++)
        {
            inFile.read(reinterpret_cast<char*>(&newMDL.patches[i].posU), sizeof(newMDL.patches[i].posU));
            inFile.read(reinterpret_cast<char*>(&newMDL.patches[i].posV), sizeof(newMDL.patches[i].posV));
        }
    }
    if (!SS1_MDL_bHasSavedFlagsOnStart)
        inFile.read(reinterpret_cast<char*>(&newMDL.flags), sizeof(newMDL.flags));
    inFile.read(reinterpret_cast<char*>(&newMDL.shadowQuality), sizeof(newMDL.shadowQuality));
    inFile.read(reinterpret_cast<char*>(&newMDL.stretch[0]), sizeof(newMDL.stretch[0]));
    inFile.read(reinterpret_cast<char*>(&newMDL.stretch[1]), sizeof(newMDL.stretch[1]));
    inFile.read(reinterpret_cast<char*>(&newMDL.stretch[2]), sizeof(newMDL.stretch[2]));
    if (SS1_MDL_bHasSavedCenter) {
        inFile.read(reinterpret_cast<char*>(&newMDL.center[0]), sizeof(newMDL.center[0]));
        inFile.read(reinterpret_cast<char*>(&newMDL.center[1]), sizeof(newMDL.center[1]));
        inFile.read(reinterpret_cast<char*>(&newMDL.center[2]), sizeof(newMDL.center[2]));
    }

    if (SS1_MDL_bHasMultipleCollisionBoxes)
    {
        uint32_t ctCollisionBoxes;
        inFile.read(reinterpret_cast<char*>(&ctCollisionBoxes), sizeof(ctCollisionBoxes));
        for (int i = 0; i < ctCollisionBoxes; i++)
        {
            ss1_mdl_CollisionBox newCB;
            inFile.read(reinterpret_cast<char*>(&newCB.minX), sizeof(newCB.minX));
            inFile.read(reinterpret_cast<char*>(&newCB.minY), sizeof(newCB.minY));
            inFile.read(reinterpret_cast<char*>(&newCB.minZ), sizeof(newCB.minZ));
            inFile.read(reinterpret_cast<char*>(&newCB.maxX), sizeof(newCB.maxX));
            inFile.read(reinterpret_cast<char*>(&newCB.maxY), sizeof(newCB.maxY));
            inFile.read(reinterpret_cast<char*>(&newCB.maxZ), sizeof(newCB.maxZ));
            newCB.name = ReadCString(inFile);
            newMDL.collisionBoxes.push_back(newCB);
        }
    }
    else
    {
        ss1_mdl_CollisionBox newCB;
        inFile.read(reinterpret_cast<char*>(&newCB.minX), sizeof(newCB.minX));
        inFile.read(reinterpret_cast<char*>(&newCB.minY), sizeof(newCB.minY));
        inFile.read(reinterpret_cast<char*>(&newCB.minZ), sizeof(newCB.minZ));
        inFile.read(reinterpret_cast<char*>(&newCB.maxX), sizeof(newCB.maxX));
        inFile.read(reinterpret_cast<char*>(&newCB.maxY), sizeof(newCB.maxY));
        inFile.read(reinterpret_cast<char*>(&newCB.maxZ), sizeof(newCB.maxZ));
        //no name for this one
        newMDL.collisionBoxes.push_back(newCB);
    }
    inFile.read(ChunkBuffer, 4);
    inFile.seekg(-4, ios_base::cur);
    if (memcmp(ChunkBuffer, "COLI", 4) == 0) {
        ReadChunkSubHeader(inFile);
        newMDL.collideAsCube = Read4ByteBool(inFile);
        SS1_MDL_bHasCOLIChunk = true;
    }
    else
        newMDL.collideAsCube = false;
    if (SS1_MDL_bHasAttachedPositions)
    {
        uint32_t ctAttachedPositions;
        inFile.read(reinterpret_cast<char*>(&ctAttachedPositions), sizeof(ctAttachedPositions));
        for (int i = 0; i < ctAttachedPositions; i++)
        {
            ss1_mdl_AttachedPosition newap;
            inFile.read(reinterpret_cast<char*>(&newap.centerVertex), sizeof(newap.centerVertex));
            inFile.read(reinterpret_cast<char*>(&newap.frontVertex), sizeof(newap.frontVertex));
            inFile.read(reinterpret_cast<char*>(&newap.upVertex), sizeof(newap.upVertex));
            inFile.read(reinterpret_cast<char*>(&newap.Position[0]), sizeof(newap.Position[0]));
            inFile.read(reinterpret_cast<char*>(&newap.Position[1]), sizeof(newap.Position[1]));
            inFile.read(reinterpret_cast<char*>(&newap.Position[2]), sizeof(newap.Position[2]));
            inFile.read(reinterpret_cast<char*>(&newap.Angle[0]), sizeof(newap.Angle[0]));
            inFile.read(reinterpret_cast<char*>(&newap.Angle[1]), sizeof(newap.Angle[1]));
            inFile.read(reinterpret_cast<char*>(&newap.Angle[2]), sizeof(newap.Angle[2]));
            newMDL.attachedPositions.push_back(newap);
        }
    }
    ReadChunkHeader(inFile);//ICLN
    int iValidColorsCt;
    inFile.read(reinterpret_cast<char*>(&iValidColorsCt), sizeof(iValidColorsCt));
    for (int i = 0; i < iValidColorsCt; i++) {
        int iExistingColorName;
        inFile.read(reinterpret_cast<char*>(&iExistingColorName), sizeof(iExistingColorName));
        newMDL.colorNames[iExistingColorName] = ReadCString(inFile);
    }
    ReadChunkSubHeader(inFile);//ADAT
    int ad_NumberOfAnims;
    inFile.read(reinterpret_cast<char*>(&ad_NumberOfAnims), sizeof(ad_NumberOfAnims));
    for (int i = 0; i < ad_NumberOfAnims; i++) {
        ss1_mdl_AnimData newad;
        inFile.read(newad.name, 32);
        inFile.read(reinterpret_cast<char*>(&newad.SecondsPerFrame), sizeof(newad.SecondsPerFrame));
        int32_t numFrames;
        inFile.read(reinterpret_cast<char*>(&numFrames), sizeof(numFrames));
        for (int j = 0; j < numFrames; j++) {
            int32_t newfi;
            inFile.read(reinterpret_cast<char*>(&newfi), sizeof(newfi));
            newad.FrameIndices.push_back(newfi);
        }
        newMDL.animations.push_back(newad);
    }
    if (SS1_MDL_bHasDiffuseColor)
        inFile.read(reinterpret_cast<char*>(&newMDL.colorDiffuse), sizeof(newMDL.colorDiffuse));
    if (SS1_MDL_bHasColorForReflectionAndSpecularity)
    {
        inFile.read(reinterpret_cast<char*>(&newMDL.colorReflections), sizeof(newMDL.colorReflections));
        inFile.read(reinterpret_cast<char*>(&newMDL.colorSpecular), sizeof(newMDL.colorSpecular));
        inFile.read(reinterpret_cast<char*>(&newMDL.colorBump), sizeof(newMDL.colorBump));
    }

    if (inFile.eof())
        cout << "Warning: Parser has went beyond EoF!" << endl;
    else {
        inFile.read(ChunkBuffer, 1);
        if (inFile.eof())
            cout << "EoF reached successfully!" << endl;
        else
            cout << "Warning: Parser stopped before reaching EoF!" << endl;
    }
    inFile.close();
    return newMDL;
}

void SS1MDL2JSON(const SS1_MDL_file& newMDL, const fs::path& outPath) {
    json jsonMDL;
    jsonMDL["Format"] = "SeriousSam1MDL";
    
    json jmodelinfo;
    jmodelinfo["ident"] = "MDAT";
    jmodelinfo["version"] = newMDL.version;
    jmodelinfo["flags"] = newMDL.flags;
    jmodelinfo["verticesCount"] = newMDL.verticesCount;
    jmodelinfo["framesCount"] = newMDL.framesCount;
    jmodelinfo["mipCount"] = newMDL.mipCount;
    jmodelinfo["texWidth"] = newMDL.texWidth;
    jmodelinfo["texHeight"] = newMDL.texHeight;
    jmodelinfo["shadowQuality"] = newMDL.shadowQuality;
    jmodelinfo["stretch"] = { newMDL.stretch[0], newMDL.stretch[1], newMDL.stretch[2] };
    if (SS1_MDL_bHasSavedCenter)
        jmodelinfo["center"] = { newMDL.center[0], newMDL.center[1], newMDL.center[2] };
    else
        jmodelinfo["center"] = "";
    jsonMDL["ModelInfo"] = jmodelinfo;

    jsonMDL["AV16"] = json::array();
    for (const auto& vert : newMDL.frameVertices16_old) {
        json v;
        v = { vert.x, vert.y, vert.z, vert.normalIndex };
        jsonMDL["AV16"].push_back(v);
    }
    jsonMDL["AV17"] = json::array();
    for (const auto& vert : newMDL.frameVertices16) {
        json v;
        v = { vert.x, vert.y, vert.z, vert.normH, vert.normP };
        jsonMDL["AV17"].push_back(v);
    }
    jsonMDL["AFVX"] = json::array();
    for (const auto& vert : newMDL.frameVertices8) {
        json v;
        v = { vert.x, vert.y, vert.z, vert.normalIndex };
        jsonMDL["AFVX"].push_back(v);
    }

    jsonMDL["frameInfos"] = json::array();
    for (const auto& fi : newMDL.frameInfos) {
        json jfi;
        jfi = { fi.MinX, fi.MinY, fi.MinZ, fi.MaxX, fi.MaxY, fi.MaxZ };
        jsonMDL["frameInfos"].push_back(jfi);
    }
    jsonMDL["mainMipVertices"] = json::array();
    for (const auto& mv : newMDL.mainMipVertices) {
        json jmv;
        jmv = { mv.x, mv.y, mv.z };
        jsonMDL["mainMipVertices"].push_back(jmv);
    }
    jsonMDL["vertexMipMask"] = json::array();;
    for (const auto& mv : newMDL.vertexMipMask) {
        jsonMDL["vertexMipMask"].push_back(mv);
    }
    jsonMDL["mipSwitchFactors"] = json::array();;
    for (const auto& sf : newMDL.mipSwitchFactors) {
        jsonMDL["mipSwitchFactors"].push_back(sf);
    }

    jsonMDL["mips"] = json::array();;
    for (const auto& mip : newMDL.mips) {
        json jmip;
        jmip["polygons"] = json::array();
        for (const auto& poly : mip.polygons) {
            json jpoly;
            jpoly["newformat"] = poly.newformat;
            jpoly["vertices"] = json::array();
            for (const auto& pv : poly.vertices) {
                json jpv;
                jpv = { pv.transformedVertexIndex, pv.textureVertexIndex };
                jpoly["vertices"].push_back(jpv);
            }
            jpoly["renderFlags"] = poly.renderFlags;
            jpoly["colorAndAlpha"] = poly.colorAndAlpha;
            jpoly["surfaceIndex"] = poly.surfaceIndex;
            if (poly.newformat) {
                jpoly["exONcolor"] = "";
                jpoly["exOFFcolor"] = "";
            }
            else {
                jpoly["exONcolor"] = poly.exONcolor;
                jpoly["exOFFcolor"] = poly.exOFFcolor;
            }
            jmip["polygons"].push_back(jpoly);
        }
        jmip["textureVertices"] = json::array();
        for (const auto& tv : mip.textureVertices) {
            json jtv;
            jtv["newformat"] = tv.newformat;
            jtv["UVW"] = { tv.uvwX, tv.uvwY, tv.uvwZ };
            jtv["UV"] = { tv.u, tv.v };
            if(!tv.newformat || !SS1_MDL_bHasPolygonsPerSurface)
                jtv["done"] = tv.done;
            else
                jtv["done"] = "";
            if (SS1_MDL_bHasPolygonsPerSurface) {
                jtv["SurfaceIndex"] = tv.SurfaceIndex;
                jtv["transformedVertexIndex"] = tv.transformedVertexIndex;
                jtv["bumpU"] = { tv.bumpU[0], tv.bumpU[1], tv.bumpU[2]};
                jtv["bumpV"] = { tv.bumpV[0], tv.bumpV[1], tv.bumpV[2]};
            }
            else {
                jtv["SurfaceIndex"] = "";
                jtv["transformedVertexIndex"] = "";
                jtv["bumpU"] = "";
                jtv["bumpV"] = "";
            }
            jmip["textureVertices"].push_back(jtv);
        }
        jmip["mappingSurfaces"] = json::array();
        for (const auto& ms : mip.mappingSurfaces) {
            json jms;
            jms["name"] = ms.name;
            jms["surfaceOffset"] = { ms.surfaceOffsetX, ms.surfaceOffsetY, ms.surfaceOffsetZ };
            jms["hpb"] = { ms.h, ms.p, ms.b };
            jms["zoom"] = ms.zoom;
            if (SS1_MDL_bHasPolygonsPerSurface) {
                jms["shadingType"] = ms.shadingType;
                jms["translucencyType"] = ms.translucencyType;
                jms["renderingFlags"] = ms.renderingFlags;
                jms["polygonIndices"] = json::array();
                for (const auto& pi : ms.polygonIndices)
                    jms["polygonIndices"].push_back(pi);
                jms["textureVertexIndices"] = json::array();
                for (const auto& tvi : ms.textureVertexIndices)
                    jms["textureVertexIndices"].push_back(tvi);
                jms["color"] = ms.color;
            }
            else {
                jms["shadingType"] = "";
                jms["translucencyType"] = "";
                jms["renderingFlags"] = "";
                jms["polygonIndices"] = "";
                jms["textureVertexIndices"] = "";
                jms["color"] = "";
            }
            if (SS1_MDL_bHasDiffuseColor) {
                jms["diffuseColor"] = ms.diffuseColor;
                jms["reflectionColor"] = ms.reflectionColor;
                jms["specularColor"] = ms.specularColor;
                jms["bumpColor"] = ms.bumpColor;
                jms["onColorMask"] = ms.onColorMask;
                jms["offColorMask"] = ms.offColorMask;
            }
            else
            {
                jms["diffuseColor"] = "";
                jms["reflectionColor"] = "";
                jms["specularColor"] = "";
                jms["bumpColor"] = "";
                jms["onColorMask"] = "";
                jms["offColorMask"] = "";
            }
            jmip["mappingSurfaces"].push_back(jms);
        }
        if (SS1_MDL_bHasPolygonalPatches) {
            jmip["flags"] = mip.flags;
            jmip["patches"] = json::array();
            for (const auto& patch : mip.patches) {
                json jpatch;
                jpatch["ctOccupied"] = patch.ctOccupied;
                jpatch["PolygonIndices"] = json::array();
                for (const auto& pi : patch.PolygonIndices)
                    jpatch["PolygonIndices"].push_back(pi);
                jmip["patches"].push_back(jpatch);
            }
        }
        else {
            jmip["flags"] = "";
            jmip["patches"] = "";
        }
        jsonMDL["mips"].push_back(jmip);
    }

    json jpatchinfo;
    jpatchinfo["NewFormat"] = newMDL.NewPatchFormat;
    jpatchinfo["Entries"] = json::array();
    for (const auto& patch : newMDL.patches) {
        json jp;
        jp["name"] = patch.name;
        jp["textureName"] = patch.textureName;
        jp["position"] = { patch.posU, patch.posV };
        if (newMDL.NewPatchFormat)
            jp["stretch"] = patch.stretch;
        else
            jp["stretch"] = "";
        jpatchinfo["Entries"].push_back(jp);
    }
    jsonMDL["Patches"] = jpatchinfo;

    json jcollisioninfo;
    if (SS1_MDL_bHasCOLIChunk)
        jcollisioninfo["collideAsCube"] = newMDL.collideAsCube;
    else
        jcollisioninfo["collideAsCube"] = nullptr;
    jcollisioninfo["Entries"] = json::array();
    for (const auto& cb : newMDL.collisionBoxes) {
        json jcb;
        jcb["min"] = { cb.minX, cb.minY, cb.minZ };
        jcb["max"] = { cb.maxX, cb.maxY, cb.maxZ };
        jcb["name"] = cb.name;
        jcollisioninfo["Entries"].push_back(jcb);
    }
    jsonMDL["CollisionBoxes"] = jcollisioninfo;

    jsonMDL["attachedPositions"] = json::array();
    for (const auto& ap : newMDL.attachedPositions) {
        json jap;
        jap["centerVertex"] = ap.centerVertex;
        jap["frontVertex"] = ap.frontVertex;
        jap["upVertex"] = ap.upVertex;
        jap["Position"] = { ap.Position[0], ap.Position[1], ap.Position[2] };
        jap["Angle"] = { ap.Angle[0], ap.Angle[1], ap.Angle[2] };
        jsonMDL["attachedPositions"].push_back(jap);
    }

    jsonMDL["colorNames"] = json::array();;
    for (const auto& cn : newMDL.colorNames) {
        jsonMDL["colorNames"].push_back(cn);
    }

    jsonMDL["animations"] = json::array();
    for (const auto& anm : newMDL.animations) {
        json jan;
        jan["name"] = anm.name;
        jan["SecondsPerFrame"] = anm.SecondsPerFrame;
        jan["FrameIndices"] = json::array();
        for (const auto& fi : anm.FrameIndices)
            jan["FrameIndices"].push_back(fi);
        jsonMDL["animations"].push_back(jan);
    }
    
    json jcolourinfo;
    if(SS1_MDL_bHasDiffuseColor)
        jcolourinfo["Diffuse"] = newMDL.colorDiffuse;
    if (SS1_MDL_bHasColorForReflectionAndSpecularity) {
        jcolourinfo["Reflections"] = newMDL.colorReflections;
        jcolourinfo["Specular"] = newMDL.colorSpecular;
        jcolourinfo["Bump"] = newMDL.colorBump;
    }
    jsonMDL["Colors"] = jcolourinfo;

    cout << "JSON prepped, time to export." << endl;
    ofstream outFile(outPath);
    if (outFile.is_open()) {
        outFile << jsonMDL.dump(2);
        outFile.close();
        cout << "JSON created successfully: " << outPath << endl;
    }
    else {
        cout << "Error: Could not write output file." << endl;
    }
}

 