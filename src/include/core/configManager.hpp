/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


//***** TODO(?):  implement isMandatory option for configType fields! (and check for those fields)//

#ifndef __CONFIG_MANAGER_HPP
#define __CONFIG_MANAGER_HPP


#define CONFIGTYPE_STRLEN 255
#define NEL_ALLOC_BLOCK   10

#include <core/smileCommon.hpp>
#include <core/commandlineParser.hpp>
#define RAPIDJSON_HAS_STDSTRING 1 
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <unordered_set>

int instNameSplit(const char *n, char **b, const char **s);


#define CFTP_NUM   0
#define CFTP_STR   1
#define CFTP_CHR   2
#define CFTP_OBJ   3
#define CFTP_ARR   99
#define CFTP_NUM_ARR   100
#define CFTP_STR_ARR   101
#define CFTP_CHR_ARR   102
#define CFTP_OBJ_ARR   103

class ConfigInstance;

class ConfigValue {
  protected:
    int set;
    int type;

  public:
    virtual void setValue(int v, int n=0) {}
    virtual void setValue(double v, int n=0) {}
    virtual void setValue(const char *v, int n=0) {}
    virtual void setValue(char v, int n=0) {}
    virtual void setValue(ConfigInstance *v, int n=0) {}
    virtual void setValue(ConfigValue *v, int n=0) {}

    virtual int getInt(int n=0) const { return 0; }
    virtual double getDouble(int n=0) const { return 0.0; }
    virtual const char* getStr(int n=0) const { return NULL; }
    virtual char getChar(int n=0) const { return 0; }
    virtual ConfigInstance* getObj(int n=0) const { return NULL; }
    virtual ConfigValue* getValue(int n=0) const { return NULL; }

    virtual void copyFrom(const ConfigValue *val) {};
    virtual int missingFrom(const ConfigValue *val) {
      if (!isSet()) { copyFrom(val); return 1; }
      else { return 0; }
    }
    virtual int updateWith(const ConfigValue *val) {
      if ((val!=NULL)&&(val->isSet())) { copyFrom(val); return 1; }  
      else { return 0; }
    }

    virtual int findField(const char *_name=NULL, int noerr=0) const { return -1; }
    virtual int findFieldWr(const char *_name=NULL) { return -1; }

    virtual int getType() const { return type; }
    virtual int isSet(int n=-1) const { return set; }
    virtual int getN() const { return -1; }
    virtual int getSize() const { return -1; }
    virtual char ** getAAkeys() const { return NULL; }  // get keys of associative array fields
    int unset() { set = 0; return 1; }
    
    virtual ~ConfigValue() {}
};


class ConfigValueNum : public ConfigValue {
  private:
    int valueI;
    double valueD;
  public:
    ConfigValueNum() : valueI(0), valueD(0) { type = CFTP_NUM; set = 0; }
    ConfigValueNum(int value) : valueI(value), valueD((double)value) { type = CFTP_NUM; set = 1; }
    ConfigValueNum(double value) : valueI((int)value), valueD(value) { type = CFTP_NUM; set = 1; }

    virtual int getInt(int n=0) const override { return valueI; }
    virtual double getDouble(int n=0) const override { return valueD; }

    virtual void setValue(int value, int n=0) override { valueI = value; valueD = (double)value; set=1; }
    virtual void setValue(double value, int n=0) override { valueD = value; valueI = (int)value; set=1; }

    virtual void copyFrom(const ConfigValue *val) override;
    
    ~ConfigValueNum() {}
};

class ConfigValueStr : public ConfigValue {
  private:
    char *str;  // string data
    size_t N;      // string length
  public:
    ConfigValueStr() : str(NULL), N(0) { type = CFTP_STR; set = 0; }
    ConfigValueStr(const char * value) : str(NULL), N(0) {
      type = CFTP_STR;
      setValue(value);
    }
    virtual const char * getStr(int n=0) const override { return str; }
    virtual char getChar(int n=0) const override { if (str!=NULL) return str[0]; else return 0; }

