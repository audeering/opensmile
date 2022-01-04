/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#ifndef __CSMILEUTILCSV_HPP
#define __CSMILEUTILCSV_HPP

#include <smileutil/smileUtilBase.hpp>
#include <string>
#include <vector>
#include <map>


// TODO: file rewind function,
// which positions pointer to first line or line after
// the header

// TODO: start/end line filter (i.e. readRow auto read upto start line)
//  or make readRangeRow wrapper for this...

// TODO: column filter by range and by name

// TODO: support mixed numeric (Float) and string types for rows

class cSmileUtilCsv : public cSmileUtilBase {
private:
  const char * filename_;
  FILE *filehandle_;
  long nColumns_;
  std::vector<std::string> columns_;
  std::map<std::string, long> colMap_;
  FLOAT_DMEM * rowCache_;
  char * line_;
  long lineNr_;
  char delimChar_;
  bool hasHeader_;

  void parseHeader(char *line, std::vector<int> &delimPos);
  void parseNumericLine(int lineNr, char *line,
      FLOAT_DMEM *buffer, int nColumnsExpected, FLOAT_DMEM defaultValue);
  void createColumnNameMap() {
    if (colMap_.size() != columns_.size()) {
      colMap_.clear();
      for (unsigned int i = 0; i < columns_.size(); i++) {
        colMap_[columns_[i]] = i;
      }
    }
  }

public:
  cSmileUtilCsv() : filename_(NULL), nColumns_(0), filehandle_(NULL),
      lineNr_(0), line_(NULL), rowCache_(NULL), delimChar_(';'),
      hasHeader_(false) {
    setInstName("cSmileUtilCsv");
  }
  cSmileUtilCsv(const char * filename) : filename_(filename),
      nColumns_(0), filehandle_(NULL), delimChar_(';'),
      lineNr_(0), line_(NULL), rowCache_(NULL),
      hasHeader_(false) {
    setInstName("cSmileUtilCsv");
  }
  bool hasHeader() {
    return hasHeader_;
  }
  void setDelimChar(char delim) {
    delimChar_ = delim;
  }
  char getDelimChar() {
    return delimChar_;
  }
  FILE * getFileHandle() {
    return filehandle_;
  }
  bool openFileAndReadHeader(bool headerAuto, bool forceHeader);
  bool openFileAndReadHeader(const char * filename,
      bool headerAuto, bool forceHeader) {
    if (filename != NULL)
      filename_ = filename;
    return openFileAndReadHeader(headerAuto, forceHeader);
  }
  long getNcolumns() {
    return nColumns_;
  }
  void setNcolumns(long n) {
    nColumns_ = n;
  }
  void setColumnNames(const std::vector<std::string> &names) {
    clearColumnNames();
    columns_ = names;
    nColumns_ = names.size();
  }
  void clearColumnNames() {
    columns_.clear();
    nColumns_ = 0;
  }
  void setColumnName(int i, const char *name) {
    columns_[i] = name;
    nColumns_ = columns_.size();
  }
  const std::vector<std::string> &getColumnNames() {
    return columns_;
  }
  std::map<std::string, long> &getColumnNameMap() {
    createColumnNameMap();
    return colMap_;
  }
  long getLineNr() {
    return lineNr_;
  }
  bool openFileForWriting(bool header, bool append);
  bool openFileForWriting(const char *filename, bool header,
      bool append) {
    filename_ = filename;
    return openFileForWriting(header, append);
  }

  bool writeRowStrings(const std::vector<std::string> &elements);
  bool writeRow(const FLOAT_DMEM *vals, int N);

  const FLOAT_DMEM * readRow(FLOAT_DMEM defaultValue);
  void closeFile() {
    if (filehandle_ != NULL) {
      fclose(filehandle_);
      filehandle_ = NULL;
    }
    if (rowCache_ != NULL) {
      free(rowCache_);
      rowCache_ = NULL;
    }
    if (line_ != NULL) {
      free(line_);
      line_ = NULL;
    }
    lineNr_ = 0;
    hasHeader_ = false;
  }
  bool test(const char * input, const char * output,
      const char * output2append);
  ~cSmileUtilCsv() {
    closeFile();
  }
};

#endif  // __CSMILEUTILCSV_HPP

