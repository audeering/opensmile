/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

Input: candidates produced by a pitchBase descendant

*/


#ifndef __CPITCHSMOOTHER_HPP
#define __CPITCHSMOOTHER_HPP

#include <core/smileCommon.hpp>
#include <core/vectorProcessor.hpp>

#define COMPONENT_DESCRIPTION_CPITCHSMOOTHER "This component performs temporal pitch smoothing. Input: candidates produced by a pitchBase descendant (e.g. cPitchSHS). The voicing cutoff threshold is inherited from the input component, thus this smoother component does not provide its own threshold option."
#define COMPONENT_NAME_CPITCHSMOOTHER "cPitchSmoother"

#define POSTSMOOTHING_NONE   0
#define POSTSMOOTHING_SIMPLE 1
#define POSTSMOOTHING_MEDIAN 2

class cPitchSmoother : public cVectorProcessor {
  private:
    int firstFrame;
    int no0f0;
    int medianFilter0;
    int postSmoothing, postSmoothingMethod;
    int onsFlag, onsFlagO;
    int octaveCorrection;

    int F0final, F0finalEnv, voicingFinalClipped, voicingFinalUnclipped;
    int F0raw, voicingC1, voicingClip;

    int nInputLevels; // number of input fields called F0cand (= number of input pdas (algos))
    int voicing, scores; // are candVoicing and candScores fields also present? (1/0 flag)
    
    FLOAT_DMEM lastVoice;
    FLOAT_DMEM pitchEnv;

    // per frame data:
    int *nCands; // array holding number of candidates for each pda (current)
    // global:
    int totalCands;
    int *nCandidates; // array holding max. number of candidates for each pda; array of size nInputLevels;
    FLOAT_DMEM *f0cand, *candVoice, *candScore;

    // index lookup:
    int *f0candI, *candVoiceI, *candScoreI;  // array of size nInputLevels;
    int *F0rawI, *voicingClipI, *voicingC1I;

    FLOAT_DMEM *median0WorkspaceF0cand;
    FLOAT_DMEM *lastFinal;
    FLOAT_DMEM lastFinalF0;

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
    
    cPitchSmoother(const char *_name);
    
    virtual ~cPitchSmoother();
};




#endif // __CPITCHSMOOTHER_HPP
