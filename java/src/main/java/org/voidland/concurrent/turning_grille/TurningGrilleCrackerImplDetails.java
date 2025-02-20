package org.voidland.concurrent.turning_grille;


public interface TurningGrilleCrackerImplDetails
{
	void bruteForce(TurningGrilleCracker cracker);
	
	void tryMilestone(TurningGrilleCracker cracker, long milestoneEnd, long grilleCountSoFar);
	
	default String milestonesSummary(TurningGrilleCracker cracker)
	{
		return "";
	}
}
