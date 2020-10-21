/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#ifndef __SMILEUTILRINGBUFFER_HPP
#define __SMILEUTILRINGBUFFER_HPP

// general ringbuffer
template <class DataType> class cSmileUtilRingBuffer {
protected:
  DataType * x_;
  DataType * tmp_;
  long N_;
  long rdPtr_;
  long wrPtr_;
  bool filled_;
  bool noHang_;  // if true, new incoming data will overwrite old, non-read data

  long rIdx(long vIdx) {
    return vIdx % N_;
  }

public:
  cSmileUtilRingBuffer() : N_(0), x_(NULL), rdPtr_(0), wrPtr_(0), tmp_(NULL), filled_(false), noHang_(false) {}

  cSmileUtilRingBuffer(long N) : N_(N), rdPtr_(0), wrPtr_(0), tmp_(NULL), filled_(false), noHang_(false) {
    x_ = new DataType[N];
  }

  virtual ~cSmileUtilRingBuffer() {
    if (x_ != NULL) {
      delete[] x_;
    }
  }

  void setNoHang() { 
    noHang_ = true;
  }
  
  void setHang() {
    noHang_ = false;
  }

  long nUsed() {
    return wrPtr_ - rdPtr_;
  }

  long nFree() {
    return (N_ - nUsed());
  }

  // sets the read pointer to writePtr - N
  void updateRdPtr() {
    if (wrPtr_ - rdPtr_ > N_) {
      rdPtr_ = wrPtr_ - N_;
    }
  }

  virtual bool setNext(const DataType * x, long n) {
    if (n <= nFree() || noHang_) {
      for (long i = 0; i < n; i++) {
        x_[rIdx(i + wrPtr_)] = x[i];
      }
      wrPtr_ += n;
      if (noHang_) {
        updateRdPtr();
      }
      return true;
    }
    return false;
  }

  virtual DataType get(long vIdx) {
    if (vIdx >= rdPtr_) {
      rdPtr_ = vIdx;
    }
    return x_[rIdx(vIdx)];
  }

  virtual bool set(long vIdx, DataType x) {
    if (vIdx <= wrPtr_ && vIdx > wrPtr_ - N_) {
      x_[rIdx(vIdx)] = x;
      if (vIdx == wrPtr_) {
        wrPtr_++;
      }
      return true;
    }
    return false;
  }

  virtual DataType getNext() {
    return x_[rIdx(rdPtr_++)];
  }

  virtual bool setNext(DataType x) {
    if (nFree() > 0 || noHang_) {
      x_[rIdx(wrPtr_++)] = x;
      if (noHang_) {
        updateRdPtr();
      }
      return true;
    }
    return false;
  }

  long getWritePointer() {
    return wrPtr_;
  }

  long getReadPointer() {
    return rdPtr_;
  }

  bool filled() {
    return (wrPtr_ >= N_);
  }

  void clear() {
    wrPtr_ = 0;
    rdPtr_ = 0;
  }
};

