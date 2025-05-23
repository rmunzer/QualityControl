// Copyright 2019-2022 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   ZDCRecDataTask.cxx
/// \author Carlo Puggioni
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "ZDC/ZDCRecDataTask.h"
#include <Framework/InputRecord.h>
#include <Framework/DataRefUtils.h>
#include <fairlogger/Logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <gsl/span>
#include <vector>
#include "ZDCBase/Constants.h"
#include "CommonUtils/NameConf.h"
#include "ZDCReconstruction/RecoConfigZDC.h"
#include "ZDCReconstruction/ZDCEnergyParam.h"
#include "ZDCReconstruction/ZDCTowerParam.h"
#include "DataFormatsZDC/RecEventFlat.h"
#include "ZDCSimulation/ZDCSimParam.h"

using namespace o2::zdc;
namespace o2::quality_control_modules::zdc
{

ZDCRecDataTask::~ZDCRecDataTask()
{
  mVecCh.clear();
  mVecType.clear();
  mNameHisto.clear();

  for (auto h : mHisto1D) {
    delete h.histo;
  }
  mHisto1D.clear();
  for (auto h : mHisto2D) {
    delete h.histo;
  }
  mHisto2D.clear();
}

void ZDCRecDataTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize ZDCRecDataTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  init();
}

void ZDCRecDataTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void ZDCRecDataTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void ZDCRecDataTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  auto bcrec = ctx.inputs().get<gsl::span<o2::zdc::BCRecData>>("zdc-bcrec");
  auto energy = ctx.inputs().get<gsl::span<o2::zdc::ZDCEnergy>>("zdc-energyrec");
  auto tdc = ctx.inputs().get<gsl::span<o2::zdc::ZDCTDCData>>("zdc-tdcrec");
  auto info = ctx.inputs().get<gsl::span<uint16_t>>("zdc-inforec");
  process(bcrec, energy, tdc, info);
}

void ZDCRecDataTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void ZDCRecDataTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ZDCRecDataTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  for (int i = 0; i < (int)mHisto1D.size(); i++) {
    mHisto1D.at(i).histo->Reset();
  }
  for (int i = 0; i < (int)mHisto2D.size(); i++) {
    mHisto2D.at(i).histo->Reset();
  }
}
void ZDCRecDataTask::init()
{
  initVecCh();
  initVecType();
  initHisto();
  // dumpHistoStructure();
}

void ZDCRecDataTask::initVecCh()
{
  // ZNA
  insertChVec("ZNAC");
  insertChVec("ZNA1");
  insertChVec("ZNA2");
  insertChVec("ZNA3");
  insertChVec("ZNA4");
  insertChVec("ZNAS");
  // ZPA
  insertChVec("ZPAC");
  insertChVec("ZPA1");
  insertChVec("ZPA2");
  insertChVec("ZPA3");
  insertChVec("ZPA4");
  insertChVec("ZPAS");
  // ZNC
  insertChVec("ZNCC");
  insertChVec("ZNC1");
  insertChVec("ZNC2");
  insertChVec("ZNC3");
  insertChVec("ZNC4");
  insertChVec("ZNCS");
  // ZPC
  insertChVec("ZPCC");
  insertChVec("ZPC1");
  insertChVec("ZPC2");
  insertChVec("ZPC3");
  insertChVec("ZPC4");
  insertChVec("ZPCS");
  // ZEM
  insertChVec("ZEM1");
  insertChVec("ZEM2");
  // Particular channels
  insertChVec("ZNC-ZNA");
  insertChVec("ZNC+ZNA");
  insertChVec("CH");
  insertChVec("MSG");
  insertChVec("CXZNA");
  insertChVec("CYZNA");
  insertChVec("CXZNC");
  insertChVec("CYZNC");
  insertChVec("CXZPA");
  insertChVec("CXZPC");
}

void ZDCRecDataTask::initVecType()
{
  insertTypeVec("ADC");
  insertTypeVec("TDCV");
  insertTypeVec("TDCA");
  insertTypeVec("TDCAC");
  insertTypeVec("ADCAC");
  insertTypeVec("BC");
  insertTypeVec("INFO");
}

void ZDCRecDataTask::insertChVec(std::string ch)
{
  mVecCh.push_back(ch);
}

void ZDCRecDataTask::insertTypeVec(std::string type)
{
  mVecType.push_back(type);
}

void ZDCRecDataTask::setBinHisto1D(int numBinX, double minBinX, double maxBinX)
{
  setNumBinX(numBinX);
  setMinBinX(minBinX);
  setMaxBinX(maxBinX);
}

void ZDCRecDataTask::setBinHisto2D(int numBinX, double minBinX, double maxBinX, int numBinY, double minBinY, double maxBinY)
{
  setNumBinX(numBinX);
  setMinBinX(minBinX);
  setMaxBinX(maxBinX);
  setNumBinY(numBinY);
  setMinBinY(minBinY);
  setMaxBinY(maxBinY);
}

// CENTRAL_EVENT_CONFIG -> tdcLimit [ns] ; centraleventconfig [discrete value]
void ZDCRecDataTask::SetConfigCentralEvent(float tdcLimit, int centraleventconfig)
{
  settdcLimit(tdcLimit);
  setcentraleventconfigvalue(centraleventconfig);
}

void ZDCRecDataTask::dumpHistoStructure()
{
  std::ofstream dumpFile;
  dumpFile.open("dumpStructuresRec.txt");

  dumpFile << "\n Vector Channels\n";
  for (int i = 0; i < (int)mVecCh.size(); i++) {
    dumpFile << mVecCh.at(i) << ", ";
  }
  dumpFile << "\n";

  dumpFile << "\n Vector Type\n";
  for (int i = 0; i < (int)mVecType.size(); i++) {
    dumpFile << mVecType.at(i) << ", ";
  }
  dumpFile << "\n";
  dumpFile << "\n Histos 1D \n";
  for (int i = 0; i < (int)mHisto1D.size(); i++) {
    dumpFile << mHisto1D.at(i).typeh << mHisto1D.at(i).histo->GetName() << " \t" << mHisto1D.at(i).ch << " \t" << mHisto1D.at(i).typech << "\n";
  }
  dumpFile << "\n Histos 2D \n";
  for (int i = 0; i < (int)mHisto2D.size(); i++) {
    dumpFile << mHisto2D.at(i).typeh << mHisto2D.at(i).histo->GetName() << " \t" << mHisto2D.at(i).typech1 << " \t" << mHisto2D.at(i).ch1 << " \t" << mHisto2D.at(i).typech2 << " \t" << mHisto2D.at(i).ch2 << "\n";
  }
  dumpFile << "\n HistoName \n";
  for (int i = 0; i < (int)mNameHisto.size(); i++) {
    dumpFile << mNameHisto.at(i) << "\n";
  }
  dumpFile.close();
}

