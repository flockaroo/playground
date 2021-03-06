include(Common.inc)

# ===== Open Asset Import Library = http://assimp.sf.net ===========================================

cloneAndClean(Assimp, https://github.com/assimp/assimp.git, tags/v3.3.1)

PARAMS  = -DBUILD_SHARED_LIBS=OFF
PARAMS += -DASSIMP_BUILD_ASSIMP_TOOLS=OFF
PARAMS += -DASSIMP_BUILD_TESTS=OFF
PARAMS += -DASSIMP_BUILD_FBX_IMPORTER=OFF
PARAMS += -DASSIMP_BUILD_BLEND_IMPORTER=OFF
PARAMS += -DASSIMP_BUILD_IFC_IMPORTER=OFF
PARAMS += -DASSIMP_BUILD_XGL_IMPORTER=OFF
PARAMS += -DASSIMP_BUILD_3MF_IMPORTER=OFF

buildDebugAndRelease(Assimp, $${PARAMS})
