// implementation of the G4WCDRunAction class

#include "G4Timer.hh"
#include "G4Run.hh"
#include "g4root.hh"
#include "G4AccumulableManager.hh"
 
#include "G4WCDRunAction.h"
//#include "histosRun.hh"

//#include <TFile.h>

#include "G4RootAnalysisManager.hh"
#include "G4SystemOfUnits.hh"

namespace
{
    constexpr const char* kPhotonWavelength =
        "PhotonWavelengthAtPMT";

    constexpr const char* kPhotonOpticalPath =
        "PhotonOpticalPathAtPMT";

    constexpr const char* kPhotonWavelengthVsPath =
        "PhotonWavelengthVsPathAtPMT";

    constexpr const char* kPhotonArrivalTime =
        "PhotonArrivalTimeAtPMT";

    constexpr const char* kPEWavelength =
        "PEWavelength";

    constexpr const char* kPEOpticalPath =
        "PEOpticalPath";

    constexpr const char* kPEArrivalTime =
        "PEArrivalTime";
}

G4WCDRunAction::G4WCDRunAction()
 : G4UserRunAction()
{
  G4cout << "...G4WCDRunAction..." << G4endl;

  auto* analysisManager =
      G4RootAnalysisManager::Instance();

  // Keep this at 0 or 1 while developing.
  analysisManager->SetVerboseLevel(1);

  // -------------------------------------------------------------------------
  // Photon transport histograms
  // -------------------------------------------------------------------------

  analysisManager->CreateH1(
      kPhotonWavelength,
      "Wavelength of optical photons reaching the PMT",
      90,              // 5 nm bins
      250.0 CLHEP::nm,
      700.0 * CLHEP::nm,
      "nm"
  );

  analysisManager->CreateH1(
      kPhotonOpticalPath,
      "Optical path length of photons reaching the PMT",
      100,             // 0.5 m bins
      0.0 * CLHEP::m,
      50.0 * CLHEP::m,
      "m"
  );

  analysisManager->CreateH2(
      kPhotonWavelengthVsPath,
      "Photon wavelength vs optical path at PMT",
      90,
      250.0 * CLHEP::mm,
      700.0 * CLHEP::nm,
      100,
      0.0 * CLHEP::m,
      50.0 * CLHEP::m,
      "nm",
      "m"
  );

  analysisManager->CreateH1(
      kPhotonArrivalTime,
      "Arrival time of optical photons at the PMT",
      200,
      0.0 CLHEP::ns,
      1000.0 * CLHEP::ns,
      "ns"
  );

  // -------------------------------------------------------------------------
  // PE histograms
  //
  // These are filled only after the existing PMT quantum-efficiency
  // decision accepts the photon.
  // -------------------------------------------------------------------------

  analysisManager->CreateH1(
      kPEWavelength,
      "Wavelength of photons producing photoelectrons",
      90,
      250.0 * CLHEP::nm,
      700.0 * CLHEP::nm,
      "nm"
  );

  analysisManager->CreateH1(
      kPEOpticalPath,
      "Optical path length of detected photons",
      100,
      0.0 * CLHEP::m,
      50.0 * CLHEP::m,
      "m"
  );

  analysisManager->CreateH1(
      kPEArrivalTime,
      "Arrival time of detected photons",
      200,
      0.0 * CLHEP::ns,
      1000.0 * CLHEP::ns,
      "ns"
  );
}


G4WCDRunAction::~G4WCDRunAction()
{}


void 
G4WCDRunAction::BeginOfRunAction(const G4Run* )
{
  // Intentionally empty.
  //
  // Your application calls BeamOn(1) once for every primary particle.
  // Therefore BeginOfRunAction/EndOfRunAction refer to individual
  // BeamOn calls, not to the complete simulation batch.
}


void 
G4WCDRunAction::EndOfRunAction(const G4Run* )
{
  // Intentionally empty for the same reason.   
}

void
G4WCDRunAction::OpenOpticalAnalysis(const G4String& fileName)
{
    if (fOpticalFileOpen) {
        G4Exception(
            "G4WCDRunAction::OpenOpticalAnalysis",
            "Optical001",
            FatalException,
            "Optical analysis file is already open."
        );
    }

    auto* analysisManager =
        G4AnalysisManager::Instance();

    if (!analysisManager->OpenFile(fileName)) {
        G4Exception(
            "G4WCDRunAction::OpenOpticalAnalysis",
            "Optical002",
            FatalException,
            ("Could not open optical analysis file: " + fileName).c_str()
        );
    }

    fOpticalFileOpen = true;

    G4cout
        << "[INFO] Optical analysis output: "
        << fileName
        << G4endl;
}


void
G4WCDRunAction::WriteAndCloseOpticalAnalysis()
{
    if (!fOpticalFileOpen) {
        return;
    }

    auto* analysisManager =
        G4RootAnalysisManager::Instance();

    analysisManager->Write();
    analysisManager->CloseFile();

    fOpticalFileOpen = false;

    G4cout
        << "[INFO] Optical analysis file written and closed."
        << G4endl;
}