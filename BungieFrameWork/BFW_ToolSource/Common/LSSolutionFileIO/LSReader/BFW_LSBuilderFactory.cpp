/*
	FILE:	BFW_LSBuilderFactory.cpp
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Jan 1, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/

#include "LtTTypes.h"
#include "rmap.h"

#include "BFW_LSSolution.h"
#include "BFW_LSBuilder.h"


BFW_LSBuilderFactory::BFW_LSBuilderFactory(LStData* lsData, UUtBool inBuild)
{
	m_lsData = lsData;
	m_build = inBuild;
}

LtTBool BFW_LSBuilderFactory::Finish()
{
	return TRUE;
}

LtTParameterBuilderApi* BFW_LSBuilderFactory::OnGetParameterBuilder()
{
	return new BFW_LSParameterBuilder( this, m_lsData, m_build );
}


LtTInfoBuilderApi* BFW_LSBuilderFactory::OnGetInfoBuilder()
{
	return new BFW_LSInfoBuilder( this, m_lsData, m_build);
}


LtTViewBuilderApi* BFW_LSBuilderFactory::OnGetViewBuilder()
{
	return new BFW_LSViewBuilder( this, m_lsData, m_build);
}


LtTLayerBuilderApi* BFW_LSBuilderFactory::OnGetLayerBuilder()
{
	return new BFW_LSLayerBuilder( this, m_lsData, m_build);
}


LtTMaterialBuilderApi* BFW_LSBuilderFactory::OnGetMaterialBuilder()
{
	return new BFW_LSMaterialBuilder( this, m_lsData, m_build);
}


LtTTextureBuilderApi* BFW_LSBuilderFactory::OnGetTextureBuilder()
{
	return new BFW_LSTextureBuilder( this, m_lsData, m_build);
}


LtTMeshBuilderApi* BFW_LSBuilderFactory::OnGetMeshBuilder()
{
	return new BFW_LSMeshBuilder( this, m_lsData, m_build);
}


LtTLampBuilderApi* BFW_LSBuilderFactory::OnGetLampBuilder()
{
	return new BFW_LSLampBuilder( this, m_lsData, m_build);
}

LtTBool BFW_LSBuilderFactory::Progress(const Real percent)
{
	return FALSE;
}