void ZDCRecDataTask::initHisto()
{
  ILOG(Debug, Devel) << "initialize ZDC REC DATA HISTOGRAMS" << ENDM;
  std::vector<std::string> tokenString;

  if (auto param = mCustomParameters.find("ADC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ADC: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(1051, -202.5, 4002.5);
  }
  addNewHisto("ADC1D", "h_ADC_ZNA_TC", "ADC ZNA TC ", "ADC", "ZNAC", "", "", 1);
  addNewHisto("ADC1D", "h_ADC_ZNA_T1", "ADC ZNA T1 ", "ADC", "ZNA1", "", "", 2);
  addNewHisto("ADC1D", "h_ADC_ZNA_T2", "ADC ZNA T2 ", "ADC", "ZNA2", "", "", 3);
  addNewHisto("ADC1D", "h_ADC_ZNA_T3", "ADC ZNA T3 ", "ADC", "ZNA3", "", "", 4);
  addNewHisto("ADC1D", "h_ADC_ZNA_T4", "ADC ZNA T4 ", "ADC", "ZNA4", "", "", 5);
  addNewHisto("ADC1D", "h_ADC_ZNA_SUM", "ADC ZNA SUM ", "ADC", "ZNAS", "", "", 6);

  addNewHisto("ADC1D", "h_ADC_ZPA_TC", "ADC ZPA TC ", "ADC", "ZPAC", "", "", 7);
  addNewHisto("ADC1D", "h_ADC_ZPA_T1", "ADC ZPA T1 ", "ADC", "ZPA1", "", "", 8);
  addNewHisto("ADC1D", "h_ADC_ZPA_T2", "ADC ZPA T2 ", "ADC", "ZPA2", "", "", 9);
  addNewHisto("ADC1D", "h_ADC_ZPA_T3", "ADC ZPA T3 ", "ADC", "ZPA3", "", "", 10);
  addNewHisto("ADC1D", "h_ADC_ZPA_T4", "ADC ZPA T4 ", "ADC", "ZPA4", "", "", 11);
  addNewHisto("ADC1D", "h_ADC_ZPA_SUM", "ADC ZPA SUM ", "ADC", "ZPAS", "", "", 12);

  addNewHisto("ADC1D", "h_ADC_ZNC_TC", "ADC ZNC TC ", "ADC", "ZNCC", "", "", 15);
  addNewHisto("ADC1D", "h_ADC_ZNC_T1", "ADC ZNC T1 ", "ADC", "ZNC1", "", "", 16);
  addNewHisto("ADC1D", "h_ADC_ZNC_T2", "ADC ZNC T2 ", "ADC", "ZNC2", "", "", 17);
  addNewHisto("ADC1D", "h_ADC_ZNC_T3", "ADC ZNC T3 ", "ADC", "ZNC3", "", "", 18);
  addNewHisto("ADC1D", "h_ADC_ZNC_T4", "ADC ZNC T4 ", "ADC", "ZNC4", "", "", 19);
  addNewHisto("ADC1D", "h_ADC_ZNC_SUM", "ADC ZNC SUM ", "ADC", "ZNCS", "", "", 20);

  addNewHisto("ADC1D", "h_ADC_ZPC_TC", "ADC ZPC TC ", "ADC", "ZPCC", "", "", 21);
  addNewHisto("ADC1D", "h_ADC_ZPC_T1", "ADC ZPC T1 ", "ADC", "ZPC1", "", "", 22);
  addNewHisto("ADC1D", "h_ADC_ZPC_T2", "ADC ZPC T2 ", "ADC", "ZPC2", "", "", 23);
  addNewHisto("ADC1D", "h_ADC_ZPC_T3", "ADC ZPC T3 ", "ADC", "ZPC3", "", "", 24);
  addNewHisto("ADC1D", "h_ADC_ZPC_T4", "ADC ZPC T4 ", "ADC", "ZPC4", "", "", 25);
  addNewHisto("ADC1D", "h_ADC_ZPC_SUM", "ADC ZPC SUM ", "ADC", "ZPCS", "", "", 26);
  if (auto param = mCustomParameters.find("ADCZEM"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ADCZEM: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(1051, -202.5, 4002.5);
  }
  addNewHisto("ADC1D", "h_ADC_ZEM1", "ADC ZEM1 ", "ADC", "ZEM1", "", "", 13);
  addNewHisto("ADC1D", "h_ADC_ZEM2", "ADC ZEM2 ", "ADC", "ZEM2", "", "", 14);

  if (auto param = mCustomParameters.find("ADCH"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ADCH: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(1051, -202.5, 4002.5);
  }
  addNewHisto("ADC1D", "h_ADC_ZNA_TC_H", "ADC ZNA TC ZOOM", "ADC", "ZNAC", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZNA_SUM_H", "ADC ZNA SUM ZOOM", "ADC", "ZNAS", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZPA_TC_H", "ADC ZPA TC ZOOM", "ADC", "ZPAC", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZPA_SUM_H", "ADC ZPA SUM ZOOM", "ADC", "ZPAS", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZNC_TC_H", "ADC ZNC TC ZOOM", "ADC", "ZNCC", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZNC_SUM_H", "ADC ZNC SUM ZOOM", "ADC", "ZNCS", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZPC_TC_H", "ADC ZPC TC ZOOM", "ADC", "ZPCC", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZPC_SUM_H", "ADC ZPC SUM ZOOM", "ADC", "ZPCS", "", "", 0);

  addNewHisto("ADC1D", "h_ADC_ZPA_TC_H_CUT", "ADC ZPA TC ZOOM with cut", "ADCAC", "ZPAC", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZPA_SUM_H_CUT", "ADC ZPA SUM ZOOM with cut", "ADCAC", "ZPAS", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZPC_TC_H_CUT", "ADC ZPC TC ZOOM with cut", "ADCAC", "ZPCC", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZPC_SUM_H_CUT", "ADC ZPC SUM ZOOM with cut", "ADCAC", "ZPCS", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZNA_TC_H_CUT", "ADC ZNA TC ZOOM with cut", "ADCAC", "ZNAC", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZNA_SUM_H_CUT", "ADC ZNA SUM ZOOM with cut", "ADCAC", "ZNAS", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZNC_TC_H_CUT", "ADC ZNC TC ZOOM with cut", "ADCAC", "ZNCC", "", "", 0);
  addNewHisto("ADC1D", "h_ADC_ZNC_SUM_H_CUT", "ADC ZNC SUM ZOOM with cut", "ADCAC", "ZNCS", "", "", 0);

  if (auto param = mCustomParameters.find("TDCT"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCT: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(2500, -5.5, 245.5);
  }
  addNewHisto("TDC1D", "h_TDC_ZNA_TC_V", "TDC Time (ns) ZNA TC", "TDCV", "ZNAC", "", "", 1);
  addNewHisto("TDC1D", "h_TDC_ZNA_SUM_V", "TDC Time (ns) ZNA SUM", "TDCV", "ZNAS", "", "", 2);
  addNewHisto("TDC1D", "h_TDC_ZPA_TC_V", "TDC Time (ns) ZPA TC", "TDCV", "ZPAC", "", "", 3);
  addNewHisto("TDC1D", "h_TDC_ZPA_SUM_V", "TDC Time (ns) ZPA SUM", "TDCV", "ZPAS", "", "", 4);
  addNewHisto("TDC1D", "h_TDC_ZNC_TC_V", "TDC Time (ns) ZNC TC", "TDCV", "ZNCC", "", "", 7);
  addNewHisto("TDC1D", "h_TDC_ZNC_SUM_V", "TDC Time (ns) ZNC SUM", "TDCV", "ZNCS", "", "", 8);
  addNewHisto("TDC1D", "h_TDC_ZPC_TC_V", "TDC Time (ns) ZPC TC", "TDCV", "ZPCC", "", "", 9);
  addNewHisto("TDC1D", "h_TDC_ZPC_SUM_V", "TDC Time (ns) ZPC SUM", "TDCV", "ZPCS", "", "", 10);
  addNewHisto("TDC1D", "h_TDC_ZEM1_V", "TDC Time (ns)  ZEM1", "TDCV", "ZEM1", "", "", 5);
  addNewHisto("TDC1D", "h_TDC_ZEM2_V", "TDC Time (ns)  ZEM2", "TDCV", "ZEM2", "", "", 6);

  if (auto param = mCustomParameters.find("TDCA"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCA: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(2000, -0.5, 3999.5);
  }
  addNewHisto("TDC1D", "h_TDC_ZNA_TC_A", "TDC Amplitude ZNA TC", "TDCA", "ZNAC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNA_SUM_A", "TDC Amplitude ZNA SUM", "TDCA", "ZNAS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPA_TC_A", "TDC Amplitude ZPA TC", "TDCA", "ZPAC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPA_SUM_A", "TDC Amplitude ZPA SUM", "TDCA", "ZPAS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNC_TC_A", "TDC Amplitude ZNC TC", "TDCA", "ZNCC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNC_SUM_A", "TDC Amplitude ZNC SUM", "TDCA", "ZNCS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPC_TC_A", "TDC Amplitude ZPC TC", "TDCA", "ZPCC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPC_SUM_A", "TDC Amplitude ZPC SUM", "TDCA", "ZPCS", "", "", 0);
  if (auto param = mCustomParameters.find("TDCAZEM"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCAZEM: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(2000, -0.5, 3999.5);
  }
  addNewHisto("TDC1D", "h_TDC_ZEM1_A", "TDC Amplitude ZEM1", "TDCA", "ZEM1", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZEM2_A", "TDC Amplitude ZEM2", "TDCA", "ZEM2", "", "", 0);
  // TDC_A ZOOM
  if (auto param = mCustomParameters.find("TDCAH"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCAH: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(1051, -202.5, 4002.5);
  }
  addNewHisto("TDC1D", "h_TDC_ZNA_TC_A_H", "TDC Amplitude ZNA TC  ZOOM", "TDCA", "ZNAC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNA_SUM_A_H", "TDC Amplitude ZNA SUM  ZOOM", "TDCA", "ZNAS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPA_TC_A_H", "TDC Amplitude ZPA TC  ZOOM", "TDCA", "ZPAC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPA_SUM_A_H", "TDC Amplitude ZPA SUM  ZOOM", "TDCA", "ZPAS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNC_TC_A_H", "TDC Amplitude ZNC TC  ZOOM", "TDCA", "ZNCC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNC_SUM_A_H", "TDC Amplitude ZNC SUM  ZOOM", "TDCA", "ZNCS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPC_TC_A_H", "TDC Amplitude ZPC TC  ZOOM", "TDCA", "ZPCC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPC_SUM_A_H", "TDC Amplitude ZPC SUM  ZOOM", "TDCA", "ZPCS", "", "", 0);

  addNewHisto("TDC1D", "h_TDC_ZPA_TC_A_H_CUT", "TDC Amplitude ZPA TC ZOOM with cut", "TDCAC", "ZPAC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPA_SUM_A_H_CUT", "TDC Amplitude ZPA SUM ZOOM with cut", "TDCAC", "ZPAS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPC_TC_A_H_CUT", "TDC Amplitude ZPC TC ZOOM with cut", "TDCAC", "ZPCC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZPC_SUM_A_H_CUT", "TDC Amplitude ZPC SUM ZOOM with cut", "TDCAC", "ZPCS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNA_TC_A_H_CUT", "TDC Amplitude ZNA TC ZOOM with cut", "TDCAC", "ZNAC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNA_SUM_A_H_CUT", "TDC Amplitude ZNA SUM ZOOM with cut", "TDCAC", "ZNAS", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNC_TC_A_H_CUT", "TDC Amplitude ZNC TC ZOOM with cut", "TDCAC", "ZNCC", "", "", 0);
  addNewHisto("TDC1D", "h_TDC_ZNC_SUM_A_H_CUT", "TDC Amplitude ZNC SUM ZOOM with cut", "TDCAC", "ZNCS", "", "", 0);

  // Centroid ZPA
  if (auto param = mCustomParameters.find("CENTR_ZPA"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - CENTR_ZPA: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(2240, 0, 22.4);
  }
  addNewHisto("CENTR_ZPA", "h_CENTR_ZPA", "ZPA Centroid (cm)", "ADC", "CXZPA", "", "", 0);
  // Centroid ZPA
  if (auto param = mCustomParameters.find("CENTR_ZPC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - CENTR_ZPC: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto1D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()));
  } else {
    setBinHisto1D(2240, -22.4, 0);
  }
  addNewHisto("CENTR_ZPC", "h_CENTR_ZPC", "ZPC Centroid (cm)", "ADC", "CXZPC", "", "", 0);

  // 2D Histos

  if (auto param = mCustomParameters.find("ADCSUMvsTC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ADCSUMvsTC: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(1051, -202.5, 4002.5, 1051, -202.5, 4002.5);
  }
  addNewHisto("ADCSUMvsTC", "h_ADC_ZNAS_ZNAC", "ADC  ZNA SUM vs ADC  ZNA TC", "ADC", "ZNAC", "ADC", "ZNAS", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZPAS_ZPAC", "ADC  ZPA SUM vs ADC  ZPA TC", "ADC", "ZPAC", "ADC", "ZPAS", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZNCS_ZNCC", "ADC  ZNC SUM vs ADC  ZNC TC", "ADC", "ZNCC", "ADC", "ZNCS", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZPCS_ZPCC", "ADC  ZPC SUM vs ADC  ZPC TC", "ADC", "ZPCC", "ADC", "ZPCS", 0);

  addNewHisto("ADCSUMvsTC", "h_ADC_ZNAC_ZPAC", "ADC  ZNA TC vs ADC  ZPA TC", "ADC", "ZPAC", "ADC", "ZNAC", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZNCC_ZPCC", "ADC  ZNC TC vs ADC  ZPC TC", "ADC", "ZPCC", "ADC", "ZNCC", 0);
  if (auto param = mCustomParameters.find("ADCZEMvsADCZEM"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ADCZEMvsADCZEM: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(1051, -202.5, 4002.5, 1051, -202.5, 4002.5);
  }
  addNewHisto("ADCSUMvsTC", "h_ADC_ZEM1_ZEM2", "ADC  ZEM1 vs ADC  ZEM2", "ADC", "ZEM2", "ADC", "ZEM1", 0);
  if (auto param = mCustomParameters.find("ADCZEMvsTC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ADCZEMvsTC: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(1051, -202.5, 4002.5, 1051, -202.5, 4002.5);
  }
  addNewHisto("ADCSUMvsTC", "h_ADC_ZNA_ZEM1", "ADC  ZNA TC vs ADC  ZEM1", "ADC", "ZEM1", "ADC", "ZNAC", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZNA_ZEM2", "ADC  ZNA TC vs ADC  ZEM2", "ADC", "ZEM2", "ADC", "ZNAC", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZNC_ZEM1", "ADC  ZNC TC vs ADC  ZEM1", "ADC", "ZEM1", "ADC", "ZNCC", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZNC_ZEM2", "ADC  ZNC TC vs ADC  ZEM2", "ADC", "ZEM2", "ADC", "ZNCC", 0);

  addNewHisto("ADCSUMvsTC", "h_ADC_ZPA_ZEM1", "ADC  ZPA TC vs ADC  ZEM1", "ADC", "ZEM1", "ADC", "ZPAC", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZPA_ZEM2", "ADC  ZPA TC vs ADC  ZEM2", "ADC", "ZEM2", "ADC", "ZPAC", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZPC_ZEM1", "ADC  ZPC TC vs ADC  ZEM1", "ADC", "ZEM1", "ADC", "ZPCC", 0);
  addNewHisto("ADCSUMvsTC", "h_ADC_ZPC_ZEM2", "ADC  ZPC TC vs ADC  ZEM2", "ADC", "ZEM2", "ADC", "ZPCC", 0);

  if (auto param = mCustomParameters.find("ADCvsTDCT"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ADCvsTDCT: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(250, -5.5, 24.5, 1051, -202.5, 4002.5);
  }
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZNAC", "ADC  ZNA TC vs TDC Time (ns)  ZNA TC", "TDCV", "ZNAC", "ADC", "ZNAC", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZNAS", "ADC  ZNA SUM vs TDC Time (ns) ZNA SUM", "TDCV", "ZNAS", "ADC", "ZNAS", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZPAC", "ADC  ZPA TC vs TDC Time (ns) ZPA TC", "TDCV", "ZPAC", "ADC", "ZPAC", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZPAS", "ADC  ZPA SUM vs TDC Time (ns) ZPA SUM", "TDCV", "ZPAS", "ADC", "ZPAS", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZNCC", "ADC  ZNC TC vs TDC Time (ns) ZNC TC", "TDCV", "ZNCC", "ADC", "ZNCC", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZNCS", "ADC  ZNC SUM vs TDC Time (ns) ZNC SUM", "TDCV", "ZNCS", "ADC", "ZNCS", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZPCC", "ADC  ZPC TC vs TDC Time (ns) ZPC TC", "TDCV", "ZPCC", "ADC", "ZPCC", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZPCS", "ADC  ZPC SUM vs TDC Time (ns) ZPC SUM", "TDCV", "ZPCS", "ADC", "ZPCS", 0);
  if (auto param = mCustomParameters.find("ADCZEMvsTDCT"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ADCZEMvsTDCT: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(250, -5.5, 24.5, 1051, -202.5, 4002.5);
  }
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZEM1", "ADC  ZEM1 vs TDC Time (ns) ZEM1", "TDCV", "ZEM1", "ADC", "ZEM1", 0);
  addNewHisto("ADCvsTDC", "h_ADC_TDC_ZEM2", "ADC  ZEM2 vs TDC Time (ns) ZEM2", "TDCV", "ZEM2", "ADC", "ZEM2", 0);

  if (auto param = mCustomParameters.find("TDCDIFF"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCDIFF: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(100, -10.5, 10.5, 100, -10.5, 10.5);
  }
  addNewHisto("TDC-DIFF", "h_TDC_ZNC_DIFF_ZNA_ZNC_SUM_ZNA_V", "TDC Time (ns) TDC ZNC + ZNA vs ZNC - ZNA", "TDCV", "ZNC-ZNA", "TDCV", "ZNC+ZNA", 0);
  addNewHisto("TDC-DIFF", "h_TDC_ZNC_DIFF_ZNA_ZNC_SUM_ZNA_V_cut", "TDC Time (ns) TDC ZNC + ZNA vs ZNC - ZNA with cut on ZEMs", "TDCV", "ZNC-ZNA", "TDCV", "ZNC+ZNA", 0);

  if (auto param = mCustomParameters.find("TDCAvsTDCT"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCAvsTDCT: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(250, -5.5, 24.5, 2000, -0.5, 3999.5);
  }
  addNewHisto("TDC_T_A", "h_TDC_ZNAC_V_A", "ZNA TC TDC amplitude vs time (ns)", "TDCV", "ZNAC", "TDCA", "ZNAC", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZPAC_V_A", "ZPA TC TDC amplitude vs time (ns)", "TDCV", "ZPAC", "TDCA", "ZPAC", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZNCC_V_A", "ZNC TC TDC amplitude vs time (ns)", "TDCV", "ZNCC", "TDCA", "ZNCC", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZPCC_V_A", "ZPC TC TDC amplitude vs time (ns)", "TDCV", "ZPCC", "TDCA", "ZPCC", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZNAS_V_A", "ZNA SUM TDC amplitude vs time (ns)", "TDCV", "ZNAS", "TDCA", "ZNAS", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZPAS_V_A", "ZPA SUM TDC amplitude vs time (ns)", "TDCV", "ZPAS", "TDCA", "ZPAS", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZNCS_V_A", "ZNC SUM TDC amplitude vs time (ns)", "TDCV", "ZNCS", "TDCA", "ZNCS", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZPCS_V_A", "ZPC SUM TDC amplitude vs time (ns)", "TDCV", "ZPCS", "TDCA", "ZPCS", 0);
  if (auto param = mCustomParameters.find("TDCAZEMvsTDCT"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCAZEMvsTDCT: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(250, -5.5, 24.5, 2000, -0.5, 3999.5);
  }
  addNewHisto("TDC_T_A", "h_TDC_ZEM1_V_A", "ZEM1 TDC amplitude vs time (ns)", "TDCV", "ZEM1", "TDCA", "ZEM1", 0);
  addNewHisto("TDC_T_A", "h_TDC_ZEM2_V_A", "ZEM2 TDC amplitude vs time (ns)", "TDCV", "ZEM2", "TDCA", "ZEM2", 0);
  if (auto param = mCustomParameters.find("TDCAvsTDCA"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCAvsTDCA: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(1000, -0.5, 3999.5, 1000, -0.5, 3999.5);
  }
  addNewHisto("TDC_A_A", "h_TDC_ZNA_ZPA", "ZNA TDC amplitude vs ZPA TDC amplitude", "TDCA", "ZPAC", "TDCA", "ZNAC", 0);
  addNewHisto("TDC_A_A", "h_TDC_ZNC_ZPC", "ZNC TDC amplitude vs ZPC TDC amplitude", "TDCA", "ZPCC", "TDCA", "ZNCC", 0);

  addNewHisto("TDC_A_A", "h_TDC_ZNAS_ZNAC", "TDC amplitude ZNA SUM vs TDC amplitude ZNA TC", "TDCA", "ZNAC", "TDCA", "ZNAS", 0);
  addNewHisto("TDC_A_A", "h_TDC_ZPAS_ZPAC", "TDC amplitude ZPA SUM vs TDC amplitude ZPA TC", "TDCA", "ZPAC", "TDCA", "ZPAS", 0);
  addNewHisto("TDC_A_A", "h_TDC_ZNCS_ZNCC", "TDC amplitude ZNC SUM vs TDC amplitude ZNC TC", "TDCA", "ZNCC", "TDCA", "ZNCS", 0);
  addNewHisto("TDC_A_A", "h_TDC_ZPCS_ZPCC", "TDC amplitude ZPC SUM vs TDC amplitude ZPC TC", "TDCA", "ZPCC", "TDCA", "ZPCS", 0);
  if (auto param = mCustomParameters.find("TDCAZEMvsTDCAZEM"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCAZEMvsTDCAZEM: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(1000, -0.5, 3999.5, 1000, -0.5, 3999.5);
  }
  addNewHisto("TDC_A_A", "h_TDC_ZEM1_ZEM2", "ZEM1 TDC amplitude vs ZEM2 TDC amplitude", "TDCA", "ZEM2", "TDCA", "ZEM1", 0);
  if (auto param = mCustomParameters.find("TDCAZEMvsTDCA"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TDCAZEMvsTDCA: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(1000, -0.5, 3999.5, 1000, -0.5, 3999.5);
  }
  addNewHisto("TDC_A_A", "h_TDC_ZNA_ZEM1", "ZNA TDC amplitude vs ZEM1 TDC amplitude", "TDCA", "ZEM1", "TDCA", "ZNAC", 0);
  addNewHisto("TDC_A_A", "h_TDC_ZNA_ZEM2", "ZNA TDC amplitude vs ZEM2 TDC amplitude", "TDCA", "ZEM2", "TDCA", "ZNAC", 0);
  addNewHisto("TDC_A_A", "h_TDC_ZNC_ZEM1", "ZNC TDC amplitude vs ZEM1 TDC amplitude", "TDCA", "ZEM1", "TDCA", "ZNCC", 0);
  addNewHisto("TDC_A_A", "h_TDC_ZNC_ZEM2", "ZNC TDC amplitude vs ZEM2 TDC amplitude", "TDCA", "ZEM2", "TDCA", "ZNCC", 0);

  addNewHisto("TDC_A_A", "h_TDC_ZPA_ZEM1", "ZPA TDC amplitude vs ZEM1 TDC amplitude", "TDCA", "ZEM1", "TDCA", "ZPAC", 0);
  addNewHisto("TDC_A_A", "h_TDC_ZPA_ZEM2", "ZPA TDC amplitude vs ZEM2 TDC amplitude", "TDCA", "ZEM2", "TDCA", "ZPAC", 0);
  addNewHisto("TDC_A_A", "h_TDC_ZPC_ZEM1", "ZPC TDC amplitude vs ZEM1 TDC amplitude", "TDCA", "ZEM1", "TDCA", "ZPCC", 0);
  addNewHisto("TDC_A_A", "h_TDC_ZPC_ZEM2", "ZPC TDC amplitude vs ZEM2 TDC amplitude", "TDCA", "ZEM2", "TDCA", "ZPCC", 0);

  // msg histo
  setBinHisto2D(26, -0.5, 26.0 - 0.5, 19, -0.5, 19.0 - 0.5);
  addNewHisto("MSG_REC", "h_msg", "Reconstruction messages", "INFO", "CH", "INFO", "MSG", 0);
  int idh_msg = (int)mHisto2D.size() - 1;
  mHisto2D.at(idh_msg).histo->SetStats(0);
  // Centroid ZNA
  if (auto param = mCustomParameters.find("CENTR_ZNA"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - CENTR_ZNA: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(200, -2, 2, 200, -2, 2);
  }
  addNewHisto("CENTR_ZNA", "h_CENTR_ZNA", "ZNA Centroid (cm)", "ADC", "CXZNA", "ADC", "CYZNA", 0);
  addNewHisto("CENTR_ZNA", "h_CENTR_ZNA_cut_ZEM", "ZNA Centroid (cm)", "ADC", "CXZNA", "ADC", "CYZNA", 0);

  // Centroid ZNC
  if (auto param = mCustomParameters.find("CENTR_ZNC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - CENTR_ZNC: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    setBinHisto2D(atoi(tokenString.at(0).c_str()), atof(tokenString.at(1).c_str()), atof(tokenString.at(2).c_str()), atoi(tokenString.at(3).c_str()), atof(tokenString.at(4).c_str()), atof(tokenString.at(5).c_str()));
  } else {
    setBinHisto2D(200, -2, 2, 200, -2, 2);
  }
  addNewHisto("CENTR_ZNC", "h_CENTR_ZNC", "ZNC Centroid (cm)", "ADC", "CXZNC", "ADC", "CYZNC", 0);
  addNewHisto("CENTR_ZNC", "h_CENTR_ZNC_cut_ZEM", "ZNC Centroid (cm)", "ADC", "CXZNC", "ADC", "CYZNC", 0);

  // Here we set the parameters for the configuration of the logic which selects the central events
  if (auto param = mCustomParameters.find("CENTRAL_EVENT_CONFIG"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - CENTRAL_EVENT_CONFIG: " << param->second << ENDM;
    tokenString = tokenLine(param->second, ";");
    SetConfigCentralEvent(atof(tokenString.at(0).c_str()), atoi(tokenString.at(1).c_str()));
  } else {
    SetConfigCentralEvent(0.0, 0);
  }
}

bool ZDCRecDataTask::add1DHisto(std::string typeH, std::string name, std::string title, std::string typeCh1, std::string ch1, int bin)
{

  TString hname = TString::Format("%s", name.c_str());
  TString htit = TString::Format("%s", title.c_str());
  mNameHisto.push_back(name);
  sHisto1D h1d;
  h1d.histo = new TH1F(hname, htit, fNumBinX, fMinBinX, fMaxBinX);
  h1d.typeh = typeH;
  h1d.typech = typeCh1;
  h1d.ch = ch1;
  h1d.bin = bin;
  int ih = (int)mHisto1D.size();
  mHisto1D.push_back(h1d);
  h1d.typeh.clear();
  h1d.typech.clear();
  h1d.ch.clear();
  // delete h1d.histo;
  if (ih < (int)mHisto1D.size()) {
    getObjectsManager()->startPublishing(mHisto1D.at(ih).histo);
    try {
      getObjectsManager()->addMetadata(mHisto1D.at(ih).histo->GetName(), mHisto1D.at(ih).histo->GetName(), "34");
      return true;
    } catch (...) {
      ILOG(Warning, Support) << "Metadata could not be added to " << mHisto1D.at(ih).histo->GetName() << ENDM;
      return false;
    }
  } else {
    return false;
  }
}

bool ZDCRecDataTask::add2DHisto(std::string typeH, std::string name, std::string title, std::string typeCh1, std::string ch1, std::string typeCh2, std::string ch2)
{
  TString hname = TString::Format("%s", name.c_str());
  TString htit = TString::Format("%s", title.c_str());
  mNameHisto.push_back(name);
  sHisto2D h2d;
  h2d.histo = new TH2F(hname, htit, fNumBinX, fMinBinX, fMaxBinX, fNumBinY, fMinBinY, fMaxBinY);
  h2d.typeh = typeH;
  h2d.typech1 = typeCh1;
  h2d.ch1 = ch1;
  h2d.typech2 = typeCh2;
  h2d.ch2 = ch2;
  int ih = (int)mHisto2D.size();
  mHisto2D.push_back(h2d);
  h2d.typeh.clear();
  h2d.typech1.clear();
  h2d.typech2.clear();
  h2d.ch1.clear();
  h2d.ch2.clear();
  h2d.typeh.clear();
  // delete h1d.histo;
  if (ih < (int)mHisto2D.size()) {
    getObjectsManager()->startPublishing(mHisto2D.at(ih).histo);
    try {
      getObjectsManager()->addMetadata(mHisto2D.at(ih).histo->GetName(), mHisto2D.at(ih).histo->GetName(), "34");
      return true;
    } catch (...) {
      ILOG(Warning, Support) << "Metadata could not be added to " << mHisto2D.at(ih).histo->GetName() << ENDM;
      return false;
    }
  } else
    return false;
}

bool ZDCRecDataTask::addNewHisto(std::string typeH, std::string name, std::string title, std::string typeCh1, std::string ch1, std::string typeCh2, std::string ch2, int bin)
{
  // Check if Histogram Exist
  if (std::find(mNameHisto.begin(), mNameHisto.end(), name) == mNameHisto.end()) {

    // ADC 1D (ENERGY) OR TDC 1D
    if (typeH == "ADC1D" || typeH == "TDC1D" || typeH == "CENTR_ZPA" || typeH == "CENTR_ZPC") {
      if (add1DHisto(typeH, name, title, typeCh1, ch1, bin)) {
        return true;
      } else {
        return false;
      }
    } else if (typeH == "ADCSUMvsTC" || typeH == "ADCvsTDC" || typeH == "TDC-DIFF" || typeH == "TDC_T_A" || typeH == "TDC_A_A" || typeH == "MSG_REC" || typeH == "CENTR_ZNA" || typeH == "CENTR_ZNC") {
      if (add2DHisto(typeH, name, title, typeCh1, ch1, typeCh2, ch2)) {
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  } else {
    reset();
    return true;
  }
  return false;
}

int ZDCRecDataTask::process(const gsl::span<const o2::zdc::BCRecData>& RecBC,
                            const gsl::span<const o2::zdc::ZDCEnergy>& Energy,
                            const gsl::span<const o2::zdc::ZDCTDCData>& TDCData,
                            const gsl::span<const uint16_t>& Info)
{
  LOG(info) << "o2::zdc::InterCalibEPN processing " << RecBC.size();
  float x, y;
  mEv.init(RecBC, Energy, TDCData, Info);
  while (mEv.next()) {
    // Histo 1D
    for (int i = 0; i < (int)mHisto1D.size(); i++) {
      // Fill ADC 1D
      if (mHisto1D.at(i).typeh == "ADC1D" && (mHisto1D.at(i).typech == "ADC" || mHisto1D.at(i).typech == "ADCAC")) {

        if (mHisto1D.at(i).typech == "ADC") {
          mHisto1D.at(i).histo->Fill(getADCRecValue(mHisto1D.at(i).typech, mHisto1D.at(i).ch));
        }

        if (mHisto1D.at(i).typech == "ADCAC") {
          if (mHisto1D.at(i).ch == "ZPAC" || mHisto1D.at(i).ch == "ZPAS") {
            if (mEv.NtdcA(o2::zdc::TDCZNAC) == 0 && mEv.NtdcA(o2::zdc::TDCZNAS) == 0) {
              if (getADCRecValue("ADC", mHisto1D.at(i).ch) > -8000) {
                mHisto1D.at(i).histo->Fill(getADCRecValue("ADC", mHisto1D.at(i).ch));
              }
            }
          }
          if (mHisto1D.at(i).ch == "ZPCC" || mHisto1D.at(i).ch == "ZPCS") {
            if (mEv.NtdcA(o2::zdc::TDCZNCC) == 0 && mEv.NtdcA(o2::zdc::TDCZNCS) == 0) {
              if (getADCRecValue("ADC", mHisto1D.at(i).ch) > -8000) {
                mHisto1D.at(i).histo->Fill(getADCRecValue("ADC", mHisto1D.at(i).ch));
              }
            }
          }
          if (mHisto1D.at(i).ch == "ZNAC" || mHisto1D.at(i).ch == "ZNAS") {
            if (mEv.NtdcA(o2::zdc::TDCZPAC) == 0 && mEv.NtdcA(o2::zdc::TDCZPAS) == 0) {
              if (getADCRecValue("ADC", mHisto1D.at(i).ch) > -8000) {
                mHisto1D.at(i).histo->Fill(getADCRecValue("ADC", mHisto1D.at(i).ch));
              }
            }
          }
          if (mHisto1D.at(i).ch == "ZNCC" || mHisto1D.at(i).ch == "ZNCS") {
            if (mEv.NtdcA(o2::zdc::TDCZPCC) == 0 && mEv.NtdcA(o2::zdc::TDCZPCS) == 0) {
              if (getADCRecValue("ADC", mHisto1D.at(i).ch) > -8000) {
                mHisto1D.at(i).histo->Fill(getADCRecValue("ADC", mHisto1D.at(i).ch));
              }
            }
          }
        }
      }

      // Fill TDC 1D
      if (mHisto1D.at(i).typeh == "TDC1D" && (mHisto1D.at(i).typech == "TDCV" || mHisto1D.at(i).typech == "TDCA")) {
        int tdcid = getIdTDCch(mHisto1D.at(i).typech, mHisto1D.at(i).ch);
        auto nhitv = mEv.NtdcV(tdcid);
        if (mEv.NtdcA(tdcid) == nhitv && nhitv > 0) {
          for (int ihit = 0; ihit < nhitv; ihit++) {
            if (mHisto1D.at(i).typech == "TDCV") {
              mHisto1D.at(i).histo->Fill(mEv.tdcV(tdcid, ihit));
            }
            if (mHisto1D.at(i).typech == "TDCA") {
              mHisto1D.at(i).histo->Fill(mEv.tdcA(tdcid, ihit));
            }
          }
        }
      }

      // Fill TDCA with cut 1D
      if (mHisto1D.at(i).typeh == "TDC1D" && (mHisto1D.at(i).typech == "TDCAC")) {
        int tdcid = getIdTDCch("TDCA", mHisto1D.at(i).ch);
        auto nhitv = mEv.NtdcV(tdcid);
        if (tdcid == o2::zdc::TDCZPAC || tdcid == o2::zdc::TDCZPAS) {
          if (mEv.NtdcA(o2::zdc::TDCZNAC) == 0 && mEv.NtdcA(o2::zdc::TDCZNAS) == 0) {
            if (mEv.NtdcA(tdcid) == nhitv && nhitv > 0) {
              for (int ihit = 0; ihit < nhitv; ihit++) {
                if (mHisto1D.at(i).typech == "TDCAC") {
                  if ((mEv.tdcV(tdcid, ihit) > -2.5 && mEv.tdcV(tdcid, ihit) < 2.5)) {
                    mHisto1D.at(i).histo->Fill(mEv.tdcA(tdcid, ihit));
                  }
                }
              }
            }
          }
        }
        if (tdcid == o2::zdc::TDCZPCC || tdcid == o2::zdc::TDCZPCS) {
          if (mEv.NtdcA(o2::zdc::TDCZNCC) == 0 && mEv.NtdcA(o2::zdc::TDCZNCS) == 0) {
            if (mEv.NtdcA(tdcid) == nhitv && nhitv > 0) {
              for (int ihit = 0; ihit < nhitv; ihit++) {
                if (mHisto1D.at(i).typech == "TDCAC") {
                  if ((mEv.tdcV(tdcid, ihit) > -2.5 && mEv.tdcV(tdcid, ihit) < 2.5)) {
                    mHisto1D.at(i).histo->Fill(mEv.tdcA(tdcid, ihit));
                  }
                }
              }
            }
          }
        }
        if (tdcid == o2::zdc::TDCZNAC || tdcid == o2::zdc::TDCZNAS) {
          if (mEv.NtdcA(o2::zdc::TDCZPAC) == 0 && mEv.NtdcA(o2::zdc::TDCZPAS) == 0) {
            if (mEv.NtdcA(tdcid) == nhitv && nhitv > 0) {
              for (int ihit = 0; ihit < nhitv; ihit++) {
                if (mHisto1D.at(i).typech == "TDCAC") {
                  if ((mEv.tdcV(tdcid, ihit) > -2.5 && mEv.tdcV(tdcid, ihit) < 2.5)) {
                    mHisto1D.at(i).histo->Fill(mEv.tdcA(tdcid, ihit));
                  }
                }
              }
            }
          }
        }
        if (tdcid == o2::zdc::TDCZNCC || tdcid == o2::zdc::TDCZNCS) {
          if (mEv.NtdcA(o2::zdc::TDCZPCC) == 0 && mEv.NtdcA(o2::zdc::TDCZPCS) == 0) {
            if (mEv.NtdcA(tdcid) == nhitv && nhitv > 0) {
              for (int ihit = 0; ihit < nhitv; ihit++) {
                if (mHisto1D.at(i).typech == "TDCAC") {
                  if ((mEv.tdcV(tdcid, ihit) > -2.5 && mEv.tdcV(tdcid, ihit) < 2.5)) {
                    mHisto1D.at(i).histo->Fill(mEv.tdcA(tdcid, ihit));
                  }
                }
              }
            }
          }
        }
      }

      // Fill CENTROID ZP
      if (mHisto1D.at(i).typeh == "CENTR_ZPA" && mHisto1D.at(i).typech == "ADC") {
        mHisto1D.at(i).histo->Fill(mEv.xZPA());
      }
      if (mHisto1D.at(i).typeh == "CENTR_ZPC" && mHisto1D.at(i).typech == "ADC") {
        mHisto1D.at(i).histo->Fill(mEv.xZPC());
      }
    } // for histo 1D

    // Histo 2D
    for (int i = 0; i < (int)mHisto2D.size(); i++) {
      if (mHisto2D.at(i).typeh == "ADCSUMvsTC" && mHisto2D.at(i).typech1 == "ADC" && mHisto2D.at(i).typech2 == "ADC") {
        mHisto2D.at(i).histo->Fill((Double_t)getADCRecValue(mHisto2D.at(i).typech1, mHisto2D.at(i).ch1), getADCRecValue(mHisto2D.at(i).typech2, mHisto2D.at(i).ch2));
      }
      if (mHisto2D.at(i).typeh == "ADCvsTDC" && mHisto2D.at(i).typech1 == "TDCV" && mHisto2D.at(i).typech2 == "ADC") {
        int tdcid = getIdTDCch(mHisto2D.at(i).typech1, mHisto2D.at(i).ch1);
        auto nhit = mEv.NtdcV(tdcid);
        if (mEv.NtdcA(tdcid) == nhit && nhit > 0) {
          mHisto2D.at(i).histo->Fill(mEv.tdcV(tdcid, 0), getADCRecValue(mHisto2D.at(i).typech2, mHisto2D.at(i).ch2));
        }
      }
      if (mHisto2D.at(i).typeh == "TDC-DIFF" && mHisto2D.at(i).typech1 == "TDCV" && mHisto2D.at(i).typech2 == "TDCV") {
        int zncc_id = getIdTDCch("TDCV", "ZNCC");
        int znac_id = getIdTDCch("TDCV", "ZNAC");
        auto nhit_zncc = mEv.NtdcV(zncc_id);
        auto nhit_znac = mEv.NtdcV(znac_id);
        if (mHisto2D.at(i).histo->GetName() == TString::Format("h_TDC_ZNC_DIFF_ZNA_ZNC_SUM_ZNA_V")) {
          if ((mEv.NtdcA(zncc_id) == nhit_zncc && nhit_zncc > 0) && (mEv.NtdcA(znac_id) == nhit_znac && nhit_znac > 0)) {
            auto sum = mEv.tdcV(zncc_id, 0) + mEv.tdcV(znac_id, 0);
            auto diff = mEv.tdcV(zncc_id, 0) - mEv.tdcV(znac_id, 0);
            mHisto2D.at(i).histo->Fill(diff, sum);
          }
        }
        if (mHisto2D.at(i).histo->GetName() == TString::Format("h_TDC_ZNC_DIFF_ZNA_ZNC_SUM_ZNA_V_cut")) {
          // if (( (float)o2::zdc::TDCZEM2 > -2.5 && (float)o2::zdc::TDCZEM2 < 2.5 ) && ( (float)o2::zdc::TDCZEM1 > -2.5 && (float)o2::zdc::TDCZEM1 < 2.5 ) ){
          if ((mEv.NtdcA(zncc_id) == nhit_zncc && nhit_zncc > 0) && (mEv.NtdcA(znac_id) == nhit_znac && nhit_znac > 0) && ((float)mEv.tdcV(5, 0) > -12.5 && (float)mEv.tdcV(5, 0) < 12.5) && ((float)mEv.tdcV(4, 0) > -12.5 && (float)mEv.tdcV(4, 0) < 12.5)) {
            auto sum = mEv.tdcV(zncc_id, 0) + mEv.tdcV(znac_id, 0);
            auto diff = mEv.tdcV(zncc_id, 0) - mEv.tdcV(znac_id, 0);
            mHisto2D.at(i).histo->Fill(diff, sum);
          }
        }
      }
      if (mHisto2D.at(i).typeh == "TDC_T_A" && mHisto2D.at(i).typech1 == "TDCV" && mHisto2D.at(i).typech2 == "TDCA") {
        int tdcid = getIdTDCch(mHisto2D.at(i).typech1, mHisto2D.at(i).ch1);
        auto nhitv = mEv.NtdcV(tdcid);
        if (mEv.NtdcA(tdcid) == nhitv && nhitv > 0) {
          for (int ihit = 0; ihit < nhitv; ihit++) {
            mHisto2D.at(i).histo->Fill(mEv.tdcV(tdcid, ihit), mEv.tdcA(tdcid, ihit));
          }
        }
      }
      if (mHisto2D.at(i).typeh == "TDC_A_A" && mHisto2D.at(i).typech1 == "TDCA" && mHisto2D.at(i).typech2 == "TDCA") {
        int tdcid1 = getIdTDCch(mHisto2D.at(i).typech1, mHisto2D.at(i).ch1);
        auto nhitv1 = mEv.NtdcV(tdcid1);
        int tdcid2 = getIdTDCch(mHisto2D.at(i).typech2, mHisto2D.at(i).ch2);
        auto nhitv2 = mEv.NtdcV(tdcid2);
        if ((mEv.NtdcA(tdcid1) == nhitv1 && nhitv1 > 0) && (mEv.NtdcA(tdcid2) == nhitv1 && nhitv2 > 0)) {
          mHisto2D.at(i).histo->Fill(mEv.tdcA(tdcid1, 0), mEv.tdcA(tdcid2, 0));
        }
      }

      if (mEv.getNInfo() > 0 && mHisto2D.at(i).typech1 == "INFO") {
        auto& decodedInfo = mEv.getDecodedInfo();
        for (uint16_t info : decodedInfo) {
          uint8_t ch = (info >> 10) & 0x1f;
          uint16_t code = info & 0x03ff;
          mHisto2D.at(i).histo->Fill(ch, code);
        }
      }
      if (mHisto2D.at(i).typeh == "CENTR_ZNA" && mHisto2D.at(i).typech1 == "ADC" && mHisto2D.at(i).typech2 == "ADC") {
        if (mHisto2D.at(i).histo->GetName() == TString::Format("h_CENTR_ZNA")) {
          mEv.centroidZNA(x, y);
          mHisto2D.at(i).histo->Fill(x, y);
        } else {
          if (IsEventCentral()) {
            mEv.centroidZNA(x, y);
            mHisto2D.at(i).histo->Fill(x, y);
          }
        }
      }
      if (mHisto2D.at(i).typeh == "CENTR_ZNC" && mHisto2D.at(i).typech1 == "ADC" && mHisto2D.at(i).typech2 == "ADC") {
        if (mHisto2D.at(i).histo->GetName() == TString::Format("h_CENTR_ZNC")) {
          mEv.centroidZNC(x, y);
          mHisto2D.at(i).histo->Fill(x, y);
        } else {
          if (IsEventCentral()) {
            mEv.centroidZNC(x, y);
            mHisto2D.at(i).histo->Fill(x, y);
          }
        }
      }
    }
  }
  return 0;
}

bool ZDCRecDataTask::IsEventCentral()
{
  if (fcentraleventconfigvalue == 1) {
    // Both ZEMs between a configurable value
    if (((float)mEv.tdcV(5, 0) > -ftdcLimit && (float)mEv.tdcV(5, 0) < ftdcLimit) && ((float)mEv.tdcV(4, 0) > -ftdcLimit && (float)mEv.tdcV(4, 0) < ftdcLimit)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

float ZDCRecDataTask::getADCRecValue(std::string typech, std::string ch)
{
  if (typech == "ADC" && ch == "ZNAC") {
    return mEv.EZNAC();
  }
  if (typech == "ADC" && ch == "ZNA1") {
    return mEv.EZNA1();
  }
  if (typech == "ADC" && ch == "ZNA2") {
    return mEv.EZNA2();
  }
  if (typech == "ADC" && ch == "ZNA3") {
    return mEv.EZNA3();
  }
  if (typech == "ADC" && ch == "ZNA4") {
    return mEv.EZNA4();
  }
  if (typech == "ADC" && ch == "ZNAS") {
    return mEv.EZNASum();
  }
  if (typech == "ADC" && ch == "ZPAC") {
    return mEv.EZPAC();
  }
  if (typech == "ADC" && ch == "ZPA1") {
    return mEv.EZPA1();
  }
  if (typech == "ADC" && ch == "ZPA2") {
    return mEv.EZPA2();
  }
  if (typech == "ADC" && ch == "ZPA3") {
    return mEv.EZPA3();
  }
  if (typech == "ADC" && ch == "ZPA4") {
    return mEv.EZPA4();
  }
  if (typech == "ADC" && ch == "ZPAS") {
    return mEv.EZPASum();
  }
  if (typech == "ADC" && ch == "ZNCC") {
    return mEv.EZNCC();
  }
  if (typech == "ADC" && ch == "ZNC1") {
    return mEv.EZNC1();
  }
  if (typech == "ADC" && ch == "ZNC2") {
    return mEv.EZNC2();
  }
  if (typech == "ADC" && ch == "ZNC3") {
    return mEv.EZNC3();
  }
  if (typech == "ADC" && ch == "ZNC4") {
    return mEv.EZNC4();
  }
  if (typech == "ADC" && ch == "ZNCS") {
    return mEv.EZNCSum();
  }
  if (typech == "ADC" && ch == "ZPCC") {
    return mEv.EZPCC();
  }
  if (typech == "ADC" && ch == "ZPC1") {
    return mEv.EZPC1();
  }
  if (typech == "ADC" && ch == "ZPC2") {
    return mEv.EZPC2();
  }
  if (typech == "ADC" && ch == "ZPC3") {
    return mEv.EZPC3();
  }
  if (typech == "ADC" && ch == "ZPC4") {
    return mEv.EZPC4();
  }
  if (typech == "ADC" && ch == "ZPCS") {
    return mEv.EZPCSum();
  }
  if (typech == "ADC" && ch == "ZEM1") {
    return mEv.EZEM1();
  }
  if (typech == "ADC" && ch == "ZEM2") {
    return mEv.EZEM2();
  }
  return -9000.0;
}

int ZDCRecDataTask::getIdTDCch(std::string typech, std::string ch)
{
  if ((typech == "TDCV" || typech == "TDCA") && ch == "ZNAC") {
    return o2::zdc::TDCZNAC;
  }
  if ((typech == "TDCV" || typech == "TDCA") && ch == "ZNAS") {
    return o2::zdc::TDCZNAS;
  }
  if ((typech == "TDCV" || typech == "TDCA") && ch == "ZPAC") {
    return o2::zdc::TDCZPAC;
  }
  if ((typech == "TDCV" || typech == "TDCA") && ch == "ZPAS") {
    return o2::zdc::TDCZPAS;
  }
  if ((typech == "TDCV" || typech == "TDCA") && ch == "ZNCC") {
    return o2::zdc::TDCZNCC;
  }
  if ((typech == "TDCV" || typech == "TDCA") && ch == "ZNCS") {
    return o2::zdc::TDCZNCS;
  }
  if ((typech == "TDCV" || typech == "TDCA") && ch == "ZPCC") {
    return o2::zdc::TDCZPCC;
  }
  if ((typech == "TDCV" || typech == "TDCA") && ch == "ZPCS") {
    return o2::zdc::TDCZPCS;
  }
  if ((typech == "TDCV" || typech == "TDCA") && ch == "ZEM1") {
    return o2::zdc::TDCZEM1;
  }
  if ((typech == "TDCV" || typech == "TDCA") && ch == "ZEM2") {
    return o2::zdc::TDCZEM2;
  }
  return 0;
}

std::vector<std::string> ZDCRecDataTask::tokenLine(std::string Line, std::string Delimiter)
{
  std::string token;
  size_t pos = 0;
  int i = 0;
  std::vector<std::string> stringToken;
  while ((pos = Line.find(Delimiter)) != std::string::npos) {
    token = Line.substr(i, pos);
    stringToken.push_back(token);
    Line.erase(0, pos + Delimiter.length());
  }
  stringToken.push_back(Line);
  return stringToken;
}

} // namespace o2::quality_control_modules::zdc