    virtual void setValue(const char * value, int n=0) override {
      if (str != NULL) { free(str); str=NULL; }
      if (value==NULL) { set = 0; return; }
      str = strdup(value); N=strlen(value);
      set=1;
    }
    virtual void copyFrom(const ConfigValue *val) override;

    ~ConfigValueStr() { if (str != NULL) free(str); }
};

class ConfigValueChr : public ConfigValue {
  private:
    char c;

  public:
    char s[2];
    
    ConfigValueChr() : c(0) { type = CFTP_CHR; set = 0; }
    ConfigValueChr(const char _c) : c(_c) { s[0]=_c; s[1]=0; type = CFTP_CHR; set = 1; }
    virtual char getChar(int n=0) const override { return c; }
    virtual const char * getStr(int n=0) const override { return s; }
    virtual void setValue(char _c, int n=0) override { c=_c; set=1; } // XXX mod : removed const
    virtual void copyFrom(const ConfigValue *val) override;
    ~ConfigValueChr() {}
};


class ConfigValueObj : public ConfigValue {
  private:
    ConfigInstance *obj;
    int freeObj;

  public:
    ConfigValueObj() : obj(NULL), freeObj(0) { type = CFTP_OBJ; set = 0; }
    ConfigValueObj(ConfigInstance *_obj, int _freeObj=1) :
      obj(_obj), freeObj(_freeObj) { type = CFTP_OBJ; set = 1; }

    virtual ConfigInstance * getObj(int n=0) const override { return obj; }
    virtual void setValue(ConfigInstance * value, int n=0) override;
    void setFreeObj(int _f=1) { freeObj = _f; }
    virtual void copyFrom(const ConfigValue *val) override;
    virtual int missingFrom(const ConfigValue *val) override;
    virtual int updateWith(const ConfigValue *val) override;

    ~ConfigValueObj();
};

// TODO: associative array functionality by over loading operators (int n  AND const char *name)
class ConfigValueArr : public ConfigValue {
  protected:
    ConfigValue **el;
    char **aName;  // names for associative array
    int N, maxN;  // N is total number of elements allocated, maxN is the maximum index that was set

    int checkIdx(int n) const;
    int checkIdxWr(int n);
    
  public:
    ConfigValueArr(int initN=NEL_ALLOC_BLOCK);
    // : N(0), el(NULL)

    virtual ~ConfigValueArr();

    virtual ConfigValue * operator[](int n) const;
    virtual ConfigValue * operator[](const char *_name) const;

    virtual int findField(const char *_name, int noerr=0) const override;
    virtual int findFieldWr(const char *_name) override;

    virtual void setValue(ConfigValue *v, int n) override;
    virtual void copyFrom(const ConfigValue *val) override;
//    virtual void copyFrom(const ConfigValue *val, int arrIdx) override;  // copy single element ??
    virtual int missingFrom(const ConfigValue *val) override;
    virtual int updateWith(const ConfigValue *val) override;
    virtual int getN() const override { return N; }
    virtual int getSize() const override { return maxN+1; }
    virtual char ** getAAkeys() const override { return aName; }  // get keys of associative array fields

    virtual int isSet(int n=-1) const override {
      if (n<0) return set;
      else { if ((n<=maxN)&&(el[n]!=NULL)&&(el[n]->isSet())) return 1; else return 0; }
    }
};

class ConfigValueNumArr : public ConfigValueArr {
  public:
    ConfigValueNumArr(int initN=NEL_ALLOC_BLOCK) : ConfigValueArr(initN) { type=CFTP_NUM_ARR; }

    virtual int getInt(int n=0) const override {
      if (checkIdx(n)&&(el[n] != NULL)) return ((ConfigValueNum *)(el[n]))->getInt();  // get value via () operator
      else return 0;
    }

