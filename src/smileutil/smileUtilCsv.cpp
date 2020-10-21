/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#include <smileutil/smileUtilCsv.hpp>

#define MODULE "cSmileUtilCsv"

void cSmileUtilCsv::parseHeader(char *line, std::vector<int> &delimPos) {
  int len = (int)strlen(line);
  const char * start = line;
  columns_.clear();
  for (int i = 0; i < delimPos.size(); i++) {
    // if at line separator and not quite or EOL, save previous column value
    int p = delimPos[i];
    line[p] = 0;
    if (start != NULL && start != line + p)
      columns_.push_back(start);
    start = line + p + 1;  // point to next column
  }
}

// parses line with numeric csv values
void cSmileUtilCsv::parseNumericLine(int lineNr, char *line,
    FLOAT_DMEM *buffer, int nColumnsExpected, FLOAT_DMEM defaultValue) {
  int len = (int)strlen(line);
  const char * start = line;
  char *x;
  char *x0 = line;
  int nDelim = 0;
  int nColumns = 0;
  do {
    x = strchr(x0, delimChar_);
    if (x != NULL) {
      *(x++) = 0;
      nDelim++;
    }
    if (nColumns < nColumnsExpected) {
      char *ep = NULL;
      double val = defaultValue;
      while (*x0 == ' ')
        x0++;
      if (*x0 != 0) {
        val = strtod(x0, &ep);
        if ((val==0.0) && (ep==x0)) {
          SMILE_ERR(3, "parsing numeric value in CSV file '%s' (line %i), expected float/int value (element %i).",
              filename_, lineNr_, nColumns);
          val = defaultValue;
        }
      }
      buffer[nColumns] = (FLOAT_DMEM)val;
      nColumns++;
    } else {
      SMILE_ERR(1, "trying to parse more columns than expected from header or first line (#%i >= total %i) on line %i of CSV file '%s'. Ignoring the excess fields!",
          nColumns, nColumnsExpected, lineNr, filename_);
    }
    if (x != NULL) {
      x0 = x;
    }
  } while (x != NULL);
  if (nColumns != nColumnsExpected) {
    SMILE_WRN(2, "cSmileUtilCsv: numer of columns (%i) on line %i of CSV file '%s' does not match the number of expected columns (%i) (read from first line or file header)",
        nDelim + 1, lineNr, filename_, nColumnsExpected);
    for (int i = nColumns; i < nColumnsExpected; i++)
      buffer[i] = defaultValue;
  }
}

// return value == NULL means EOF
// otherwise: return is pointer to parsed vector of floats
// defaultValue will be inserted if empty column or missing column
const FLOAT_DMEM * cSmileUtilCsv::readRow(FLOAT_DMEM defaultValue = 0.0) {
  size_t read;
  size_t n = 0;
  bool l = true;
  FLOAT_DMEM * res = NULL;
  if (filehandle_ == NULL)
    return res;
  do {
    read = smile_getline(&line_, &n, filehandle_);
    //SMILE_IMSG(2, "read line '%s', n %i, read %i", line_, n, read);
    if (((long)read > 0) && (line_ != NULL)) {
      lineNr_++;
      char * line = line_;
      long len = (int)strlen(line);
      // remove new-line characters
      if (len > 0) {
        if (line[len - 1] == '\n') {
          line[len - 1] = 0;
          len--;
        }
      }
      if (len > 0) {
        if (line[len - 1] == '\r') {
          line[len - 1] = 0;
          len--;
        }
      }
      // remove leading whitespaces
      while (((line[0] == ' ') || (line[0] == '\t')) && (len >= 0)) {
        line[0] = 0;
        line++;
        len--;
      }
      if (len > 0) {
        l = false; // <- termination of while loop, when a non-empty line was found
        parseNumericLine(lineNr_, line_, rowCache_, nColumns_, defaultValue);
        res = rowCache_;
      }
    } else {
      l = false; // EOF....
      //SMILE_IMSG(3, "EOF");
    }
  } while (l);
  return res;
}

bool cSmileUtilCsv::openFileForWriting(bool header, bool append) {
  closeFile();
  if (append) {
    filehandle_ = fopen(filename_, "r");
    if (filehandle_ == NULL) {
      filehandle_ = fopen(filename_, "w");
    } else {
      fclose(filehandle_);
      // appending if file exists, disable output of header line
      filehandle_ = fopen(filename_, "a");
      header = false;
    }
  } else {
    filehandle_ = fopen(filename_, "w");
  }
  if (filehandle_ == NULL) {
    SMILE_PRINT("cSmileUtilCsv: ERROR: Failed to open file '%s' for writing/appending.",
        filename_);
    return false;
  }
  lineNr_ = 0;
  if (header) {
    for (int i = 0; i < nColumns_; i++) {
      if (i < columns_.size()) {
        fprintf(filehandle_, "%s", columns_[i].c_str());
      } else {
        fprintf(filehandle_, "_unnamed_column_");
      }
      if (i < nColumns_ - 1)
        fprintf(filehandle_, "%c", delimChar_);
    }
    fprintf(filehandle_, "\n");
    lineNr_++;
    hasHeader_ = true;
  }
  return true;
}

