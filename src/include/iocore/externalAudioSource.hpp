/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

This component reads audio input that is passed to the component programmatically.

*/


#ifndef __CEXTERNALAUDIOSOURCE_HPP
#define __CEXTERNALAUDIOSOURCE_HPP

#include <core/smileCommon.hpp>
#include <core/smileThread.hpp>
#include <core/dataSource.hpp>

#define COMPONENT_DESCRIPTION_CEXTERNALAUDIOSOURCE "This component reads audio input that is passed to the component programmatically."
#define COMPONENT_NAME_CEXTERNALAUDIOSOURCE "cExternalAudioSource"

class cExternalAudioSource : public cDataSource {
  private:
    int sampleRate_;
    int channels_;
    int nBits_;
    int nBPS_;
    const char *fieldName_;
    sWaveParameters pcmParam_;

    cMatrix *writtenDataBuffer_;       // buffer holding written data converted to float format
    mutable smileMutex writeDataMtx_;  // mutex for ensuring writeData is thread-safe

    bool externalEOI_;
    bool eoiProcessed_;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int setupNewNames(long nEl) override;
    virtual eTickResult myTick(long long t) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cExternalAudioSource(const char *name);

    // Checks if a write of Nbytes bytes will succeed, i.e. if there is enough space
    // in the data memory level.  Usually this is not needed before calls to
    // writeData, as writeData itself calls checkWrite internally.
    bool checkWrite(int nBytes) const;

    // Writes data to the dataMemory level. The passed buffer must contain audio
    // in the PCM format specified in the component config.
    bool writeData(const void *data, int nBytes);

    // Signals that this component will not receive any more external data. Attempts to
    // call writeData will fail afterwards.
    void setExternalEOI();

    // external code can use these public accessors to retrieve the format configuration
    // that was set in the configuration file
    int getSampleRate() const { return sampleRate_; }
    int getChannels() const { return channels_; }
    int getNBits() const { return nBits_; }
    int getNBPS() const { return nBPS_; }
    
    virtual ~cExternalAudioSource();
};


#endif // __CEXTERNALAUDIOSOURCE_HPP
