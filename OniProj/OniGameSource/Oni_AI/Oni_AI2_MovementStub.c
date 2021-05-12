/*
	FILE:	Oni_AI2_MovementStub.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: July 29, 2000
	
	PURPOSE: Non-incarnated Movement AI for Oni
	
	Copyright (c) Bungie Software, 2000

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Timer.h"

#include "Oni_AI2.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cMovementStub_FindFloorFrames		30
#define AI2cMovementStub_StickToFloorHeight		8.0f

// ------------------------------------------------------------------------------------
// -- internal prototypes

void AI2iMovementStub_SetupPath(ONtCharacter *ioCharacter);

// ------------------------------------------------------------------------------------
// -- high-level control functions

void AI2rMovementStub_Initialize(ONtCharacter *ioCharacter, AI2tMovementState *inState)
{	
	ioCharacter->movementStub.cur_facing = ioCharacter->facing;
	ioCharacter->movementStub.cur_point = ioCharacter->location;

	if ((inState == NULL) || (inState->grid_num_points == 0)) {
		AI2rMovementStub_ClearPath(ioCharacter);
	} else {
		// take our current path from the input movement state
		ioCharacter->movementStub.path_start = ioCharacter->actual_position;
		ioCharacter->movementStub.path_end = inState->grid_path_end;

		AI2iMovementStub_SetupPath(ioCharacter);
	}

	if (ioCharacter->movementStub.path_active) {
		ioCharacter->movementStub.findfloor_timer = UUmRandomRange(1, AI2cMovementStub_FindFloorFrames);
	} else {
		ioCharacter->movementStub.findfloor_timer = 0;
	}
	ioCharacter->movementStub.cur_floor_gq = (UUtUns32) -1;
}

// ------------------------------------------------------------------------------------
// -- query functions

float AI2rMovementStub_GetMoveDirection(ONtCharacter *ioCharacter)
{
	if (ioCharacter->movementStub.path_active) {
		return ioCharacter->movementStub.cur_facing;
	} else {
		return AI2cAngle_None;
	}
}

// ------------------------------------------------------------------------------------
// -- command interface to high-level pathfinding system

// clear our pathfinding error information
void AI2rMovementStub_NewDestination(ONtCharacter *ioCharacter)
{
	// nothing
}

// reset our path
void AI2rMovementStub_ClearPath(ONtCharacter *ioCharacter)
{
	ioCharacter->movementStub.path_active = UUcFalse;
}

UUtBool AI2rMovementStub_AdvanceThroughGrid(ONtCharacter *ioCharacter)
{
	// check to see if we are actually still moving
	if ((ioCharacter->pathState.at_finalpoint) || (!ioCharacter->movementStub.path_active)) {
		// we aren't moving, we're done
		return UUcTrue;
	} else {
		// we are still moving
		return UUcFalse;
	}
}

// update for one game tick
void AI2rMovementStub_Update(ONtCharacter *ioCharacter)
{
	UUtBool has_weapon, twohanded_weapon;
	AI2tMovementMode movement_mode;
	AKtGQ_Collision *floor_gq;
	AKtEnvironment *environment;
	M3tPlaneEquation floor_plane;
	float floor_height;

	if (!ioCharacter->movementStub.path_active)
		return;

	has_weapon = (ioCharacter->inventory.weapons[0] != NULL);
	twohanded_weapon = (has_weapon && (WPrGetClass(ioCharacter->inventory.weapons[0])->flags & WPcWeaponClassFlag_TwoHanded));

	movement_mode = ioCharacter->movementOrders.movementMode;
	if (movement_mode == AI2cMovementMode_Default)
		movement_mode = AI2rMovement_Default(ioCharacter);
		
	ioCharacter->movementStub.cur_distance += ONrCharacterClass_MovementSpeed(ioCharacter->characterClass, AI2cMovementDirection_Forward,
																				movement_mode, has_weapon, twohanded_weapon);
	if (ioCharacter->movementStub.cur_distance > ioCharacter->movementStub.path_distance) {
		// we have hit the end of our path, snap to this XZ location
		ioCharacter->movementStub.cur_distance = ioCharacter->movementStub.path_distance;
		ioCharacter->movementStub.cur_point.x = ioCharacter->movementStub.path_end.x;
		ioCharacter->movementStub.cur_point.z = ioCharacter->movementStub.path_end.z;
		ioCharacter->movementStub.cur_floor_gq = ONrCharacter_FindFloorWithRay(ioCharacter, 0,
											&ioCharacter->movementStub.cur_point, &ioCharacter->movementStub.cur_point);

		ioCharacter->movementStub.path_active = UUcFalse;
//		COrConsole_Printf("hit end of path");
	} else {
		M3tVector3D delta_pos;
		M3tPoint3D new_position;

		// work out how far along the path we are now, and calculate whereabouts this places us
		MUmVector_Copy(new_position, ioCharacter->movementStub.path_start);
		MUmVector_Subtract(delta_pos, ioCharacter->movementStub.path_end, ioCharacter->movementStub.path_start);
		MUmVector_ScaleIncrement(new_position, ioCharacter->movementStub.cur_distance / ioCharacter->movementStub.path_distance, delta_pos);

		ioCharacter->movementStub.cur_point.x = new_position.x;
		ioCharacter->movementStub.cur_point.z = new_position.z;

		// check to see if we need to find our floor point
		if (ioCharacter->movementStub.findfloor_timer > 0) {
			ioCharacter->movementStub.findfloor_timer--;
		}

		if (ioCharacter->movementStub.findfloor_timer == 0) {
			// cast a ray to try and find the floor
			ioCharacter->movementStub.cur_floor_gq = ONrCharacter_FindFloorWithRay(ioCharacter, AI2cMovementStub_FindFloorFrames,
										&ioCharacter->movementStub.cur_point, &ioCharacter->movementStub.cur_point);
			ioCharacter->movementStub.findfloor_timer = AI2cMovementStub_FindFloorFrames;

		} else if (ioCharacter->movementStub.cur_floor_gq != (UUtUns32) -1) {
			// find the plane of the floor that we were last on
			environment = ONrGameState_GetEnvironment();
			floor_gq = environment->gqCollisionArray->gqCollision + ioCharacter->movementStub.cur_floor_gq;
			AKmPlaneEqu_GetComponents(floor_gq->planeEquIndex, environment->planeArray->planes,
										floor_plane.a, floor_plane.b, floor_plane.c, floor_plane.d);

			if ((float)fabs(floor_plane.b) > 1e-03f) {
				// continuously adjust our height to stay on this plane
				floor_height = - (floor_plane.a * ioCharacter->movementStub.cur_point.x +
								  floor_plane.c * ioCharacter->movementStub.cur_point.z + floor_plane.d) / floor_plane.b;

				if (ioCharacter->currentNode != NULL) {
					// ensure that we never go below the bottom of our BNV
					floor_height = UUmMax(floor_height, ioCharacter->currentNode->roomData.origin.y + 2.0f);
				}

				if ((ioCharacter->movementStub.cur_point.y < floor_height + AI2cMovementStub_StickToFloorHeight) && 
					(ioCharacter->movementStub.cur_point.y > floor_height - AI2cMovementStub_StickToFloorHeight)) {
					ioCharacter->movementStub.cur_point.y = floor_height;
				}
			}
		}

/*		COrConsole_Printf("dist %f of %f, position (%f %f %f)... path (%f %f %f) - (%f %f %f)",
					ioCharacter->movementStub.cur_distance, ioCharacter->movementStub.path_distance,
					ioCharacter->movementStub.cur_point.x, ioCharacter->movementStub.cur_point.y, ioCharacter->movementStub.cur_point.z, 
					ioCharacter->movementStub.path_start.x, ioCharacter->movementStub.path_start.y, ioCharacter->movementStub.path_start.z, 
					ioCharacter->movementStub.path_end.x, ioCharacter->movementStub.path_end.y, ioCharacter->movementStub.path_end.z);*/
	}
}