bool cSmileUtilCsv::writeRowStrings(const std::vector<std::string> &elements) {
  if (nColumns_ < elements.size())
    nColumns_ = elements.size();
  if (filehandle_ == NULL)
    return false;
  for (int i = 0; i < elements.size(); i++) {
    fprintf(filehandle_, "%s", elements[i].c_str());
    if (i < elements.size() - 1)
      fprintf(filehandle_, "%c", delimChar_);
  }
  for (int i = elements.size(); i < nColumns_; i++) {
    fprintf(filehandle_, "%c", delimChar_);
  }
  fprintf(filehandle_, "\n");
  lineNr_++;
  return true;
}

bool cSmileUtilCsv::writeRow(const FLOAT_DMEM *vals, int N) {
  if (nColumns_ < N)
    nColumns_ = N;
  if (filehandle_ == NULL || vals == NULL)
    return false;
  for (int i = 0; i < N; i++) {
    fprintf(filehandle_, "%e", vals[i]);
    if (i < N - 1)
      fprintf(filehandle_, "%c", delimChar_);
  }
  for (int i = N; i < nColumns_; i++) {
    fprintf(filehandle_, "%c", delimChar_);
  }
  fprintf(filehandle_, "\n");
  lineNr_++;
  return true;
}

bool cSmileUtilCsv::openFileAndReadHeader(bool headerAuto, bool forceHeader) {
  closeFile();
  filehandle_ = fopen(filename_, "r");
  if (filehandle_ == NULL) {
    SMILE_PRINT("cSmileUtilCsv: ERROR: Failed to open file '%s' for reading.",
        filename_);
    return false;
  }
  lineNr_ = 0;
  // read the header line
  size_t read;
  size_t n;
  bool quote1 = false;
  bool quote2 = false;
  read = smile_getline(&line_, &n, filehandle_);
  if (((long)read > 0) && (line_ != NULL)) {
    lineNr_++;
    // count number of unquoted delim chars  (quoting via ' or ")
    // NOTE: quoting only allowed in header line!
    int len = (int)strlen(line_);
    char * firstEl = NULL;
    std::vector<int> delimPos;
    bool lastElementSaved = false;
    for (int i = 0; i < len; i++) {
      if ((!quote2) && (line_[i] == '\'')) {
        quote1 = !quote1;
      } else if ((!quote1) && (line_[i] == '"')) {
        quote2 = !quote2;
      }
      if ((!quote1 && !quote2) && (line_[i] == delimChar_)) {
        // save first element string
        if (firstEl == NULL) {
          line_[i] = 0;
          firstEl = strdup(line_);
          line_[i] = delimChar_;
        }
        delimPos.push_back(i);
      }
      if (line_[i] == 0 || line_[i] == '\r' || line_[i] == '\n') {
        delimPos.push_back(i);
        lastElementSaved = true;
        break;
      }
    }
    if (!lastElementSaved) {
      if (len > 1)
        delimPos.push_back(len - 1);
    }
    nColumns_ = delimPos.size();

    // set names, either generic or from csv "pseudo" header (first line)
    hasHeader_ = true;
    if (forceHeader) {
      parseHeader(line_, delimPos);
    } else if (headerAuto) {
      // check first Element
      char *eptr = NULL;
      if (firstEl == NULL) {
        SMILE_PRINT("cSmileUtilCsv: WARN: Only one element on first line of CSV file '%s'. Did you configure the right delimiter character!?\n delimChar = '%c'\n line='%s'",
            filename_, delimChar_, line_);
        strtol(line_, &eptr, 10);
        // if the line is not a pure numeric field, assume it is a header
        if ((line_[0] != 0) && ((eptr == NULL) || (eptr[0] == 0))) {
          // pure numeric or empty(?)
          hasHeader_ = false;
        }
      } else {
        strtol(firstEl, &eptr, 10);
        // if the first field is not a pure numeric field, assume it is a header
        if ((firstEl[0] != 0) && ((eptr == NULL) || (eptr[0] == 0))) {
          // pure numeric or empty(?)
          hasHeader_ = false;
        }
      }
      if (hasHeader_) {
        //SMILE_PRINT("cSmileUtilCsv: automatically detected first line of CSV input as HEADER!");
        parseHeader(line_, delimPos);
      } else {
        //SMILE_IMSG(3,"automatically detected first line of CSV input as NUMERIC DATA! (No header is present, generic element names will be used).");
        rewind(filehandle_);
        lineNr_ = 0;
      }
    } else {
      hasHeader_ = false;
      rewind(filehandle_);
      lineNr_ = 0;
    }
    if (firstEl != NULL) {
      free(firstEl);
    } else {
      // this should only be a warning, not an error (only 1 feature in CSV --> no delimiters)
      SMILE_PRINT("cSmileUtilCsv: WARN: no delimiter chars ('%c') found in first line of the CSV file: \n  %s\n    Have you selected the correct delimiter character?",
          delimChar_, line_);
    }
    if (nColumns_ > 0) {
      rowCache_ = (FLOAT_DMEM *)calloc(1, sizeof(FLOAT_DMEM) * nColumns_);
    }
  }
  return true;
}

