// implementation of the G4WCDSteppingAction class
#include "G4WCDSimulator.h"
#include "G4WCDSteppingAction.h"
#include "G4WCDConstruction.h"
#include "G4WCDEventAction.h"
#include "G4WCDTrackingAction.h"
#include "G4TransportationManager.hh"
#include "G4Navigator.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4RunManager.hh"

#include "G4OpticalPhoton.hh"

#include "SimData.h"
#include "Detector.h"

G4WCDSteppingAction::G4WCDSteppingAction(const G4WCDConstruction *det,
                                         G4WCDEventAction *G4event,
                                         Event &theEvent)
    : G4UserSteppingAction(), fDetectorConstruction(det), fEventAction(G4event),
      fEvent(theEvent), fOutputFile(nullptr), fSiPMTime(0.0), fScinTime(0.0) {
  // Obtener nombre del archivo desde Event
  Event::Config &cfg = fEvent.GetConfig();
  std::string outputFileName = cfg.fOutputFileName;

  // === CREAR EL ARCHIVO (tu nombre exacto) ===
  fOutputFile = new std::ofstream(outputFileName + "_beforeneutrons.shw",
                                  std::ios_base::app);

  // Opcional: encabezado
  if (fOutputFile && fOutputFile->tellp() == 0) {
    *fOutputFile << "# particle px[MeV/c] py[MeV/c] pz[MeV/c] x_pre[m] "
                    "y_pre[m] z_pre[m] preProcess x_pos[m] y_pos[m] z_pos[m] "
                    "postProcess E[MeV] trackID parentID myID\n";
  }
}

// ====================================================================
// DESTRUCTOR: CERRAR EL ARCHIVO, NEW:v8
// ====================================================================
G4WCDSteppingAction::~G4WCDSteppingAction() {
  if (fOutputFile) {
    if (fOutputFile->is_open()) {
      fOutputFile->close();
    }
    delete fOutputFile;
    fOutputFile = nullptr;
  }
}

// NEW:v8
class MyTrackInfo : public G4VUserTrackInformation {
public:
  MyTrackInfo(G4int id) : myID(id) {}
  virtual ~MyTrackInfo() {}

  void SetMyID(G4int id) { myID = id; }
  G4int GetMyID() const { return myID; }

private:
  G4int myID;
};

void G4WCDSteppingAction::UserSteppingAction(const G4Step *step) {
  // NEW:v4 all that follows, Pacman implementation of Luigui

  if (!step)
    return;

  G4Track *track = step->GetTrack();
  if (!track)
    return;

  // NEW:v8 save all info regardin a particle track
  // Seguridad: obtener puntos pre/post y comprobar
  G4StepPoint *postPoint = step->GetPostStepPoint();
  G4StepPoint *prePoint = step->GetPreStepPoint();
  if (!postPoint || !prePoint)
    return;

  G4String particleName = track->GetDefinition()
                              ? track->GetDefinition()->GetParticleName()
                              : "unknown";

  G4ThreeVector pos = postPoint->GetPosition();
  G4ThreeVector pre = prePoint->GetPosition();

  G4ThreeVector mom = track->GetMomentum();
  G4double E = track->GetKineticEnergy() / MeV;

  // === LÍMITES DE LA ATMÓSFERA ===
  const G4double limitX = fDetectorConstruction->GetWorldSizeX() / 2;
  const G4double limitY = fDetectorConstruction->GetWorldSizeY() / 2;
  const G4double limitZ = fDetectorConstruction->GetWorldSizeZ() / 2;

  G4bool hitZ = std::abs(pos.z()) >= limitZ;

  G4bool exitingX = std::abs(pos.x()) >= limitX;
  G4bool exitingY = std::abs(pos.y()) >= limitY;

  // NEW:v8 save track info
  // Asegurarnos de tener user info, si no crear una mínima
  MyTrackInfo *info = dynamic_cast<MyTrackInfo *>(track->GetUserInformation());
  if (!info) {
    // Si no hay info, crear y asociar para evitar nullptrs más adelante
    info = new MyTrackInfo(track->GetTrackID());
    track->SetUserInformation(info);
  }

  // Guardamos el myID si nos interesa escribirlo
  G4int myID = info ? info->GetMyID() : -1;

  // Solo trabajar con primarios (según tu lógica original)
  // if (track->GetParentID() == 0) {
  if (track->GetParentID() == 0 || particleName == "neutron" ||
      particleName == "proton") {
    // if (particleName == "neutron") {
    // if (particleName == "mu+" || particleName == "mu-") {
    //  PRE-PROCESS (seguro: comprobar puntero a proceso)
    G4String preProcess = "Start";
    if (prePoint->GetProcessDefinedStep() != nullptr) {
      preProcess = prePoint->GetProcessDefinedStep()->GetProcessName();
    }

    // POST-PROCESS (seguro)
    G4String postProcess = "Transportation";
    if ((std::abs(pos.x()) >= limitX && exitingX) ||
        (std::abs(pos.y()) >= limitY && exitingY)) {
      postProcess = "REFLECTION";
    } else {
      if (postPoint->GetProcessDefinedStep() != nullptr) {
        postProcess = postPoint->GetProcessDefinedStep()->GetProcessName();
      } else {
        postProcess = "Transportation";
      }
    }

    // Escribir línea en fichero (si está abierto)
    if (fOutputFile && fOutputFile->is_open()) {
      (*fOutputFile) << "PRIMARIA_TRACKING " << particleName << " "
                     << mom.x() / MeV << " " << mom.y() / MeV << " "
                     << mom.z() / MeV << " " << pre.x() / m << " "
                     << pre.y() / m << " " << pre.z() / m << " " << preProcess
                     << " " << pos.x() / m << " " << pos.y() / m << " "
                     << pos.z() / m << " " << postProcess << " " << E << " "
                     << track->GetTrackID() << " " << track->GetParentID()
                     << " " << myID << G4endl;
    }
  }

  // -------------------------------------------------------
  if ((exitingX || exitingY) && !hitZ) {
    // Corrección de posición: colocar la partícula ligeramente dentro del
    // límite
    G4ThreeVector correctedPos = pos;

    // Periodicidad en X
    if (correctedPos.x() >= limitX)
      correctedPos.setX(correctedPos.x() - 2.0 * limitX + 10.0 * cm);
    if (correctedPos.x() <= -limitX)
      correctedPos.setX(correctedPos.x() + 2.0 * limitX - 10.0 * cm);
    // Periodicidad en Y
    if (correctedPos.y() >= limitY)
      correctedPos.setY(correctedPos.y() - 2.0 * limitY + 10.0 * cm);
    if (correctedPos.y() <= -limitY)
      correctedPos.setY(correctedPos.y() + 2.0 * limitY - 10.0 * cm);

    // postPoint->SetPosition(correctedPos);
    track->SetPosition(correctedPos);
    // Reubicar el navigator
    G4TransportationManager::GetTransportationManager()
        ->GetNavigatorForTracking()
        ->LocateGlobalPointAndSetup(correctedPos);
  }
}