UUtBool AI2rMovementStub_DestinationFacing(ONtCharacter *ioCharacter)
{
	float facing_angle;

	if (ioCharacter->pathState.destinationType != AI2cDestinationType_Point)
		return UUcTrue;

	facing_angle = ioCharacter->pathState.destinationData.point_data.final_facing;
	if (facing_angle == AI2cAngle_None)
		return UUcTrue;

	// set up our destination facing
	ioCharacter->movementOrders.facingMode = AI2cFacingMode_FaceAtDestination;
	ioCharacter->movementOrders.facingData.faceAtDestination.angle = facing_angle;
	return UUcTrue;
}

UUtBool AI2rMovementStub_CheckFailure(ONtCharacter *ioCharacter)
{
	return UUcFalse;
}

void AI2iMovementStub_SetupPath(ONtCharacter *ioCharacter)
{
	// set up the path
	ioCharacter->movementStub.cur_facing = MUrATan2(ioCharacter->movementStub.path_end.x - ioCharacter->movementStub.path_start.x,
												ioCharacter->movementStub.path_end.z - ioCharacter->movementStub.path_start.z);
	UUmTrig_ClipLow(ioCharacter->movementStub.cur_facing);

	ioCharacter->movementStub.path_distance = MUmVector_GetDistance(ioCharacter->movementStub.path_start, ioCharacter->movementStub.path_end);
	ioCharacter->movementStub.cur_distance = 0;
	MUmVector_Copy(ioCharacter->movementStub.cur_point, ioCharacter->movementStub.path_start);
	ioCharacter->movementStub.path_active = UUcTrue;
}

