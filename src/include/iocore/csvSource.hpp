/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

CSV (Comma seperated value) file reader. 
This component reads all columns as attributes into the data memory. One line thereby represents one frame or sample. The first line may contain a header with the feature names (see header=yes/no/auto option).

*/


#ifndef __CCSVSOURCE_HPP
#define __CCSVSOURCE_HPP

#include <core/smileCommon.hpp>
#include <core/dataSource.hpp>

#define COMPONENT_DESCRIPTION_CCSVSOURCE "This component reads CSV (Comma seperated value) files. It reads all columns as attributes into the data memory. One line represents one frame. The first line may contain a header with the feature names (see header=yes/no/auto option)."
#define COMPONENT_NAME_CCSVSOURCE "cCsvSource"

class cCsvSource : public cDataSource {
  private:
    //int nAttr;
    FILE *filehandle;
    const char *filename;

    int *field;
    //int fieldNalloc;
    int nFields, nCols;
    int eof;
    int frameTimeNr_;
    bool readFrameTime_;

    int header;
    char delimChar;

    long lineNr;
    long start;
    long end;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    void setGenericNames(int nDelim);
    void setNamesFromCSVheader(char *line, int nDelim);

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int configureWriter(sDmLevelConfig &c) override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

    virtual int setupNewNames(long nEl=0) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cCsvSource(const char *_name);

    virtual ~cCsvSource();
};




#endif // __CCSVSOURCE_HPP