    virtual double getDouble(int n=0) const override {
      if (checkIdx(n)&&(el[n] != NULL)) return ((ConfigValueNum *)(el[n]))->getDouble();  // get value via () operator
      else return 0.0;
    }

    virtual void setValue(double v, int n=0) override;
    virtual void setValue(int v, int n=0) override;

    std::vector<int> asIntVector() const {
      std::vector<int> vec;
      vec.resize(getSize());
      for (int n = 0; n < getSize(); n++)
        vec[n] = getInt(n);
      return vec;
    }

    std::vector<double> asDoubleVector() const {
      std::vector<double> vec;
      vec.resize(getSize());
      for (int n = 0; n < getSize(); n++)
        vec[n] = getDouble(n);
      return vec;
    }
};

class ConfigValueStrArr : public ConfigValueArr {
  public:
    ConfigValueStrArr(int initN=NEL_ALLOC_BLOCK) : ConfigValueArr(initN) { type=CFTP_STR_ARR; }

    virtual const char* getStr(int n=0) const override {
      if (checkIdx(n)&&(el[n] != NULL))
        return ((ConfigValueStr *)(el[n]))->getStr();  // get value via () operator
      else return NULL;
    }
    virtual void setValue(const char *v, int n=0) override;

    std::vector<std::string> asVector() const {
      std::vector<std::string> vec;
      vec.resize(getSize());
      for (int n = 0; n < getSize(); n++)
        vec[n] = getStr(n);
      return vec;
    }
};

class ConfigValueChrArr : public ConfigValueArr {
  public:
    ConfigValueChrArr(int initN=NEL_ALLOC_BLOCK) : ConfigValueArr(initN) { type=CFTP_CHR_ARR; }

    virtual char getChar(int n=0) const override {
      if (checkIdx(n)&&(el[n] != NULL))
        return ((ConfigValueChr *)(el[n]))->getChar();  // get value via () operator
      else return 0;
    }
    virtual void setValue(char v, int n=0) override;

    std::vector<char> asVector() const {
      std::vector<char> vec;
      vec.resize(getSize());
      for (int n = 0; n < getSize(); n++)
        vec[n] = getChar(n);
      return vec;
    }
};

class ConfigValueObjArr : public ConfigValueArr {
  public:
    ConfigValueObjArr(int initN=NEL_ALLOC_BLOCK) : ConfigValueArr(initN) { type=CFTP_OBJ_ARR; }

    virtual ConfigInstance* getObj(int n=0) const override {
      if (checkIdx(n)&&(el[n] != NULL))
        return ((ConfigValueObj *)(el[n]))->getObj();  // get value via () operator
      else return NULL;
    }
    virtual void setValue(ConfigInstance *v, int n=0) override;
};


/*********** Config Type **********/
class ConfigType;
#define NO_ARRAY        0
#define ARRAY_TYPE      1
#define DONT_FREE       0

typedef struct sConfigDescription {
  int type;                          /* CFTP_XXX constants */
  int subtype;                       /* reserved */
  int freeType;
  const ConfigType * subType;              /* if type==CFTP_OBJ or CFTP_OBJARR -> subType */
  int fieldLen;                      /* field length (e.g. for strings), -1 for dynamic (pointer), 0 for use sizeof(type) */
  int N;                             /* number of elements for array types: 0 = no array, 1 = variable length array */
  int enabled;                       /* if set to 0, the field will not be shown by printTypeHelp, it will be, however, still available! */
  int printDflt;                     /* if set to 1, this field will be printed with the -configDflt option */
  char name[CONFIGTYPE_STRLEN+1];    /* name of field (UNIQUE per type) */
  char *description;                 /* description of field in human readable form */
  // default values... will be read by configInstance upon creation
  // for arrays: all field will be initialised with the same value, CFTP_OBJ has no default value
  char dfltChar;
  double dfltDouble;
  char *dfltStr;
  int isMandatory;       // 0=optional parameter, 1=mandatory parameter (exception will be generated, if parameter is not found in config file!)
  int inherited;         // if set to 1, this field is inherited from a parent type
  int overridden;        // if set to 1, this field is overridden
} ConfigDescription;

