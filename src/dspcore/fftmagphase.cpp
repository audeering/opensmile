/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

compute magnitude and phase from complex valued fft output
TODO: support inverse??
 
*/


#include <dspcore/fftmagphase.hpp>
#include <math.h>

#define MODULE "cFFTmagphase"

/*Library:
sComponentInfo * registerMe(cConfigManager *_confman) {
  cDataProcessor::registerComponent(_confman);
}
*/

SMILECOMPONENT_STATICS(cFFTmagphase)

SMILECOMPONENT_REGCOMP(cFFTmagphase)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFFTMAGPHASE;
  sdescription = COMPONENT_DESCRIPTION_CFFTMAGPHASE;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("inverse","If set to 1, converts magnitude and phase input data to complex frequency domain data",0);
    ct->setField("magnitude","1/0 = compute magnitude yes/no (or use magnitude as input to inverse transformation, must be enabled for inverse)",1);
    ct->setField("phase","1/0 = compute phase yes/no (or use phase as input to inverse transformation, must be enabled for inverse)",0);
    ct->setField("joinMagphase","Output magnitude and phase information to a single array field (instead of creating two array fields, one for magnitude and one for phase information). The first half contains magnitude values, the second half phase values.",0);

    ct->setField("normalise","1/0 = yes/no: normalise FFT magnitudes to input window length, to obtain spectral densities.",0);
    ct->setField("power","1/0 = yes/no: square FFT magnitudes to obtain power spectrum.",0);

    ct->setField("dBpsd","1/0 = yes/no: output logarithmic (dB SPL) power spectral density instead of linear magnitude spectrum (you should use a Hann window for analysis in this case). Setting this option also sets/implies 'normalise=1' and 'power=1'",0);
    ct->setField("dBpnorm","Value for dB power normalisation when 'dBpsd=1' (in dB SPL). Default is according to MPEG-1, psy I model.",90.302);
    ct->setField("mindBp", "Minimum dB power value for flooring when using dBpsd. (mindBp >= dBpnorm - 120.0) will be enforced, so mindBp might be higher than set here, depending on dBpnorm parameter.", -102.0);
  )

  SMILECOMPONENT_MAKEINFO(cFFTmagphase);
}

SMILECOMPONENT_CREATE(cFFTmagphase)

//-----


cFFTmagphase::cFFTmagphase(const char *_name) :
  cVectorProcessor(_name),
  magnitude(1),
  phase(0)
{

}

void cFFTmagphase::myFetchConfig()
{
  cVectorProcessor::myFetchConfig();
  
  inverse = getInt("inverse");
  magnitude = getInt("magnitude");
  phase = getInt("phase");
  joinMagphase = getInt("joinMagphase");

  if ((!magnitude)&&(!phase)&&(!power)&&(!dBpsd)) { magnitude = 1; }
  if (magnitude) { SMILE_IDBG(2,"magnitude computation enabled"); }
  if (phase) { SMILE_IDBG(2,"phase computation enabled"); }
  if (inverse) { 
    SMILE_IDBG(2,"inverse mode selected"); 
    if (!(magnitude&&phase)) {
      SMILE_IERR(1,"we need magnitude AND phase as inputs for inverse fftmagphase. Thus you need to provide the phase with the input AND enable the option 'phase' in the config file!");
      COMP_ERR("aborting");
    }
  }

  power = getInt("power");
  if (power) { magnitude = 1; }
  normalise = getInt("normalise");
  dBpsd = getInt("dBpsd");
  if (dBpsd) { normalise = 1; magnitude = 1; }
  dBpnorm = (FLOAT_DMEM)getDouble("dBpnorm");
  mindBp = (FLOAT_DMEM)getDouble("mindBp");
  if (mindBp - dBpnorm < -120.0) {
    mindBp = -120 + dBpnorm;
    SMILE_IMSG(3, "mindBp = %f", mindBp);
  }
}


