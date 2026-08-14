#include "SS1MDL.h"

bool SS1_MDL_bHasSavedCenter = false;
bool SS1_MDL_bHasMultipleCollisionBoxes = false;
bool SS1_MDL_bHasAttachedPositions = false;
bool SS1_MDL_bHasPolygonalPatches = false;
bool SS1_MDL_bHasPolygonsPerSurface = false;
bool SS1_MDL_bHasSavedFlagsOnStart = false;
bool SS1_MDL_bHasColorForReflectionAndSpecularity = false;
bool SS1_MDL_bHasDiffuseColor = false;
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
        cout << "Invalid model version: " << version << endl;
        exit(1);
    }
    cout << "Model version: " << version << endl;
}

char ChunkBuffer[4];
uint32_t ChunkSize;
void ReadChunkHeader(ifstream& inFile) {
    inFile.read(ChunkBuffer, 4);
    inFile.read(reinterpret_cast<char*>(&ChunkSize), sizeof(ChunkSize));
    cout << "Parsing Chunk ID: " << ChunkBuffer << " | size: " << ChunkSize << endl;
}
void ReadChunkSubHeader(ifstream& inFile) {
    inFile.read(ChunkBuffer, 4);
    cout << "Parsing Sub-Chunk ID: " << ChunkBuffer << endl;
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

void JSON2SS1MDL(const fs::path& inpath, const fs::path& outpath, const json& jsonMDL) {

}

SS1_MDL_file ParseSS1MDL(const fs::path& filePath) {
    ifstream inFile(filePath, ios::binary);
    SS1_MDL_file newMDL;
    
    inFile.read(ChunkBuffer, 4);//get past the header
    inFile.read(ChunkBuffer, 4);//get the version
    SS1_MDL_SetModelVersionFlags(ChunkBuffer);
    newMDL.version = ChunkBuffer;
    
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
                    inFile.read(reinterpret_cast<char*>(&newTV.done), 4);//booleans have 4 bytes in this game
                    inFile.read(reinterpret_cast<char*>(&newTV.transformedVertexIndex), sizeof(newTV.transformedVertexIndex));
                }
            }
            else
                inFile.read(reinterpret_cast<char*>(&newTV.done), 4);
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
        inFile.read(reinterpret_cast<char*>(&newMDL.collideAsCube), 4);//booleans have 4 bytes in this game
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
    jcollisioninfo["collideAsCube"] = newMDL.collideAsCube;
    jcollisioninfo["Entries"] = json::array();
    for (const auto& cb : newMDL.collisionBoxes) {
        json jcb;
        jcb["min"] = { cb.minX, cb.minY, cb.minZ };
        jcb["max"] = { cb.maxX, cb.maxY, cb.maxZ };
        jcb["name"] = cb.name;
        jcollisioninfo["Entries"].push_back(jcb);
    }
    jsonMDL["Patches"] = jcollisioninfo;

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

    jsonMDL["ss1_mdl_AnimData"] = json::array();
    for (const auto& anm : newMDL.animations) {
        json jan;
        jan["name"] = anm.name;
        jan["SecondsPerFrame"] = anm.SecondsPerFrame;
        jan["FrameIndices"] = json::array();
        for (const auto& fi : anm.FrameIndices)
            jan["FrameIndices"].push_back(fi);
        jsonMDL["ss1_mdl_AnimData"].push_back(jan);
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

 