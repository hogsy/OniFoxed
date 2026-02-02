/*
	FILE:	BFW_LSPreperation.h

	AUTHOR:	Brent H. Pease

	CREATED: Nov 20, 1998

	PURPOSE:

	Copyright 1998

*/
#ifndef _BFW_LSPREPERATION_H_
#define _BFW_LSPREPERATION_H_

#include "LpWriter.h"
#include "ParamWiz.h"

class BFW_LPWriter : public LtTPrepfileExport {
public:

    BFW_LPWriter(
		const char*			inGQFileName,
		IMPtEnv_BuildData*	inBuildData);

protected:
    // We override all of the writer methods
    virtual LtTBool OnWriteDefaults(LtTPrepDefaultsApi& writer);
    virtual LtTBool OnWriteDisplay(LtTPrepDisplayApi& writer);
    virtual LtTBool OnWriteReceiver(LtTPrepReceiverApi& writer);
    virtual LtTBool OnWriteSource(LtTPrepSourceApi& writer);
    virtual LtTBool OnWriteShadow(LtTPrepShadowApi& writer);
    virtual LtTBool OnWriteFog(LtTPrepFogApi& writer);
    virtual LtTBool OnWriteNatLight(LtTPrepNatLightApi& writer);
    virtual LtTBool OnWriteView(LtTPrepViewApi& writer);
    virtual LtTBool OnWriteTextures(LtTPrepTextureApi& writer);
    virtual LtTBool OnWriteLayers(LtTPrepLayerApi& writer);
    virtual LtTBool OnWriteLamps(LtTPrepLampApi& writer);
    virtual LtTBool OnWriteMaterials(LtTPrepMaterialApi& writer);
    virtual LtTBool OnWriteBlocks(LtTPrepBlockApi& blocks, LtTPrepEntityApi& entities);
    virtual LtTBool OnWriteEntities(LtTPrepEntityApi& writer);

private:
    // Calculate total area for model
    void calcTotalArea();

    LtTParameterWizard	params;	    // Parameter wizard

	const char*			gqFileName;
	IMPtEnv_BuildData*	buildData;

};

#endif