// ringbuffer for flat matrix data
template <class DataType> class cSmileUtilRingBuffer2D : public cSmileUtilRingBuffer<DataType> {
protected:
  long nDim_;

public:
  cSmileUtilRingBuffer2D() : cSmileUtilRingBuffer<DataType>(), nDim_(1) {}

  /* N: number of columns, dim: number of rows */
  cSmileUtilRingBuffer2D(long N, long dim) : nDim_(dim) {
    this->N_ = N;
    this->rdPtr_ = 0;
    this->wrPtr_ = 0;
    this->tmp_ = NULL;
    this->filled_ = false;
    this->noHang_ = false;
    if (this->N_ < 1) {
      this->N_ = 1;
    }
    if (nDim_ < 1) {
      nDim_ = 1;
    }
    this->x_ = new DataType[this->N_ * nDim_];
  }

  // n is the number of vectors of nDim_ contained in x
  bool setNext(const DataType * x, long n) {
    if (n <= this->nFree() || this->noHang_) {
      for (long i = 0; i < n; i++) {
        memcpy(this->x_ + this->rIdx(i + this->wrPtr_) * nDim_, x + nDim_ * i, nDim_ * sizeof(DataType));
      }
      this->wrPtr_ += n;
      if (this->noHang_) {
        this->updateRdPtr();
      }
      return true;
    }
    return false;
  }

  // gets pointer to a single vector at vIdx
  const DataType * getVectorPtr(long vIdx) {
    if (vIdx >= this->rdPtr_) {
      this->rdPtr_ = vIdx;
    }
    return this->x_ + this->rIdx(vIdx) * nDim_;
  }

  // gets the whole filled part of the buffer as matrix
  // caller must free the matrix!
  long getBufferMatrixDup(DataType **x, long nPast = 0) {
    long nu = this->nUsed();
    if (x != NULL) {
      if (*x != NULL) {
        delete[] *x;
      }
      *x = new DataType[nDim_ * nu];
      if (nPast > 0) {
        if (nPast > nu) { nPast = nu; }
        for (long i = nPast; i > 0; i--) {
          memcpy(*x + (nPast - i) * nDim_, this->x_ + this->rIdx(this->wrPtr_ - i), nDim_ * sizeof(DataType));
        }
        return nPast;
      } else {
        for (long i = 0; i < nu; i++) {
          memcpy(*x + i * nDim_, this->x_ + this->rIdx(i + this->rdPtr_), nDim_ * sizeof(DataType));
        }
        return nu;
      }
    } else {
      return 0;
    }
  }

  // sets a single vector at vIdx
  bool setVector(long vIdx, DataType * x) {
    if (vIdx <= this->wrPtr_ && vIdx > this->wrPtr_ - this->N_) {
      memcpy(this->x_ + this->rIdx(vIdx) * nDim_, x, nDim_ * sizeof(DataType));
      if (vIdx == this->wrPtr_) {
        this->wrPtr_++;
      }
      return true;
    }
    return false;
  }

  /*
  DataType getNext() {
    // TODO
    return x_[rIdx(rdPtr_++)];
  }

  bool setNext(DataType x) {
    // TODO
    if (nFree() > 0) {
      x_[rIdx(wrPtr_++)] = x;
      return true;
    }
    return false;
  }
  */
};

/* Implements a running sum of double values with a buffer */
class cSmileUtilRunningSum {
private:
  double *x_;
  double sum_;
  int len_;
  int minlen_;
  int n_;
  int ptr_;
  bool avg_;
public:
  /* len: the maximum length of the running sum, if more samples are added the sum will warp */
  /* avg: if set to true, will return the average of the samples in the buffer, not the sum */
  cSmileUtilRunningSum(int len, bool avg = false) : sum_(0.0), n_(0), ptr_(0), avg_(avg) {
    if (len > 0) {
      x_ = new double[len];
    } else {
      x_ = NULL;
    }
    len_ = len;
    minlen_ = len / 2;
  }
  ~cSmileUtilRunningSum() {
    if (x_ != NULL) delete x_;
  }

  /* gets the current sum/average */
  double get() const {
    if (avg_ && n_ > 0) {
      return (sum_ / (double)n_);
    } else {
      return (sum_);
    }
  }

  /* adds a new sample to the sum/buffer, returns the current sum/avg value */
  double add(double x) {
    if (len_ > 0) {
      sum_ += x;
      x_[ptr_] = x;
      if (n_ < len_) n_++;
      ptr_++;
      if (ptr_ >= len_) ptr_ = 0;
      if (n_ >= len_) {
        sum_ -= x_[ptr_];
      }
      return get();
    } else {
      return x;
    }
  }

  bool empty() {
    return (n_ <= 0);
  }

  /* returns true if the buffer is fully filled (contains n == length samples) */
  bool filled() {
    return (n_ == len_);
  }

  /* see minFilled() */
  void setMinLen(int minlen) {
    if (minlen <= len_ && minlen_ > 0) {
      minlen_ = minlen;
    } else {
      minlen_ = len_;
    }
  }

  /* returns true if the buffer contains at least minlen_ samples (len_/2 by default) */
  /* minlen_ can be set with setMinLen(minlen) */
  bool minFilled() {
    return (n_ >= minlen_);
  }

  /* clears the sum buffer (resets to 0) and resets all pointers */
  void clear() {
    ptr_ = 0;
    n_ = 0;
    sum_ = 0.0;
  }
};

#endif  // __SMILEUTILRINGBUFFER_HPP

