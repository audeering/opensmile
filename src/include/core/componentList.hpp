/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

// basic functionality / SMILE API
#include <core/dataMemory.hpp>
#include <core/dataReader.hpp>
#include <core/dataWriter.hpp>
#include <core/dataSource.hpp>
#include <core/dataSink.hpp>
#include <core/dataProcessor.hpp>
#include <core/dataSelector.hpp>
#include <core/vectorProcessor.hpp>
#include <core/vectorTransform.hpp>
#include <core/windowProcessor.hpp>
#include <core/winToVecProcessor.hpp>
#include <core/vecToWinProcessor.hpp>

// examples:
#include <examples/exampleSource.hpp>
#include <examples/exampleSink.hpp>
#include <examples/simpleMessageSender.hpp>
//#include <examples/exampleProcessor.hpp>

// basic operations:
#include <other/vectorConcat.hpp>
#include <dspcore/windower.hpp>
#include <dspcore/framer.hpp>
#include <other/vectorOperation.hpp>
#include <other/vectorBinaryOperation.hpp>
#include <other/valbasedSelector.hpp>
#include <other/maxIndex.hpp>
#include <other/externalMessageInterface.hpp>
#include <dspcore/fullinputMean.hpp>
#include <dspcore/fullturnMean.hpp>

// sources:
#include <iocore/waveSource.hpp>
#include <iocore/arffSource.hpp>
#include <iocore/csvSource.hpp>
#include <iocore/htkSource.hpp>
#include <iocore/externalSink.hpp>
#include <iocore/externalSource.hpp>
#include <iocore/externalAudioSource.hpp>

// sinks:
#include <core/nullSink.hpp>
#include <iocore/csvSink.hpp>
#include <iocore/datadumpSink.hpp>
#include <iocore/dataPrintSink.hpp>
#include <iocore/arffSink.hpp>
#include <iocore/htkSink.hpp>
#include <iocore/waveSink.hpp>
#include <iocore/waveSinkCut.hpp>

//"fake" sources / producers:
#include <dsp/signalGenerator.hpp>

// portaudio code:
#ifdef HAVE_PORTAUDIO
 #include <portaudio/portaudioWavplayer.hpp>
 #include <portaudio/portaudioSource.hpp>
 #include <portaudio/portaudioSink.hpp>
 #include <portaudio/portaudioDuplex.hpp>
#endif

// for ffmpeg components:
#ifdef HAVE_FFMPEG
 #include <ffmpeg/ffmpegSource.hpp>
#endif

#ifdef __ANDROID__
 #include <android/openslesSource.hpp>
#endif

#if defined(__IOS__) && !defined(__IOS_WATCH__)
 #include <ios/coreAudioSource.hpp>
#endif

// live sinks (classifiers):
#ifdef BUILD_LIBSVM
 #include <classifiers/libsvmliveSink.hpp>
#endif

#ifdef BUILD_RNN
 #include <rnn/rnnSink.hpp>
 #include <rnn/rnnProcessor.hpp>
 #include <rnn/rnnVad2.hpp>
#endif

#ifdef BUILD_SVMSMO
#include <classifiers/svmSink.hpp>
#endif

// dsp core:
#include <dspcore/monoMixdown.hpp>
#include <dspcore/transformFft.hpp>
#include <dspcore/fftmagphase.hpp>
#include <dspcore/amdf.hpp>
#include <dspcore/acf.hpp>
#include <dspcore/preemphasis.hpp>
#include <dspcore/vectorPreemphasis.hpp>
#include <dspcore/vectorMVN.hpp>
#include <dspcore/turnDetector.hpp>
#include <dspcore/deltaRegression.hpp>
#include <dspcore/contourSmoother.hpp>

// dsp advanced:
#include <dsp/smileResample.hpp>
#include <dsp/specResample.hpp>
#include <dsp/dbA.hpp>
#include <dsp/vadV1.hpp>

#include <dsp/specScale.hpp>

// LLD core:
#include <lldcore/mzcr.hpp>
#include <lldcore/energy.hpp>
#include <lldcore/intensity.hpp>
#include <lldcore/melspec.hpp>
#include <lldcore/mfcc.hpp>
#include <lldcore/plp.hpp>
#include <lldcore/spectral.hpp>
#include <lldcore/pitchBase.hpp>
#include <lldcore/pitchACF.hpp>
#include <lldcore/pitchSmoother.hpp>

