/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once

#include "ComponentVisualizer.h"

/**
  * Visualization class for the arcade vehicle movement component.
*/
class FStaticArcadeVehicleMovementComponentVisualizer : public FComponentVisualizer
{
public:
	/** FComponentVisualizer interface. */
	void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	/** ~FComponentVisualizer interface. */
};