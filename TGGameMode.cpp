#include "TGGameMode.h"
#include "EngineUtils.h"

 ATGGameMode::ATGGameMode()
{
	DefaultPawnClass = NULL;

}



 void ATGGameMode::ResetGoalLocations()
 {
     GoalLocations.Empty(); 
 }