class ConfigType {
  private:
    char                       name[CONFIGTYPE_STRLEN+1];  /* name of type */
    char                       parentName[CONFIGTYPE_STRLEN+1];  /* name of parent type, has length 0 if there is no parent type */

    int                        N,I;  // I points to current Field that is to be set by setField()
    ConfigDescription  *       element;

  public:
    const ConfigDescription * operator[] (int n);

    ConfigType( const char *_name, int N_=10);
    ConfigType( ConfigType const& copy, const char *_newname=NULL);
    
    int setField(const char *_name, const char *description, int type, int subtype, const ConfigType *subType, int freeType=1, int N_=0, int printDflt=1);

    int setField(const char *_name, const char *description, int dflt, int arr=0, int printDflt=1);
    int setField(const char *_name, const char *description, double dflt, int arr=0, int printDflt=1);
    int setField(const char *_name, const char *description, const char * dflt, int arr=0, int printDflt=1);
    int setField(const char *_name, const char *description, char dflt, int arr=0, int printDflt=1);
    int setField(const char *_name, const char *description, const ConfigType *dflt, int arr=0, int freeType=1);

    // disable field by setting "enabled = 0", the field is still available normally, however, it will not be shown by printTypeHelp anymore!
    int disableField(const char *_name);
    int makeMandatory(int _id);
    
    double getDfltNum(int n) const { if ((n<I)&&(n>=0)) return element[n].dfltDouble; else return 0.0; }
    char * getDfltStr(int n) const { if ((n<I)&&(n>=0)) return element[n].dfltStr; else return NULL; }
    char getDfltChr(int n) const { if ((n<I)&&(n>=0)) return element[n].dfltChar; else return 0; }
    const ConfigType * getDfltObj(int n) const { if ((n<I)&&(n>=0)) return element[n].subType;  else return NULL; }
    
    // get index of field _name, and optionally an array index specified in _name
    int findField(const char *_name, int *arrI=NULL, char **arrS=NULL) const;
    int findFieldH(const char *_name, int *n, const ConfigType **tp, int *aIdx, char **aStr) const;

    // set a new name:
    void setName(const char *_name );
    // get name of ConfigType, if no argument is given
    const char * getName() const { return name; }
    // set or change name of this type (use this for derived types created via copy constructor...)
    void setName();
    // get name of parent ConfigType
    const char *getParentName() const { return parentName; }
    // set a new parent name
    void setParentName(const char *_parentName);

    // get name of element, if an index is given as argument
    const char * getName(int n) const;

    int getType(int n) const;
    const ConfigType * getTypeObj(int n) const; // for subTypes..
    int getN() const { return I; }

    // print description of fields in this type (and all sub-types, if subtype=1)
    void printTypeHelp(char *basestr=NULL, int _subtype=1) const;  

    // print default config section for fields of this type (and all sub-types, if subtype=1)
    // if withDescription == 1, then also print description of each value as comment
    void printTypeDfltConfig(char *basestr=NULL, int _subtype=1, int withDescription=0, int ignInternal=2) const; 

    rapidjson::Value exportTypeHelp(rapidjson::MemoryPoolAllocator<> &allocator) const;
    void enumSubTypes(std::unordered_set<const ConfigType *> &subTypes) const;

    virtual ~ConfigType();
};


/******* Config Instance *********/
class ConfigInstance {
  private:
    char                       name[CONFIGTYPE_STRLEN];
    const ConfigType *type;          /* single type class */
    int freeType;              /* flag that indicates whether to free type or not */
    int N;                     /* same as in type, number of values */
    ConfigValue **field;       /* array of field values */