// read a file and write the file again
bool cSmileUtilCsv::test(const char * input, const char * output,
    const char * output2append) {
  const FLOAT_DMEM *x = NULL;
  SMILE_IMSG(1, "TEST: copy input file '%s' to output file '%s' - auto header",
      input, output)
  cSmileUtilCsv inputCsv(input);
  cSmileUtilCsv outputCsv(output);
  // NOTE: we deliberately do not check the return value of openFile
  // in order to test the robustness of the other functions for
  // an invalid filehandle
  inputCsv.openFileAndReadHeader(true, false);
  outputCsv.setColumnNames(inputCsv.getColumnNames());
  outputCsv.openFileForWriting(inputCsv.hasHeader(), false);
  int N = inputCsv.getNcolumns();
  {
  SMILE_IMSG(1, "input N columns: %i", N);
  const std::vector<std::string> &cols = inputCsv.getColumnNames();
  for (int i = 0; i < cols.size(); i++) {
    SMILE_PRINT("  column[%i] = '%s'", i, cols[i].c_str());
  }
  std::map<std::string, long> &cmap = inputCsv.getColumnNameMap();
  std::string colname = "fun";
  int index = cmap.at(colname);
  SMILE_PRINT("  column '%s' is at index %i (from map)", colname.c_str(), index);
  colname = "coolstuff";
  index = cmap.at(colname);
  SMILE_PRINT("  column '%s' is at index %i (from map)", colname.c_str(), index);
  // exists = cmap.count(colname);
  //colname = "THISISNOTFOUND";
  //index = cmap.at(colname);
  //SMILE_PRINT("  column '%s' is at index %i (from map)", colname.c_str(), index);
  }
  {
  int N2 = outputCsv.getNcolumns();
  SMILE_IMSG(1, "output N columns: %i", N2);
  const std::vector<std::string> &cols = outputCsv.getColumnNames();
  for (int i = 0; i < cols.size(); i++) {
    SMILE_PRINT("  column[%i] = '%s'", i, cols[i].c_str());
  }
  }

  do {
    x = inputCsv.readRow(0.0);
    outputCsv.writeRow(x, N);
    int nIn = inputCsv.getLineNr();
    int nOut = outputCsv.getLineNr();
    if (x != NULL)
      SMILE_PRINT("copying line %i from input to line %i of output", nIn, nOut);
  } while(x != NULL);
  outputCsv.closeFile();
  inputCsv.closeFile();

  SMILE_IMSG(1, "TEST: copy input file '%s' to output2 file '%s' (or append if exists)",
      input, output2append);
  inputCsv.openFileAndReadHeader(input, true, false);
  outputCsv.setColumnNames(inputCsv.getColumnNames());
  outputCsv.setDelimChar(',');
  outputCsv.openFileForWriting(output2append, inputCsv.hasHeader(), true);
  do {
    x = inputCsv.readRow(0.0);
    outputCsv.writeRow(x, N);
    int nIn = inputCsv.getLineNr();
    int nOut = outputCsv.getLineNr();
    if (x != NULL)
      SMILE_PRINT("copying line %i from input to line %i of output2append", nIn, nOut);
  } while(x != NULL);
  outputCsv.closeFile();
  inputCsv.closeFile();

  SMILE_IMSG(1, "TEST: now appending...");
  inputCsv.openFileAndReadHeader(input, true, false);
  outputCsv.setColumnNames(inputCsv.getColumnNames());
  outputCsv.openFileForWriting(output2append, inputCsv.hasHeader(), true);
  do {
    x = inputCsv.readRow(0.0);
    outputCsv.writeRow(x, N);
    int nIn = inputCsv.getLineNr();
    int nOut = outputCsv.getLineNr();
    if (x != NULL)
      SMILE_PRINT("copying line %i from input to line %i of output2append", nIn, nOut);
  } while(x != NULL);
  outputCsv.closeFile();
  inputCsv.closeFile();

  return true;
}
