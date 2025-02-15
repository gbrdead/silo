package org.voidland.concurrent.turning_grille;


public interface TurningGrilleCrackerImplDetails
{
	void bruteForce(TurningGrilleCracker cracker);
	
	default String milestone(TurningGrilleCracker cracker, long grillesPerSecond)
	{
		return "";
	}
	
	default String milestonesSummary(TurningGrilleCracker cracker)
	{
		return "";
	}
}