// LLD (low-level descriptors):
#include <lld/tonespec.hpp>
#include <lld/tonefilt.hpp>
#include <lld/chroma.hpp>
#include <lld/cens.hpp>
#include <lld/pitchDirection.hpp>
#include <lld/formantSmoother.hpp>
#include <lld/pitchSmootherViterbi.hpp>
#include <lld/pitchJitter.hpp>
#include <lld/pitchShs.hpp>
#include <lld/lpc.hpp>
#include <lld/lsp.hpp>
#include <lld/formantLpc.hpp>
#include <lld/harmonics.hpp>

// functionals:
#include <functionals/functionals.hpp>
#include <functionals/functionalExtremes.hpp>
#include <functionals/functionalMeans.hpp>
#include <functionals/functionalPeaks.hpp>
#include <functionals/functionalPeaks2.hpp>
#include <functionals/functionalSegments.hpp>
#include <functionals/functionalOnset.hpp>
#include <functionals/functionalMoments.hpp>
#include <functionals/functionalCrossings.hpp>
#include <functionals/functionalPercentiles.hpp>
#include <functionals/functionalRegression.hpp>
#include <functionals/functionalSamples.hpp>
#include <functionals/functionalTimes.hpp>
#include <functionals/functionalDCT.hpp>
#include <functionals/functionalLpc.hpp>
#include <functionals/functionalModulation.hpp>

// advanced io
#include <io/libsvmSink.hpp>

// OpenCV:
#ifdef HAVE_OPENCV
  #include <video/openCVSource.hpp>
#endif