    int findField(const char *_name, int *n, ConfigInstance **in, int *aIdx, char **aStr);
    void setVal(int n, ConfigValue *val, int idx=-1);
    const ConfigValue* getVal(int n, int idx=-1) const;

    int sanityCheck(ConfigInstance *_match) const;

  public:
    ConfigInstance(const char *_name, const ConfigType *_type, int _freeType=0);
    int missingFrom(ConfigInstance *_default); /* update missing values, take values from _default */
    int updateWith(ConfigInstance *_new); /* values in _new overwrite values in current instance */

    const int getN() const { return N; }
    const char * getName() const { return name; }  // get name of instance
    const char * getName(int n) const { return type->getName(n); } // get name of field n

    const ConfigType * getType() const { return type; } // get type object
    const char * getTypeName() const { return type->getName(); } // get type object
    int getType(int n) const { return type->getType(n); } // get type of field n
    int getType(const char *_name); /* recursively dereference, via findField */
    //const ConfigType * getTypeObj(const char *_name) const; // get type object  xxx
    
    /* either N or name may be specified. if not specified name=NULL and N=-1 */
    void setValue(ConfigValue *val, int n=-1, const char *_name=NULL, int arrIdx=-1);   /* recursively dereference subobj pointers */
    void setValue(const char *_name, ConfigValue *val) {
      setValue(val,-1,_name);
    }
    const ConfigValue *getValue(int n=-1, const char *_name=NULL, int arrIdx=-1) const; /* recursively dereference subobj pointers */
    const ConfigValue *getValue(const char *_name, int arrIdx=-1) const {
      return getValue(-1,_name,arrIdx);
    }
    ConfigInstance *getSubInstance(const char *_name);

	// note: added "const" here...?
    int getInt(const char *_name) { const ConfigValue *r = getValue(-1,_name); if (r!=NULL) return r->getInt(); else return 0; }
    double getDouble(const char *_name) { const ConfigValue *r = getValue(-1,_name); if (r!=NULL) return r->getDouble(); else return 0; }
    const char* getStr(const char *_name) { const ConfigValue *r = getValue(-1,_name); if (r!=NULL) return r->getStr(); else return 0; }
    char getChar(const char *_name) { const ConfigValue *r = getValue(-1,_name); if (r!=NULL) return r->getChar(); else return 0; }
    ConfigInstance * getObj(const char *_name) { const ConfigValue *r = getValue(-1,_name); if (r!=NULL) return r->getObj(); else return 0; }

	// when setting hierarchical config names such as "reader.dmLevel" via setStr(), etc. the "reader" field of type CFTP_OBJ should automatically be created by setStr()... if not, please report this bug!
    void setInt(const char *_name, int value) { ConfigValue *r = new ConfigValueNum(value); if (r!=NULL) setValue(_name,r); }
    void setDouble(const char *_name, double value) { ConfigValue *r = new ConfigValueNum(value); if (r!=NULL) setValue(_name,r); }
    void setStr(const char *_name, const char * value) { ConfigValue *r = new ConfigValueStr(value); if (r!=NULL) setValue(_name,r); }
    void setChar(const char *_name, char value) { ConfigValue *r = new ConfigValueChr(value); if (r!=NULL) setValue(_name,r); }
    void setObj(const char *_name, ConfigInstance *value) { ConfigValue *r = new ConfigValueObj(value); if (r!=NULL) setValue(_name,r); }

    ~ConfigInstance();
};


/******* Config Reader *******/
class cConfigManager;

class cConfigReader {
  protected:
    char *inputPath;      // top level config file
    char *lastLevelFile;  // config file that was previously included (can be pointer identical to inputPath or NULL in this case)
    int  inputId;
    cCommandlineParser *cmdparser;
    