int cFFTmagphase::setupNamesForField(int i, const char*name, long nEl)
{
  long newNEl = 0; 
  
  // copy frequency axis information for this field...
  const FrameMetaInfo *c = reader_->getFrameMetaInfo();

  if (inverse) {
    // TODO: find Mag and Phase fields by name, build lookup table and create only a single output for the Mag field, ignore the phase field...
    // TODO2: cache in processVector, or method to access the phase field when the magnitude field is processed...
    // OR: do not derive from cVectorProcessor...?
    // ASSUMPTION: The first field is the magnitudes, and the second the phases, both same size, we use only the first here!
    if (i == 0) {
      addNameAppendFieldAuto(name, "fftcInv", (nEl*2)-2);
      newNEl += (nEl*2)-2;
    }
  } else {
    if (magnitude) {
      int dtype = DATATYPE_SPECTRUM_BINS_MAG;
      if (phase && joinMagphase) {
        dtype = DATATYPE_SPECTRUM_BINS_MAGPHASE;
        // TODO: new datatypes for mag_phase version of power spectra, etc.
        if (dBpsd) {
          addNameAppendFieldAuto(name, "fftMag_dBsplPSD_Phase", nEl + 2);
          dtype = DATATYPE_SPECTRUM_BINS_DBPSD;
        } else if (power && !normalise) {
          addNameAppendFieldAuto(name, "fftMag_PowSpec_Phase", nEl + 2);
          dtype = DATATYPE_SPECTRUM_BINS_POWSPEC;
        } else if (power && normalise) {
          addNameAppendFieldAuto(name, "fftMag_PowSpecDens_Phase", nEl + 2);
          dtype = DATATYPE_SPECTRUM_BINS_POWSPECDENS;
        } else if (normalise && !power) {
          addNameAppendFieldAuto(name, "fftMag_SpecDens_Phase", nEl + 2);
          dtype = DATATYPE_SPECTRUM_BINS_SPECDENS;
        } else {
          addNameAppendFieldAuto(name, "fftMagphase", nEl + 2);
        }
        newNEl += nEl + 2; 
      } else {
        if (dBpsd) {
          addNameAppendFieldAuto(name, "fftMag_dBsplPSD", nEl/2 + 1);
          dtype = DATATYPE_SPECTRUM_BINS_DBPSD;
        } else if (power && !normalise) {
          addNameAppendFieldAuto(name, "fftMag_PowSpec", nEl/2 + 1);
          dtype = DATATYPE_SPECTRUM_BINS_POWSPEC;
        } else if (power && normalise) {
          addNameAppendFieldAuto(name, "fftMag_PowSpecDens", nEl/2 + 1);
          dtype = DATATYPE_SPECTRUM_BINS_POWSPECDENS;
        } else if (normalise && !power) {
          addNameAppendFieldAuto(name, "fftMag_SpecDens", nEl/2 + 1);
          dtype = DATATYPE_SPECTRUM_BINS_SPECDENS;
        } else {
          addNameAppendFieldAuto(name, "fftMag", nEl/2 + 1);
        }
        newNEl += nEl/2 +1; 
      }
      void * _buf = malloc(c->field[i].infoSize);
      memcpy(_buf, c->field[i].info, c->field[i].infoSize);
      writer_->setFieldInfo(-1,dtype,_buf,c->field[i].infoSize);
    } 
    if (phase && (!magnitude || !joinMagphase)) {
      addNameAppendFieldAuto(name, "fftPhase", nEl/2 + 1);
      newNEl += nEl/2 +1; 
      void * _buf = malloc(c->field[i].infoSize);
      memcpy(_buf, c->field[i].info, c->field[i].infoSize);
      writer_->setFieldInfo(-1,DATATYPE_SPECTRUM_BINS_PHASE,_buf,c->field[i].infoSize);  
    }
  }
  
  

  return newNEl;
}



