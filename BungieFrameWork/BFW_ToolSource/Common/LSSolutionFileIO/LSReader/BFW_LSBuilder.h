/*
	FILE:	BFW_LSBuilder.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Jan 1, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/


#ifndef _BFW_LSBUIDLER_H
#define _BFW_LSBUIDLER_H

#include "rmap.h"
#include "LsBaseBuilders.h"

#include "BFW_LSSolution.h"

class BFW_LSBuilder;


//-----------------------------------------------------------------------
//				
//					Factory Classes
//


//-----------------------------------------------------------------------
//Class BFW_LSBuilderFactory
//
//Main factory class for all BFW_LS*Builder classes. 
//
class BFW_LSBuilderFactory : public LtTBuilderFactory
{

public:
    virtual LtTBool Finish();
	BFW_LSBuilderFactory(LStData* lsData, UUtBool build);

protected:    
    virtual LtTParameterBuilderApi *OnGetParameterBuilder();
	virtual LtTInfoBuilderApi *OnGetInfoBuilder();
    virtual LtTViewBuilderApi *OnGetViewBuilder();
    virtual LtTLayerBuilderApi *OnGetLayerBuilder();
    virtual LtTMaterialBuilderApi *OnGetMaterialBuilder();
    virtual LtTTextureBuilderApi *OnGetTextureBuilder();
    virtual LtTMeshBuilderApi *OnGetMeshBuilder();
    virtual LtTLampBuilderApi *OnGetLampBuilder();
	LtTBool Progress(const Real percent);

private:

	LStData* m_lsData;
	UUtBool	m_build;
};



//-----------------------------------------------------------------------
//				
//					Builder Classes
//


//-----------------------------------------------------------------------
//Class BFW_LSInfoBuilder
//
class BFW_LSInfoBuilder : public LtTBaseInfoBuilder
{
    friend class BFW_LSBuilderFactory;

protected:
    BFW_LSInfoBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild);
	LtTBool Finish(); 

private:
	LStData* m_lsData;
	UUtBool	m_build;
};


//-----------------------------------------------------------------------
//Class BFW_LSParameterBuilder
//
class BFW_LSParameterBuilder : public LtTBaseParameterBuilder
{
    friend class BFW_LSBuilderFactory;

protected:
    BFW_LSParameterBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild);
	LtTBool Finish(); 

private:
	LStData* m_lsData;
	UUtBool	m_build;
};


//-----------------------------------------------------------------------
//Class BFW_LSViewBuilder
//
class BFW_LSViewBuilder : public LtTBaseViewBuilder
{
    friend class BFW_LSBuilderFactory;

protected:
    BFW_LSViewBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild);
	LtTBool Finish(); 

private:
	LStData* m_lsData;
	UUtBool	m_build;
};


//-----------------------------------------------------------------------
//Class BFW_LSLayerBuilder
//
class BFW_LSLayerBuilder : public LtTBaseLayerBuilder
{
    friend class BFW_LSBuilderFactory;

protected:
    BFW_LSLayerBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild);
	LtTBool Finish(); 

private:
	LStData* m_lsData;
	UUtBool	m_build;
};


//-----------------------------------------------------------------------
//Class BFW_LSMaterialBuilder
//
class BFW_LSMaterialBuilder : public LtTBaseMaterialBuilder
{
    friend class BFW_LSBuilderFactory;

protected:
    BFW_LSMaterialBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild);
	LtTBool Finish(); 

private:
	LStData* m_lsData;
	UUtBool	m_build;

};


//-----------------------------------------------------------------------
//Class BFW_LSLampBuilder
//
class BFW_LSLampBuilder : public LtTBaseLampBuilder
{
    friend class BFW_LSBuilderFactory;

protected:
    BFW_LSLampBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild);
	LtTBool Finish(); 

private:
	LStData* m_lsData;
	UUtBool	m_build;
};


//-----------------------------------------------------------------------
//Class BFW_LSTextureBuilder
//
class BFW_LSTextureBuilder : public LtTBaseTextureBuilder
{
    friend class BFW_LSBuilderFactory;

protected:
    BFW_LSTextureBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild);
	LtTBool Finish(); 

private:
	LStData* m_lsData;
	UUtBool	m_build;
};


//-----------------------------------------------------------------------
//Class BFW_LSMeshBuilder
//
//The most complicated class.  Notice that for each face, the vertices,
//irradiances, uvws, norms and faces are found by the appropriate Get*()
//member function.  
//
//The face information is stored in a LtTFace object, described in 
//LtTTypes.h. It is important that you understand the order of the faces
//that are present in the list of faces for the mesh.  In particular, 
//any face that is not a leaf face is followed immediately by its
//descendants.
//
//The InputFaceInfo() member function explains how to traverse the face
//structure to find the vertices, colors, normals and uvws.
class BFW_LSMeshBuilder : public LtTBaseMeshBuilder
{
    friend class BFW_LSBuilderFactory;

protected:
    BFW_LSMeshBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild);
	~BFW_LSMeshBuilder();
	LtTBool Finish(); 

private:
	LStData* m_lsData;
	UUtBool	m_build;
	
	RadianceMap	rmap;
};

#endif