  public:
    cConfigReader(const char *_inputPath, int _inputId=-1, cCommandlineParser *cmdparser_=NULL);
    void setCmdparser(cCommandlineParser *_parser) { cmdparser = _parser; }
    virtual void setId(int _id) { if (_id>=0) inputId = _id; }
    virtual int openInput(const char*name, int *idx0=NULL) { return 0; }
    virtual int closeInput() { return 0; }
    virtual char ** findInstancesByTypeName(const char *_typename, int *N)=0;  /* returns names (and no.) of instances of type "typename" */
    char ** findInstances(const ConfigType *_type, int *N);  /* returns names (and no.) of instances of type "typename" */
    void destroyStrArray(char **arr, int n);
    virtual ConfigInstance *getInstance(const char *_instname, const ConfigType *_type, cConfigManager *cman=NULL)=0;    /* get an instance with name _instname */
    virtual ~cConfigReader();
};



struct fileInstance {
  char *name;
  char *type;
  char **lines;
  int *lineNr;  // absolute line numbers (as in file)
  int N;  // number of lines
  int Nalloc;
};

class cConfigFileBuffer {
private:
  int size_;
  int sizeAllocated_;
  char *buffer_;
public:
  cConfigFileBuffer() : sizeAllocated_(0), buffer_(NULL), size_(0){}

  cConfigFileBuffer(int sizeAllocated) : size_(0),
      sizeAllocated_(sizeAllocated){
    buffer_ = (char *)calloc(1, sizeof(char) * sizeAllocated_);
  }

  char *getPointer() {
    return buffer_;
  }

  int getSize() {
    return size_;
  }

  char * resize(int newSize) {
    if (newSize > 0) {
      buffer_ = (char *)crealloc(buffer_,
          sizeof(char) * newSize, sizeAllocated_);
      sizeAllocated_ = newSize;
    }
    return buffer_;
  }

  void saveFile(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (f != NULL) {
      long nWritten = fwrite(buffer_, 1, size_, f);
      if (nWritten != size_)
        SMILE_PRINT("cConfigFileBuffer: Error: less bytes written than expected! (%i < %i)",
            nWritten, size_);
      fclose(f);
    } else {
      SMILE_PRINT("cConfigFileBuffer: Error: failed writing file '%s'", filename);
    }
  }

  int loadFile(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (f != NULL) {
      loadFile(filename, f);
      fclose(f);
      return 1;
    } else {
      //SMILE_PRINT("cConfigFileBuffer: Error: failed loading file '%s'", filename);
      return 0;
    }
  }

  void loadFile(const char *filename, FILE *f);

  cConfigFileBuffer(const cConfigFileBuffer &c) {
    size_ = c.size_;
    sizeAllocated_ = c.sizeAllocated_;
    buffer_ = (char *)calloc(1, sizeof(char) * sizeAllocated_);
    memcpy(buffer_, c.buffer_, size_);
  }

  cConfigFileBuffer& operator = (const cConfigFileBuffer &c) {
    size_ = c.size_;
    sizeAllocated_ = c.sizeAllocated_;
    if (buffer_ != NULL)
      free(buffer_);
    buffer_ = (char *)calloc(1, sizeof(char) * sizeAllocated_);
    memcpy(buffer_, c.buffer_, size_);
    return *this;
  }

  ~cConfigFileBuffer() {
    if (buffer_ != NULL)
      free(buffer_);
  }
};

#include <vector>
#include <map>

class cFileConfigReader : public cConfigReader {
  private:
    fileInstance *inst_;
    int nInst_, nInstAlloc_;
    // maps a config filename to a buffer vector index
    int nCached_;
    std::map<std::string, cConfigFileBuffer> cacheMap_;

    char *loadFileIntoCache(const char *fname,
        /*FILE *f,*/ int *bufSize);

    void setNlines(int n, int nlines);
    int addInst(const char*_instname, const char*_typename);
    int addLine(int n, const char *line, int lineNr);