const registerFunction componentlist[] = {
  // basic functionality / SMILE API
  cDataMemory::registerComponent,
  cDataWriter::registerComponent,
  cDataReader::registerComponent,
  cDataSource::registerComponent,
  cDataSink::registerComponent,
  cDataProcessor::registerComponent,
  cDataSelector::registerComponent,
  cVectorProcessor::registerComponent,
  cVectorTransform::registerComponent,
  cWindowProcessor::registerComponent,
  cWinToVecProcessor::registerComponent,
  cVecToWinProcessor::registerComponent,

  // examples
#ifdef BUILD_COMPONENT_ExampleSource
  cExampleSource::registerComponent,
#endif
#ifdef BUILD_COMPONENT_ExampleSink
  cExampleSink::registerComponent,
#endif
#ifdef BUILD_COMPONENT_SimpleMessageSender
  cSimpleMessageSender::registerComponent,
#endif
  //cExampleProcessor::registerComponent,

  // basic operations:
  cVectorConcat::registerComponent,
  cFramer::registerComponent,
  cWindower::registerComponent,
  cVectorOperation::registerComponent,
  cVectorBinaryOperation::registerComponent,
  cValbasedSelector::registerComponent,
  cMaxIndex::registerComponent,
  cExternalMessageInterface::registerComponent,
  cFullinputMean::registerComponent,
  cFullturnMean::registerComponent,

  // sources:
  cWaveSource::registerComponent,
  cArffSource::registerComponent,
  cCsvSource::registerComponent,
  cHtkSource::registerComponent,
  cExternalSink::registerComponent,
  cExternalSource::registerComponent,
  cExternalAudioSource::registerComponent,

  // sinks:
  cNullSink::registerComponent,
  cCsvSink::registerComponent,
  cDatadumpSink::registerComponent,
  cDataPrintSink::registerComponent,
  cArffSink::registerComponent,
  cHtkSink::registerComponent,
  cWaveSink::registerComponent,
  cWaveSinkCut::registerComponent,

  //"fake" sources / producers:
  #ifdef BUILD_COMPONENT_SignalGenerator
  cSignalGenerator::registerComponent,
  #endif

  #ifdef HAVE_PORTAUDIO
   cPortaudioWavplayer::registerComponent,
   cPortaudioSource::registerComponent,
   cPortaudioSink::registerComponent,
   cPortaudioDuplex::registerComponent,
  #endif

  #ifdef HAVE_FFMPEG
   cFFmpegSource::registerComponent,
  #endif

  #ifdef __ANDROID__
   #ifdef BUILD_COMPONENT_OpenslesSource
   cOpenslesSource::registerComponent,
   #endif
  #endif

  #if defined(__IOS__) && !defined(__IOS_WATCH__)
    cCoreAudioSource::registerComponent,
  #endif

  // live sinks (classifiers):
  #ifdef BUILD_LIBSVM
  #ifdef BUILD_COMPONENT_LibsvmLiveSink
   cLibsvmLiveSink::registerComponent,
  #endif
  #endif

  #ifdef BUILD_RNN
   cRnnSink::registerComponent,
   cRnnProcessor::registerComponent,
   cRnnVad2::registerComponent,
  #endif

  #ifdef BUILD_SVMSMO
  #ifdef BUILD_COMPONENT_SvmSink
   cSvmSink::registerComponent,
  #endif
  #endif

  // dsp core:
  cMonoMixdown::registerComponent,
  cTransformFFT::registerComponent,
  cFFTmagphase::registerComponent,
  cAmdf::registerComponent,
  cAcf::registerComponent,
  cPreemphasis::registerComponent,
  cVectorPreemphasis::registerComponent,
  cVectorMVN::registerComponent,
  cTurnDetector::registerComponent,
  cDeltaRegression::registerComponent,
  cContourSmoother::registerComponent,

  // dsp advanced:
  #ifdef BUILD_COMPONENT_SmileResample
  cSmileResample::registerComponent,
  #endif
  #ifdef BUILD_COMPONENT_SpecResample
  cSpecResample::registerComponent,
  #endif
  #ifdef BUILD_COMPONENT_DbA
  cDbA::registerComponent,
  #endif
  #ifdef BUILD_COMPONENT_VadV1
  cVadV1::registerComponent,
  #endif
#ifdef BUILD_COMPONENT_SpecScale
  cSpecScale::registerComponent,
#endif

  // LLD core:
  cMZcr::registerComponent,
  cEnergy::registerComponent,
  cIntensity::registerComponent,
  cMelspec::registerComponent,
  cMfcc::registerComponent,
  cPlp::registerComponent,
  cSpectral::registerComponent,
  cPitchBase::registerComponent,
  cPitchACF::registerComponent,
  cPitchSmoother::registerComponent,

  // LLD advanced:
#ifdef BUILD_COMPONENT_Tonespec
  cTonespec::registerComponent,
#endif
#ifdef BUILD_COMPONENT_Tonefilt
  cTonefilt::registerComponent,
#endif
#ifdef BUILD_COMPONENT_Chroma
  cChroma::registerComponent,
#endif
#ifdef BUILD_COMPONENT_Cens
  cCens::registerComponent,
#endif
#ifdef BUILD_COMPONENT_PitchSmootherViterbi
  cPitchSmootherViterbi::registerComponent,
#endif
#ifdef BUILD_COMPONENT_PitchJitter
  cPitchJitter::registerComponent,
#endif
#ifdef BUILD_COMPONENT_PitchDirection
  cPitchDirection::registerComponent,
#endif
#ifdef BUILD_COMPONENT_PitchShs
  cPitchShs::registerComponent,
#endif
#ifdef BUILD_COMPONENT_Lpc
  cLpc::registerComponent,
#endif
#ifdef BUILD_COMPONENT_Lsp
  cLsp::registerComponent,
#endif
#ifdef BUILD_COMPONENT_FormantLpc
  cFormantLpc::registerComponent,
#endif
#ifdef BUILD_COMPONENT_FormantSmoother
  cFormantSmoother::registerComponent,
#endif
#ifdef BUILD_COMPONENT_Harmonics
  cHarmonics::registerComponent,
#endif

  // functionals:
#ifdef BUILD_COMPONENT_Functionals
  cFunctionals::registerComponent,
  cFunctionalExtremes::registerComponent,
  cFunctionalMeans::registerComponent,
  cFunctionalPeaks::registerComponent,
  cFunctionalPeaks2::registerComponent,
  cFunctionalSegments::registerComponent,
  cFunctionalOnset::registerComponent,
  cFunctionalMoments::registerComponent,
  cFunctionalCrossings::registerComponent,
  cFunctionalPercentiles::registerComponent,
  cFunctionalRegression::registerComponent,
  cFunctionalSamples::registerComponent,
  cFunctionalTimes::registerComponent,
  cFunctionalDCT::registerComponent,
  cFunctionalLpc::registerComponent,
  cFunctionalModulation::registerComponent,
#endif

  // io advanced:
#ifdef BUILD_COMPONENT_LibsvmSink
  cLibsvmSink::registerComponent,
#endif

#ifdef BUILD_COMPONENT_OpenCVSource
  #ifdef HAVE_OPENCV
  cOpenCVSource::registerComponent,
  #endif
#endif

  NULL   // the last element must always be NULL !
};
