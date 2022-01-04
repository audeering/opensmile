/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

This component reads data that is passed to the component programmatically.

*/


#ifndef __CEXTERNALSOURCE_HPP
#define __CEXTERNALSOURCE_HPP

#include <core/smileCommon.hpp>
#include <core/smileThread.hpp>
#include <core/dataSource.hpp>

#define COMPONENT_DESCRIPTION_CEXTERNALSOURCE "This component reads data that is passed to the component programmatically."
#define COMPONENT_NAME_CEXTERNALSOURCE "cExternalSource"

class cExternalSource : public cDataSource {
  private:
    int vectorSize_;

    cMatrix *writtenDataBuffer_;       // temporary buffer holding written data
    mutable smileMutex writeDataMtx_;  // mutex for ensuring writeData is thread-safe

    bool externalEOI_;
    bool eoiProcessed_;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNewNames(long nEl) override;
    virtual eTickResult myTick(long long t) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cExternalSource(const char *name);

    // Checks if a write of nFrames will succeed, i.e. if there is enough space
    // in the data memory level. Usually this is not needed before calls to
    // writeData, as writeData itself calls checkWrite internally.
    bool checkWrite(int nFrames) const;

    // Writes data to the dataMemory level. The passed buffer must contain data
    // in the same memory format as cMatrix.
    bool writeData(const FLOAT_DMEM *data, int nFrames);

    // Signals that this component will not receive any more external data. Attempts to
    // call writeData will fail afterwards.
    void setExternalEOI();

    int getVectorSize() const { return vectorSize_; }

    virtual ~cExternalSource();
};


#endif // __CEXTERNALSOURCE_HPP