    // check if config file is in cache
    char * getConfigFileBufferFromCache(const char *fname,
        int *fbufferSize) {
      std::string s(fname);
      if (cacheMap_.find(s) != cacheMap_.end()) {
        cConfigFileBuffer b = cacheMap_[s];
        if (fbufferSize != NULL)
          *fbufferSize = b.getSize();
        return cacheMap_[s].getPointer();
      }
      return NULL;
    }

    char* addAudExtensionIfNeeded(const char* fname);

  public:
    cFileConfigReader(const char *filename, int id=-1,
        cCommandlineParser *cmdparser_=NULL) :
          cConfigReader(filename,id,cmdparser_), inst_(NULL),
          nInst_(0), nInstAlloc_(0), nCached_(0) {
      openInput();
    }

    virtual int openInput(const char*name=NULL, int *idx0=NULL) override;
    virtual int closeInput() override { return 0; }

    virtual char ** findInstancesByTypeName(const char *_typename, int *N) override;  /* returns names (and no.) of instances of type "typename" */
    virtual ConfigInstance *getInstance(const char *_instname,
        const ConfigType *_type, cConfigManager *cman=NULL) override;    /* get an instance with name _instname */
    virtual ~cFileConfigReader();
};


/**** Config Manager ****/
#ifndef MODULE
#define MODULE "configManager"

class cConfigManager {
  private:
    int nTypes, nTypesAlloc;
    ConfigInstance **defaults;
    int nInst, nInstAlloc;
    ConfigInstance **inst;
    int nReaders, nReadersAlloc;
    cConfigReader   **reader;
    cCommandlineParser *cmdparser;
    
  protected:
    //int findInstance(const char *_instname);

  public:
    cConfigManager(cCommandlineParser *_parser = NULL);
    int addReader(cConfigReader *_reader);
    int registerType(ConfigInstance *_type);
    void readConfig();                     /* read the config, after readers and types have been registered */
    int addInstance(ConfigInstance *_inst);   /* stores inst object in configManager, inst object will be freed by configManager */
    int deleteInstance(const char *_instname);   /* deletes instance "_instname" */
	  int updateInstance(ConfigInstance *_inst);  /* only uses content from inst to update existing object. object is only added if it does not yet exist  (return value 1 indicated an update, while 0 indicates an adding)*/
    int findInstance(const char *_instname) const; // first level only
    int findType(const char *_typename) const;  // first level only
    const ConfigType *getTypeObj(int n) const;
    const ConfigType *getTypeObj(const char *_typename) const;  // hierarchical type resolving...

    
    /* get an instance object or value... resolve hierachical names */
    ConfigInstance * getInstance(const char *_instname);
    /* the _f getXXX functions free the memory allocated for _name before they return normally! */
    ConfigInstance * getInstance_f(char *_name) {  /* return value is read only! */
      ConfigInstance *inst = getInstance(_name);
      if (_name != NULL) free(_name);
      return inst;
    }

    const ConfigValue    * getValue(const char *_name) const;  /* return value is read only! */
    /* the _f getXXX functions free the memory allocated for _name before they return normally! */
    const ConfigValue    * getValue_f(char *_name) const {  /* return value is read only! */
      const ConfigValue *v = getValue(_name);
      if (_name != NULL) free(_name);
      return v;
    }

    int getArraySize(const char *_name) const;
    /* the _f getXXX functions free the memory allocated for _name before they return normally! */
    int getArraySize_f(char *_name) const {  /* return value is read only! */
      int s = getArraySize(_name);
      if (_name != NULL) free(_name);
      return s;
    }

    
    /* get keys of an associative array, and optionally the number N of elements
       NOTE: some keys in the returned array might be NULL if the element was not set!! */
    char **getArrayKeys(const char *_name, int *N=NULL) const;
    /* the _f getXXX functions free the memory allocated for _name before they return normally! */
    char **getArrayKeys_f(char *_name, int *N=NULL) const {  /* return value is read only! */
      char ** k = getArrayKeys(_name, N);
      if (_name != NULL) free(_name);
      return k;
    }

    
    const int getInt(const char *_name, int *isSet=NULL) const {
      const ConfigValue *v = getValue(_name);
      if (v!=NULL) {
        if (v->getType() >= CFTP_ARR) CONF_MANAGER_ERR("getInt('%s') called on array element without array index!",_name);
        if (isSet != NULL) *isSet = v->isSet();
        return v->getInt();
      }
      return 0;
    }