UUtBool AI2rMovementStub_MakePath(ONtCharacter *ioCharacter)
{
	if (ioCharacter->pathState.path_num_nodes == 0) {
		AI2rMovementStub_ClearPath(ioCharacter);
		return UUcTrue;
	}

	// we start from the character's current position - however if our path start is a ghost waypoint then we can
	// trust its height to be on the surface of the correct floor, and we should use that.
	ioCharacter->movementStub.path_start = ioCharacter->actual_position;
	if (ioCharacter->pathState.path_current_node > 0) {
		ioCharacter->movementStub.path_start.y = PHrGetConnectionHeight(ioCharacter->pathState.path_connections[ioCharacter->pathState.path_current_node - 1]);
	}

	if (ioCharacter->pathState.path_current_node + 1 == ioCharacter->pathState.path_num_nodes) {
		// we are going to the destination
		ioCharacter->movementStub.path_end = ioCharacter->pathState.path_end_location;
	} else {
		// find the point on the exit ghost quad which we are going to
		AI2rPath_FindGhostWaypoint(ioCharacter, &ioCharacter->movementStub.path_start, ioCharacter->pathState.path_current_node, &ioCharacter->movementStub.path_end);
	}

	AI2iMovementStub_SetupPath(ioCharacter);
	return UUcTrue;
}

// ------------------------------------------------------------------------------------
// -- debugging routines

// report in
void AI2rMovementStub_Report(ONtCharacter *ioCharacter, UUtBool inVerbose, AI2tReportFunction inFunction)
{
	char reportbuf[256];

	if (ioCharacter->movementStub.path_active) {
		sprintf(reportbuf, "  path: (%f %f %f) - (%f %f %f) distance %f (current distance %f)",
				ioCharacter->movementStub.path_start.x, ioCharacter->movementStub.path_start.y, ioCharacter->movementStub.path_start.z,
				ioCharacter->movementStub.path_end.x, ioCharacter->movementStub.path_end.y, ioCharacter->movementStub.path_end.z,
				ioCharacter->movementStub.path_distance, ioCharacter->movementStub.cur_distance);
		inFunction(reportbuf);
	}

	sprintf(reportbuf, "  location: (%f %f %f) facing %f (deg)",
				ioCharacter->movementStub.cur_point.x, ioCharacter->movementStub.cur_point.y, ioCharacter->movementStub.cur_point.z,
				ioCharacter->movementStub.cur_facing);
	inFunction(reportbuf);
}

// draw the character's path
void AI2rMovementStub_RenderPath(ONtCharacter *ioCharacter)
{
	M3tPoint3D line_start, line_end;
	float sintheta, costheta;

	// draw an arrow at the character's location
	sintheta = MUrSin(ioCharacter->movementStub.cur_facing);
	costheta = MUrCos(ioCharacter->movementStub.cur_facing);
	MUmVector_Copy(line_start, ioCharacter->movementStub.cur_point);
	line_start.y += 2.0f;

	MUmVector_Copy(line_end, line_start);
	line_end.x += sintheta * 10.0f;
	line_end.z += costheta * 10.0f;
	M3rGeom_Line_Light(&line_start, &line_end, IMcShade_Yellow);

	line_start = line_end;
	line_start.x += sintheta * -4.0f + costheta * 2.0f;
	line_start.z += costheta * -4.0f - sintheta * 2.0f;
	M3rGeom_Line_Light(&line_start, &line_end, IMcShade_Yellow);

	line_start = line_end;
	line_start.x += sintheta * -4.0f - costheta * 2.0f;
	line_start.z += costheta * -4.0f + sintheta * 2.0f;
	M3rGeom_Line_Light(&line_start, &line_end, IMcShade_Yellow);
}

