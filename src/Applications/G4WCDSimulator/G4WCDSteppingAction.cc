// implementation of the G4WCDSteppingAction class
#include "G4WCDSimulator.h"
#include "G4WCDSteppingAction.h"
#include "G4WCDConstruction.h"
#include "G4WCDEventAction.h"
#include "G4WCDTrackingAction.h"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4RunManager.hh"

#include "G4OpticalPhoton.hh"

#include "SimData.h"
#include "Detector.h"

G4WCDSteppingAction::G4WCDSteppingAction(const G4WCDConstruction* det, G4WCDEventAction* G4event, Event& theEvent)
	: G4UserSteppingAction(),
		fDetectorConstruction(det),
		fEventAction(G4event),
    fEvent(theEvent)
{
}

G4WCDSteppingAction::~G4WCDSteppingAction()
{
}

void
G4WCDSteppingAction::UserSteppingAction(const G4Step* step)
{
	// NEW:v4 all that follows, Pacman implementation of Luigui

	if (!step) return;


	G4Track* track = step->GetTrack();
	if (!track) return;

	G4StepPoint* postPoint = step->GetPostStepPoint();
	if (!postPoint) return;

	G4ThreeVector pos = postPoint->GetPosition();

	// === LÍMITES DE LA ATMÓSFERA ===
	const G4double limitX = fDetectorConstruction->GetWorldSizeX() / 2;
	const G4double limitY = fDetectorConstruction->GetWorldSizeY() / 2;
	const G4double limitZ = fDetectorConstruction->GetWorldSizeZ() / 2;

	G4bool hitZ = std::abs(pos.z()) >= limitZ;

	G4bool exitingX = std::abs(pos.x()) >= limitX;
	G4bool exitingY = std::abs(pos.y()) >= limitY;

	// -------------------------------------------------------
	if ((exitingX || exitingY) && !hitZ) {
		// Corrección de posición: colocar la partícula ligeramente dentro del límite
		G4ThreeVector correctedPos = pos;

		// Periodicidad en X
		if (correctedPos.x() >= limitX)  correctedPos.setX(correctedPos.x() - 2.0 * limitX + 10.0*cm );
		if (correctedPos.x() <= -limitX)  correctedPos.setX(correctedPos.x() + 2.0 * limitX - 10.0*cm );
		// Periodicidad en Y
		if (correctedPos.y() >= limitY)  correctedPos.setY(correctedPos.y() - 2.0 * limitY + 10.0*cm );
		if (correctedPos.y() <= -limitY)  correctedPos.setY(correctedPos.y() + 2.0 * limitY - 10.0*cm );

		//postPoint->SetPosition(correctedPos);
		track->SetPosition(correctedPos);
		// Reubicar el navigator
		G4TransportationManager::GetTransportationManager()
			->GetNavigatorForTracking()
			->LocateGlobalPointAndSetup(correctedPos);

		//DEBUG: show all teletransportations
		G4cout << "[PBC] Before: " << pos << G4endl;
		G4cout << "[PBC] After:  " << correctedPos << G4endl;
	}


}