    /* the _f getXXX functions free the memory allocated for _name before they return normally! */
    const int getInt_f(char *_name, int *isSet=NULL) const {  /* return value is read only! */
      int d = getInt(_name, isSet);
      if (_name != NULL) free(_name);
      return d;
    }

    const double getDouble(const char *_name, int *isSet=NULL) const {
      const ConfigValue *v = getValue(_name);
      if (v!=NULL) {
        if (v->getType() >= CFTP_ARR) CONF_MANAGER_ERR("getDouble('%s') called on array element without array index!",_name);
        if (isSet != NULL) *isSet = v->isSet();
        return v->getDouble();
      }
      return 0.0;
    }

    /* the _f getXXX functions free the memory allocated for _name before they return normally! */
    const double getDouble_f(char *_name, int *isSet=NULL) const {  /* return value is read only! */
      double d = getDouble(_name, isSet);
      if (_name != NULL) free(_name);
      return d;
    }

    const char    * getStr(const char *_name, int *isSet=NULL) const {  /* return value is read only! */
      const ConfigValue *v = getValue(_name);
      if (v!=NULL) {
        if (v->getType() >= CFTP_ARR) CONF_MANAGER_ERR("getStr('%s') called on array element without array index!",_name);
        if (isSet != NULL) *isSet = v->isSet();
        return v->getStr();
      }
      return NULL;
    }

    /* the _f getXXX functions free the memory allocated for _name before they return normally! */
    const char    * getStr_f(char *_name, int *isSet=NULL) const {  /* return value is read only! */
      const char *s = getStr(_name, isSet);
      if (_name != NULL) free(_name);
      return s;
    }
    
    const char getChar(const char *_name, int *isSet=NULL) const {
      const ConfigValue *v = getValue(_name);
      if (v!=NULL) {
        if (v->getType() >= CFTP_ARR) CONF_MANAGER_ERR("getChar('%s') called on array element without array index!",_name);
        if (isSet != NULL) *isSet = v->isSet();
        return v->getChar();
      }
      return 0;
    }

    /* the _f getXXX functions free the memory allocated for _name before they return normally! */
    const char    getChar_f(char *_name, int *isSet=NULL) const {  /* return value is read only! */
      const char c = getChar(_name, isSet);
      if (_name != NULL) free(_name);
      return c;
    }

    int isSet(const char *_name) const {
      try {
        const ConfigValue *v = getValue(_name);
        if (v!=NULL) {
          return v->isSet();
        } else return 0;
      } catch (const cConfigException&) {
        return 0;
      }
    }

    /* the _f getXXX functions free the memory allocated for _name before they return normally! */
    int isSet_f(char *_name) const {
      int s = isSet(_name);
      if (_name != NULL) free(_name);
      return s;
    }

    /* print help for all registered config types, if selection==NULL, 
       else print help for type(s) beginning with the name given in *selection */
    void printTypeHelp(int _subtype=1, const char *selection=NULL, int dfltConf=0);

    /* print a default config file section for the type given in selection, 
       or for all types if selection == NULL. 
       If _subtype == 1, explicitely print default values of all subtypes. */
    void printTypeDfltConfig(const char *selection=NULL, int _subtype=1, int fullMode=0, int withDescription=1);

    rapidjson::Value exportTypeInfo(rapidjson::MemoryPoolAllocator<> &allocator);

    /*
    naming conventions
    */
    ~cConfigManager();
};

#undef MODULE
#endif





#endif  // __CONFIG_MANAGER_HPP
