/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example for dataProcessor descendant

*/


#ifndef __CDATASELECTOR_HPP
#define __CDATASELECTOR_HPP

#include <core/smileCommon.hpp>
#include <core/dataProcessor.hpp>

#define COMPONENT_DESCRIPTION_CDATASELECTOR "This component copies data from one level to another, thereby selecting frame fields and elements by their element/field name."
#define COMPONENT_NAME_CDATASELECTOR "cDataSelector"

typedef struct {
  long eIdx; // element index

// actually these two are not needed... ??
  long fIdx; // field index
  long aIdx; // array index, if field is an array, else 0
  long N;
} sDataSelectorSelData;

class cDataSelector : public cDataProcessor {
  private:
    int elementMode, selectionIsRange, dummyMode;
    const char *outputSingleField;
    cVector *vecO;

    const char * selFile;
    int nSel; // number of names in names array
    char **names;
    
    int nElSel, nFSel;  // actual number of elements(!) or fields selected (size of mapping array)
    sDataSelectorSelData *mapping; // actual data selection map
    long *idxSelected;

    // for loading files...
    int fselType;

    int loadSelection();

  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    //virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual int setupNewNames(long nEl) override;
    //virtual int configureWriter(const sDmLevelConfig *c) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cDataSelector(const char *_name);

    virtual ~cDataSelector();
};




#endif // __CDATASELECTOR_HPP