// a derived class should override this method, in order to implement the actual processing
int cFFTmagphase::processVector(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  long n;
  if (Nsrc <= 0) return 0;
  // TODO : seperate magnitude and phase is still broken, as processVector does not allow for a single input field and more than one output fields...

  // TODO: nyquist phase
  if (inverse) { // inverse is only possible from joint Mag & phase array fields... TODO: check this!

    // we assume the first Nsrc/2+1 inputs to be magnitude and the second Nsrc/2 inputs to be phase
    if ( (Nsrc & 1) == 1 ) { 
      SMILE_IERR(1,"number of inputs (Nsrc=%i) must be even, please check if you have enabled magnitude and phase input!",Nsrc); 
      COMP_ERR("aborting");
    }
    const FLOAT_DMEM *mag = src;
    const FLOAT_DMEM *phase = src + Nsrc/2;
    long myN = Nsrc/2;

    dst[0] = src[0]*src[Nsrc/2];
    dst[1] = 0;
    for (n=1; n<myN-1; n++) {
      dst[n*2] = cos(phase[n])*mag[n];  // real
      dst[n*2+1] = sin(phase[n])*mag[n];  // imaginary
    }
    
  } else {
    int doMag = 0;

    if (magnitude && phase && !joinMagphase) {
      // check if this is an even field number... if not skip
      if ((idxi & 1) == 0) doMag = 1;
    } else {
      doMag = 1;
    }

    if (doMag) {
      if (magnitude && !dBpsd && !normalise && !power) {
        dst[0] = fabs(src[0]);
        for(n=2; n<Nsrc; n += 2) {
          dst[n/2] = sqrt(src[n]*src[n] + src[n+1]*src[n+1]);
        }
        dst[Nsrc/2] = fabs(src[1]);
        //dst += Nsrc/2+1;
        //    dst += Nsrc/2;
      } else if (magnitude && !dBpsd && normalise && !power) {
        dst[0] = ((FLOAT_DMEM)1.0/(FLOAT_DMEM)Nsrc)*fabs(src[0]);
        for(n=2; n<Nsrc; n += 2) {
          dst[n/2] = ((FLOAT_DMEM)1.0/(FLOAT_DMEM)Nsrc)*sqrt(src[n]*src[n] + src[n+1]*src[n+1]);
        }
        dst[Nsrc/2] = ((FLOAT_DMEM)1.0/(FLOAT_DMEM)Nsrc)*fabs(src[1]);
        //dst += Nsrc/2+1;
      } else if (magnitude && !dBpsd && normalise && power) {
        dst[0] = ((FLOAT_DMEM)1.0/(FLOAT_DMEM)Nsrc)*src[0]; 
        dst[0] *= dst[0];
        for(n=2; n<Nsrc; n += 2) {
          dst[n/2] = ((FLOAT_DMEM)1.0/((FLOAT_DMEM)Nsrc*(FLOAT_DMEM)Nsrc))*(src[n]*src[n] + src[n+1]*src[n+1]);
        }
        dst[Nsrc/2] = ((FLOAT_DMEM)1.0/(FLOAT_DMEM)Nsrc)*fabs(src[1]);
        dst[Nsrc/2] *= dst[Nsrc/2];
        //dst += Nsrc/2+1;
      } else if (magnitude && !dBpsd && !normalise && power) {
        dst[0] = fabs(src[0]); 
        dst[0] *= dst[0];
        for(n=2; n<Nsrc; n += 2) {
          dst[n/2] = (src[n]*src[n] + src[n+1]*src[n+1]);
        }
        dst[Nsrc/2] = fabs(src[1]);
        dst[Nsrc/2] *= dst[Nsrc/2];
        //dst += Nsrc/2+1;
      } else if (magnitude && dBpsd) {
        dst[0] = MAX(mindBp, (dBpnorm + (FLOAT_DMEM)20.0 * log10( ((FLOAT_DMEM)1.0/(FLOAT_DMEM)Nsrc)*fabs(src[0]) )));
        for(n=2; n<Nsrc; n += 2) {
          dst[n/2] = MAX(mindBp, (dBpnorm + (FLOAT_DMEM)10.0 * log10( ((FLOAT_DMEM)1.0/((FLOAT_DMEM)Nsrc*(FLOAT_DMEM)Nsrc))*(src[n]*src[n] + src[n+1]*src[n+1]) )) );
        }
        dst[Nsrc/2] = MAX(mindBp, (dBpnorm + (FLOAT_DMEM)20.0 * log10( ((FLOAT_DMEM)1.0/(FLOAT_DMEM)Nsrc)*fabs(src[1]) )));
        //dst += Nsrc/2+1;
      }
    }
    if (phase) {
      if (joinMagphase && magnitude) {
        // add Nsrc/2 to index...
        dst[Nsrc/2+1] = (src[0]>=0) ? (FLOAT_DMEM)0 : (FLOAT_DMEM)M_PI;
        for(n=2; n<Nsrc; n+=2) {
          dst[Nsrc/2+1+n/2] = atan2(src[n+1],src[n]);
        }
        // TODO: valgrind checks on this line!!
        dst[Nsrc/2+1+Nsrc/2] = (src[1]>=0) ? (FLOAT_DMEM)0 : (FLOAT_DMEM)M_PI;
      } else {
        if (magnitude) {
          // check if this field is the phase field, otherwise skip...
          if ((idxi & 1) == 1) {  // TODO: this check is wrong, it will never output phase for a single input field!
            dst[0] = (src[0]>=0) ? (FLOAT_DMEM)0 : (FLOAT_DMEM)M_PI;
            for(n=2; n<Nsrc; n+=2) {
              dst[n/2] = atan2(src[n+1],src[n]);
            }
          }
          dst[Nsrc/2] = (src[1]>=0) ? (FLOAT_DMEM)0 : (FLOAT_DMEM)M_PI;
        } else {
          // only phase output, so this field has to be the phase field...
          dst[0] = (src[0]>=0) ? (FLOAT_DMEM)0 : (FLOAT_DMEM)M_PI;
          for(n=2; n<Nsrc; n+=2) {
            dst[n/2] = atan2(src[n+1],src[n]);
          }
          dst[Nsrc/2] = (src[1]>=0) ? (FLOAT_DMEM)0 : (FLOAT_DMEM)M_PI;
        }
      }
      //    dst[Nsrc/2] = 0.0;
      //dst += Nsrc/2+1;
    }

  }

  return 1;
}

cFFTmagphase::~cFFTmagphase()
{
}

