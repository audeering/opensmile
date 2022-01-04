/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Input: candidates produced by a pitchBase descendant

*/


#ifndef __CFORMANTSMOOTHER_HPP
#define __CFORMANTSMOOTHER_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define BUILD_COMPONENT_FormantSmoother
#define COMPONENT_DESCRIPTION_CFORMANTSMOOTHER "This component performs temporal formant smoothing. Input: candidates produced by a formant** component AND(!) - appended - an F0final or voicing field (which is 0 for unvoiced frames and non-zero for voiced frames). Output: Smoothed formant frequency contours."
#define COMPONENT_NAME_CFORMANTSMOOTHER "cFormantSmoother"

#define FPOSTSMOOTHING_NONE   0
#define FPOSTSMOOTHING_SIMPLE 1
#define FPOSTSMOOTHING_MEDIAN 2

class cFormantSmoother : public cVectorProcessor {
  private:
    int firstFrame;
    int no0f0;
    int medianFilter0;
    int postSmoothing, postSmoothingMethod;
    int saveEnvs;

    long F0fieldIdx, formantFreqFieldIdx, formantBandwidthFieldIdx, formantFrameIntensField;
    long nFormantsIn;

    int nFormants, bandwidths, formants, intensity;
    //int onsFlag, onsFlagO;
    //int octaveCorrection;

    FLOAT_DMEM *median0WorkspaceF0cand;
    FLOAT_DMEM *lastFinal;

    FLOAT_DMEM *fbin;
    FLOAT_DMEM *fbinLastVoiced;
    // NOTE: difficulty when analysing data from multiple pdas : 
    // the scores and voicing probs. may be scaled differently, although they should all be in the range 0-1
    // thus, a direct comparion of these may not be feasible
    // We thus start comparison for each field (pda output) individually and then (TODO) merge information from multiple pdas

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    FLOAT_DMEM *voicingCutoff;

    virtual void myFetchConfig() override;
	  virtual int setupNewNames(long nEl) override;
    
    virtual int processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cFormantSmoother(const char *_name);
    
    virtual ~cFormantSmoother();
};




#endif // __CFORMANTSMOOTHER_HPP
