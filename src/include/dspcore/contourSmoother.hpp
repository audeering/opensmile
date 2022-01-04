/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component: contour smoother

smooth data contours by moving average filter

*/



#ifndef __CCONTOURSMOOTHER_HPP
#define __CCONTOURSMOOTHER_HPP

#include <core/smileCommon.hpp>
#include <core/windowProcessor.hpp>

#define COMPONENT_DESCRIPTION_CCONTOURSMOOTHER "This component smooths data contours by applying a moving average filter of configurable length."
#define COMPONENT_NAME_CCONTOURSMOOTHER "cContourSmoother"

class cContourSmoother : public cWindowProcessor {
  private:
    int smaWin;
    int noZeroSma;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR


    virtual void myFetchConfig() override;

    //virtual int configureWriter(const sDmLevelConfig *c) override;
    //virtual int setupNamesForField(int i, const char*name, long nEl) override;

    // buffer must include all (# order) past samples
    virtual int processBuffer(cMatrix *_in, cMatrix *_out, int _pre, int _post) override;
    
    
  public:
    SMILECOMPONENT_STATIC_DECL
    
    cContourSmoother(const char *_name);

    virtual ~cContourSmoother();
};


#endif // __CCONTOURSMOOTHER_HPP
