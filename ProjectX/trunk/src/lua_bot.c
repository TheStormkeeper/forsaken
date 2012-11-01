#ifdef LUA_BOT
#include "botai_main.h"
#include "botai_path.h"

SHIPCONTROL bot;
VECTOR FollowTargetPos;
VECTOR MissileOrigDirVector;
bool AllSensorsClear;
bool DEBUG_AVOIDANCE = true;
bool FriendlyFire = false;
bool MissileAvoidanceSet = false;
bool ReverseNetwork = true;
bool initialised = false;
float DistWall[NUM_SENSORS];
float IFiredTitan = -1.0F;
float PrevHull = MAX_HULL+1.0F;
float PrevShield = MAX_SHIELD+1.0F;
float TargetShipDistance;
float WhenWillHitSlide[NUM_SENSORS];
float accuracy = 0.0F;
int AvoidMove = -1;
int CurrentNode = -1;
int FollowTargetGroup;
int GettingPickup = -1;
int HomingMissile = -1;
int PrevDeaths = -1;
int PrevHomingMissile = -1;
int PrevKills = -1;
int TargetMine = -1;
int TargetNode = -1;
int TargetShipHealth = -1;
int TargetShipID = -1;

bool BOTAI_AimAtTarget( MATRIX * InvMat, VECTOR * SPos, VECTOR * TPos );
bool BOTAI_CanICollectPickup( u_int16_t i );
bool BOTAI_CheckForGravgons( VECTOR * Pos );
bool BOTAI_ClearLOS( VECTOR * SPos, u_int16_t Group , VECTOR * Pos );
bool BOTAI_ClearLOSNonZero( OBJECT * SObject, VECTOR * Pos, float radius );
bool BOTAI_CollectNearestPickup(bool SlideOnly);
bool BOTAI_FireTitan();
bool BOTAI_FriendlyFireCheck();
bool BOTAI_InViewCone( VECTOR * Pos, MATRIX * Mat, VECTOR * TPos, float ViewConeCos );
bool BOTAI_MaintainDistanceToTargetShip();
bool BOTAI_MoveToTarget(VECTOR * TPos);
bool BOTAI_SafeToMove();
bool BOTAI_SetAction( float * action, float value, char * str );
bool BOTAI_ShootEnemyMines();
bool BOTAI_SlideToTarget(VECTOR * TPos);
bool BOTAI_WillHomingMissileHit(VECTOR * MyPos);
float BOTAI_DistanceToWall( bool u, bool d, bool l, bool r, bool f, bool b );
float BOTAI_WhenWillBulletHitMe(VECTOR * MyPos);
float BOTAI_WhenWillBulletHitSlide( bool u, bool d, bool l, bool r, bool f, bool b );
int BOTAI_CenterCheck( int move );
void BOTAI_AimAtTargetShip();
void BOTAI_AvoidBullets();
void BOTAI_AvoidHomingMissiles();
void BOTAI_ClearActions();
void BOTAI_FireMissiles();
void BOTAI_GetNearestPickup();
void BOTAI_GetSHIPTarget();
void BOTAI_LookAhead( float Accuracy, VECTOR * SPos, BYTE TargetID, VECTOR * NewPos, float SpeedOfBullet );
void BOTAI_LookAheadPos( float Time, VECTOR * NewPos, VECTOR SlideDirection );
void BOTAI_SelectWeapon( bool MineTarget );
void BOTAI_ShootAtTargetShip();
void BOTAI_UpdateSensors();
void ProcessBot1();
void control_bot();
int BOTAI_ComparativeEnemyStrength();
int BOTAI_ShipStrength( int i );

char	*	WepMessages[] = {

	LT_PICKUP_TROJAX				,//"Trojax",						// Primary Weapons
	LT_PICKUP_PYROLITE				,//"Pyrolite Rifle",
	LT_PICKUP_TRANSPULSE			,//"Transpulse Cannon",
	LT_PICKUP_SUSSGUN				,//"Suss-Gun",
	LT_PICKUP_LASER					,//"Beam Laser",
	LT_PICKUP_MUG					,//"Mug Missile",					// Secondary Weapons
	LT_PICKUP_MUG_PACK				,//"Mug Missiles",
	LT_PICKUP_SOLARIS				,//"Heatseeker Missile",
	LT_PICKUP_SOLARIS_PACK			,//"Heatseeker Missile Pack",
	LT_PICKUP_THIEF					,//"Thief Missile",
	LT_PICKUP_SCATTER				,//"Scatter Missile",
	LT_PICKUP_GRAVGON				,//"Gravgon Missile",
	LT_PICKUP_MFRL					,//"Rocket Launcher",
	LT_PICKUP_TITAN					,//"TitanStar Missile",
	LT_PICKUP_PURGE					,//"Purge Mine Pack",
	LT_PICKUP_PINE					,//"Pine Mine Pack",
	LT_PICKUP_QUANTUM				,//"Quantum Mine Pack",
	LT_PICKUP_SPIDER				,//"Spider Mine Pack",
	LT_PICKUP_PARASITE				,//"Parasite Mine",
	LT_PICKUP_FLARE					,//"Flare",
	LT_PICKUP_GENAMMO				,//"General Ammo",					// Ammo
	LT_PICKUP_PYROFUEL				,//"Pyrolite Fuel",
	LT_PICKUP_SUSSAMMO				,//"Suss-Gun Ammo",
	LT_PICKUP_POWER_POD				,//"Power Pod",					// Extras
	LT_PICKUP_SHIELD				,//"Shield",
	LT_PICKUP_INVULBERABILITY		,//"Invulnerability",
	LT_PICKUP_EXTRA_LIFE			,//"Extra Life",
	LT_PICKUP_TARGETING_COMPUTER	,//"Targeting Computer",
	LT_PICKUP_SMOKE					,//"Smoke Streamer",
	LT_PICKUP_NITRO					,//"Nitro",
	LT_PICKUP_GOGGLES				,//"Goggles",
	LT_PICKUP_GOLD_BARS				,//"Gold Bars",
	LT_PICKUP_STEALTH				,//"Stealth Mantle",
	LT_PICKUP_CRYSTAL				,//"Crystal",
	LT_PICKUP_ORBITAL				,//"Orbit Pulsar",
	LT_PICKUP_GOLDEN_POWER_POD		,//"Golden Power Pod",
	LT_PICKUP_DNA					,//"DNA",
	"",														// skeleton key
	LT_PICKUP_BOMB					,//"Bomb",
	LT_PICKUP_GOLDEN_IDOL			,//"Golden Idol",
	"",								// flag
	"",
	"",								// flag 1
	"",								// flag 2
	"",								// flag 3
	"",								// flag 4
};

lua_State *Lbot;

#define PASS_IF_SET(prop) \
	if(bot.prop) control.prop = bot.prop

// game calls this each frame
void ProcessBot1()
{
	ZERO_STACK_MEM(bot);
	control_bot();
	if ( bot.yaw )
	{
		control.yaw  -= -bot.yaw * TurnAccell * MaxTurnSpeed * framelag;
		control.bank += -bot.yaw * BankAccell * MaxBankAngle * framelag;
	}
	if ( bot.pitch )
		control.pitch -= -bot.pitch * TurnAccell * MaxTurnSpeed * framelag;
	if ( bot.roll )
		control.roll += -bot.roll * RollAccell * MaxRollSpeed * framelag;
	if ( bot.right )
		control.right -= -bot.right * MoveAccell * MaxMoveSpeed * framelag;
	if ( bot.up )
		control.up -= -bot.up * MoveAccell * MaxMoveSpeed * framelag;
	control.turbo = bot.turbo;
	if( bot.turbo )
	{
		if ( NitroFuel > 0.0F )
			control.forward += TurboAccell * MaxTurboSpeed * framelag;
		else
			control.forward += MoveAccell * MaxMoveSpeed * framelag;
	}
	else if ( bot.forward )
		control.forward += bot.forward * MoveAccell * MaxMoveSpeed * framelag;

	PASS_IF_SET(select_primary);
	PASS_IF_SET(select_secondary);
	PASS_IF_SET(turbo);
	PASS_IF_SET(fire_primary);
	PASS_IF_SET(fire_secondary);
	PASS_IF_SET(fire_mine);
	PASS_IF_SET(drop_primary);
	PASS_IF_SET(drop_secondary);
	PASS_IF_SET(drop_shield);
	PASS_IF_SET(drop_ammo);
}

void control_bot()
{
	BOTAI_ClearActions();
	BOTAI_UpdateSensors();
	//BOTAI_AvoidTitans(); // not working properly
	//	BOTAI_AvoidHomingMissiles(); // currently dealt with in AvoidBullets()
	BOTAI_AvoidBullets();
	BOTAI_SelectWeapon( false );

	if( BOTAI_SafeToMove() )
	{
		if( BOTAI_FireTitan() )
			return;
	}

	BOTAI_AimAtTargetShip();
	BOTAI_ShootAtTargetShip();

	if( !BOTAI_SafeToMove() )
	{
		DebugPrintf("not safe to move\n");
		return;
	}

	if( BOTAI_ShootEnemyMines() )
		return;

	// if there is a ship target
	if( TargetShipID > -1 )
	{
		// try and collect any nearby pickups (sliding only so we keep aim on target)
		if(	BOTAI_CollectNearestPickup(true) )
			return;

		// maintain distance if i can see the target ship - replace later with dogfight etc.
		if( BOTAI_MaintainDistanceToTargetShip() )	
			return;

		// i have a target but i can't see it and it's safe to move so lets try and get a line of sight
		FollowTargetPos = Ships[TargetShipID].Object.Pos;
		FollowTargetGroup = Ships[TargetShipID].Object.Group;
		BOTAI_FollowNodeNetwork(true, true);
	}   

	// no ship targets and it's safe to move
	else
	{
		// collect any nearby pickups
		if(	BOTAI_CollectNearestPickup(false) )
			return;

		// no nearby pickups so roam around
		DebugPrintf("following node network...\n");
		BOTAI_FollowNodeNetwork(false, false);
	}
}

void BOTAI_ClearActions()
{
	bot.forward = 0.0F;
	bot.up = 0.0F;
	bot.right = 0.0F;
	bot.pitch = 0.0F;
	bot.yaw = 0.0F;
	bot.roll = 0.0F;
	bot.turbo = 0.0F;
	bot.fire_primary = 0.0F;
	bot.fire_secondary = 0.0F;
}

bool BOTAI_SetAction( float * action, float value, char * str )
{
	// enforce bounds
	if( value < -1.0F )
		value = -1.0F;
	else if( value > 1.0F )
		value = 1.0F;

	// try and set action
	if( *action != 0.0F )
	{
		DebugPrintf("%s already set\n", str);
		return false;
	}
	else
	{
		DebugPrintf("%s set\n", str);
		*action = value;
		return true;
	}
}

float BOTAI_WhenWillBulletHitMe(VECTOR * MyPos)
{
	int i;
	float time;
	float shortestTime = BIGDISTANCE;
	float dist;
	VECTOR TempVect;
	VECTOR temp;
	VECTOR TempVector;
	float ShipRadius;
	float Cos;

	// primary weapon bullets
	for(i = 0; i < MAXPRIMARYWEAPONBULLETS; i++)
	{
		// that are active and aren't my own
		if(PrimBulls[i].Used && PrimBulls[i].Owner != WhoIAm)
		{
			// set the collision radius
			switch( PrimaryWeaponAttribs[ PrimBulls[i].Weapon ].ColType )
			{
				case COLTYPE_Transpulse:
					TempVector.x = Ships[ WhoIAm ].Object.Pos.x - PrimBulls[i].Pos.x;
					TempVector.y = Ships[ WhoIAm ].Object.Pos.y - PrimBulls[i].Pos.y;
					TempVector.z = Ships[ WhoIAm ].Object.Pos.z - PrimBulls[i].Pos.z;
					NormaliseVector( &TempVector );
					Cos = (float) ( 1.0F - fabs( DotProduct( &TempVector, &PrimBulls[i].Dir ) ) );
					Cos = (float) ( Cos * ( 1.0F - fabs( DotProduct( &TempVector, &PrimBulls[i].UpVector ) ) ) );
					ShipRadius = SHIP_RADIUS + ( PrimBulls[i].ColRadius * Cos );
					break;

				case COLTYPE_Sphere:
				case COLTYPE_Trojax:
					ShipRadius = SHIP_RADIUS + PrimBulls[i].ColRadius;
					break;

				case COLTYPE_Point:
				default:
					ShipRadius = SHIP_RADIUS;
					break;
			}

			// bullet will (currently) eventually collide with this position
			if(RaytoSphere2( MyPos, ShipRadius, &PrimBulls[i].Pos, &PrimBulls[i].Dir, &TempVect, &TempVect ))
			{
				dist = DistanceVector2Vector(MyPos, &PrimBulls[i].Pos);
				time = dist/PrimBulls[i].Speed;
				if(time < shortestTime)
					shortestTime = time;
			}
		}
	}

	// reset homing missile flag when it's gone
	if(HomingMissile > -1)
	{
		if(!SecBulls[HomingMissile].Used)
			MissileAvoidanceSet = false;
	}

	// missiles
	HomingMissile = -1;
	for(i = 0; i< MAXSECONDARYWEAPONBULLETS; i++)
	{
		// ignore inactive missiles or my own
		if(!SecBulls[i].Used || SecBulls[i].Owner == WhoIAm)
			continue;

		// get the distance from me and time to impact
		dist = DistanceVector2Vector(MyPos, &SecBulls[i].Pos);
		time = dist/SecBulls[i].Speed;

		// straight missiles (treat like normal bullets)
		if(SecBulls[i].MoveType == MISMOVE_STRAIGHT )
		{
			// if it will hit me 
			if(RaytoSphere2(MyPos, SHIP_RADIUS, &SecBulls[i].Pos, &SecBulls[i].DirVector, &TempVect, &TempVect))
			{
				if(time < shortestTime)
					shortestTime = time; // find the closest
			}
		}
		// if a homing missile has locked on to me
		else if(SecBulls[i].MoveType == MISMOVE_HOMING && SecBulls[i].Target == WhoIAm)
		{
			// and it's close enough i have to deal with it
			if(dist < 2048.0F * GLOBAL_SCALE)
			{
				HomingMissile = i;
				shortestTime = time; // new
			}
		}
	}

	if(shortestTime < 10000.0F)
		return shortestTime;
	else
		return -1.0F; // nothing will hit
}

float BOTAI_WhenWillBulletHitSlide( bool u, bool d, bool l, bool r, bool f, bool b )
{
	VECTOR NewPos = Ships[WhoIAm].Object.Pos;
	float BulletHitTime;
	float TimeAhead = 11.0F;

	// calculate the new position
	if(u) BOTAI_LookAheadPos( TimeAhead, &NewPos, SlideUp );
	else if(d) BOTAI_LookAheadPos( TimeAhead, &NewPos, SlideDown );
	if(l) BOTAI_LookAheadPos( TimeAhead, &NewPos, SlideLeft );
	else if(r) BOTAI_LookAheadPos( TimeAhead, &NewPos, SlideRight );
	if(f) BOTAI_LookAheadPos( TimeAhead, &NewPos, Forward );
	else if(b) BOTAI_LookAheadPos( TimeAhead, &NewPos, Backward );

	// get time nearest bullet will hit new position
	BulletHitTime = BOTAI_WhenWillBulletHitMe(&NewPos);

	if(BulletHitTime > 0.0F)
		AllSensorsClear = false;

	return BulletHitTime;
}

bool BOTAI_FireTitan()
{
	int i;
	int wall;
	float dist;
	float closest = BIGDISTANCE;
	VECTOR TempVector;
	VECTOR Move_Dir;
	VECTOR ColPnt;
	u_int16_t TmpGroup;

	// no target to fire at
	if( TargetShipID < 0 )
		return false;

	// do we have the titan? 
	if( TITANSTARMISSILE != Ships[WhoIAm].Secondary && SecondaryAmmo[TITANSTARMISSILE] <= 0)
		return false;
	else
		Ships[WhoIAm].Secondary = TITANSTARMISSILE;

	// too close to target or too far away
	dist = DistanceVector2Vector(&Ships[WhoIAm].Object.Pos, &Ships[TargetShipID].Object.Pos);
	if(dist < 500.0F || dist > 2000.0F)
	{
		//DebugPrintf("too close or too far to target to fire titan\n");
		return false;
	}

	// for each r/l/u/d wall sensor
	for(i = 0; i < 4; i++)
	{
		// identify the closest wall
		if(DistWall[i] < closest)
		{
			closest = DistWall[i];
			wall = i;
		}
	}

	// slide to the wall
	if(closest > 150.0F)
	{
		switch(wall)
		{
			case 0:
				BOTAI_SetAction( &bot.right, 1.0F, "FireTitan() right" );
				break;
			case 1:
				BOTAI_SetAction( &bot.right, -1.0F, "FireTitan() left" );
				break;
			case 2:
				BOTAI_SetAction( &bot.up, 1.0F, "FireTitan() up" );
				break;
			case 3:
				BOTAI_SetAction( &bot.up, -1.0F, "FireTitan() down" );
				break;
		}
	}
	// aim towards wall
	else
	{
		// calculate collision point
		ApplyMatrix( &Ships[WhoIAm].Object.FinalMat, &Forward, &Move_Dir );
		Move_Dir.x *= MaxColDistance;
		Move_Dir.y *= MaxColDistance;
		Move_Dir.z *= MaxColDistance;
		BackgroundCollide( &MCloadheadert0, &Mloadheader, &Ships[WhoIAm].Object.Pos, Ships[WhoIAm].Object.Group, &Move_Dir, (VECTOR *) &ColPnt, &TmpGroup, &TempVector, &TempVector, false, NULL ); 

		// distance between target and collision point should be less than distance between me and target
		if( DistanceVector2Vector(&Ships[TargetShipID].Object.Pos, &ColPnt) > TargetShipDistance 
				// and target needs to be reachable by the collision point
				|| ( !BOTAI_ClearLOS(&Ships[TargetShipID].Object.Pos, Ships[TargetShipID].Object.Group, &ColPnt) )
				// and i have line of sight to the target
				|| ( !BOTAI_ClearLOS(&Ships[WhoIAm].Object.Pos, Ships[WhoIAm].Object.Group, &Ships[TargetShipID].Object.Pos) ) )
		{
			//DebugPrintf("aiming at target\n");
			BOTAI_AimAtTarget( &Ships[WhoIAm].Object.FinalInvMat, &Ships[WhoIAm].Object.Pos, &Ships[TargetShipID].Object.Pos);
		}
		// can see target so aim front sensor towards wall
		else if(DistWall[8] > 500.0F)
		{	
			//DebugPrintf("aiming toward wall\n");
			switch(wall)
			{
				case 0:
					BOTAI_SetAction( &bot.yaw, 1.0F, "FireTitan() yaw right" );
					break;
				case 1:
					BOTAI_SetAction( &bot.yaw, -1.0F, "FireTitan() yaw left" );
					break;
				case 2:
					BOTAI_SetAction( &bot.pitch, -1.0F, "FireTitan() pitch up" );
					break;
				case 3:
					BOTAI_SetAction( &bot.pitch, 1.0F, "FireTitan() pitch down" );
					break;
			}
			// helps to break alternating between too close and too far
			BOTAI_SetAction( &bot.forward, -1.0F, "FireTitan() reverse" );

		}
		// can see target but i'm too close to wall
		else if(DistWall[8] < 400.0F)
		{	
			//DebugPrintf("aiming away from wall\n");
			switch(wall)
			{
				case 0:
					BOTAI_SetAction( &bot.yaw, -1.0F, "FireTitan() yaw left" );
					break;
				case 1:
					BOTAI_SetAction( &bot.yaw, 1.0F, "FireTitan() yaw right" );
					break;
				case 2:
					BOTAI_SetAction( &bot.pitch, 1.0F, "FireTitan() pitch down" );
					break;
				case 3:
					BOTAI_SetAction( &bot.pitch, -1.0F, "FireTitan() pitch up" );
					break;
			}
			// helps to break alternating between too close and too far
			BOTAI_SetAction( &bot.forward, -1.0F, "FireTitan() reverse" );
		}
		// just right
		else
		{
			//DebugPrintf("chocs away old boy!\n");
			Ships[WhoIAm].Secondary = TITANSTARMISSILE;
			BOTAI_SetAction( &bot.fire_secondary, 1.0F, "FireTitan() fire secondary" );
			BOTAI_SetAction( &bot.forward, -1.0F, "FireTitan() reverse" );
			BOTAI_SetAction( &bot.right, -1.0F, "FireTitan() left" );
			IFiredTitan = 180.0F;
		}
	}

	return true;
}

void BOTAI_FireMissiles()
{
	// don't fire missiles if the target isn't in front of me and don't shoot directly into a wall
	//if(!BOTAI_InViewCone( &Ships[WhoIAm].Object.Pos, &Ships[WhoIAm].Object.FinalMat, &Ships[TargetShipID].Object.Pos, (float) COSD( 45.0F ) ) && DistWall[8] < 200.0F)
	//		return;


	// don't fire missiles unless we have a line of sight to the target
	if( TargetShipID < 0 )
		return;
	if( !BOTAI_ClearLOS(&Ships[WhoIAm].Object.Pos, Ships[WhoIAm].Object.Group, &Ships[TargetShipID].Object.Pos))
		return;

	// set order of preference
	if(SCATTERMISSILE == Ships[WhoIAm].Secondary || SecondaryAmmo[SCATTERMISSILE] > 0)
		Ships[WhoIAm].Secondary = SCATTERMISSILE;
	else if(SOLARISMISSILE == Ships[WhoIAm].Secondary || SecondaryAmmo[SOLARISMISSILE] > 0)
		Ships[WhoIAm].Secondary = SOLARISMISSILE;
	else if(MULTIPLEMISSILE == Ships[WhoIAm].Secondary || SecondaryAmmo[MULTIPLEMISSILE] > 0)
		Ships[WhoIAm].Secondary = MULTIPLEMISSILE;
	else if(MUGMISSILE == Ships[WhoIAm].Secondary || SecondaryAmmo[MUGMISSILE] > 0)
		Ships[WhoIAm].Secondary = MUGMISSILE;

	// fire missiles, but don't fire titan here
	if(TITANSTARMISSILE != Ships[WhoIAm].Secondary)
		BOTAI_SetAction( &bot.fire_secondary, 1.0F, "FireMissiles() fire secondary" );
}

#define TOL 50.0F
/* triple-chords to target pos in a straight line; returns true if reached target */
bool BOTAI_MoveToTargetNew(VECTOR * TPos)
{
	VECTOR Slide;
	VECTOR TriChordVector;
	VECTOR WantedVector;
	VECTOR NormVector;
	VECTOR DirVector;
	VECTOR FwdDirVector;
	float xAngle;
	float yAngle;
	float Cos;

	////DebugPrintf("moving to %f %f %f\n", TPos->x, TPos->y, TPos->z);

	// -- calculate direction vector from my position to target 
	NormVector.x = TPos->x - Ships[WhoIAm].Object.Pos.x;
	NormVector.y = TPos->y - Ships[WhoIAm].Object.Pos.y;
	NormVector.z = TPos->z - Ships[WhoIAm].Object.Pos.z;

	DirVector = NormVector;

	// apply that vector to my matrix to make direction relative to my direction and rotation 
	//ApplyMatrix( &Ships[WhoIAm].Object.FinalInvMat, &NormVector, &DirVector );
	NormaliseVector( &DirVector );
	//DebugPrintf("straight x = %f y = %f z = %f\n", DirVector.x, DirVector.y, DirVector.z);

	// calculate the desired vector
	// forward right
	Slide.x = 1.0F;
	Slide.y = 0.0F;
	Slide.z = 1.0F;


	// calculate the triple chord direction vector 
	ApplyMatrix( &Ships[WhoIAm].Object.FinalMat, &Slide, &TriChordVector );
	NormaliseVector( &TriChordVector );
	//DebugPrintf("trivec x = %f y = %f z = %f\n", TriChordVector.x, TriChordVector.y, TriChordVector.z);


	// difference between wanted vector and slide vector
	WantedVector.x = DirVector.x - TriChordVector.x;
	WantedVector.y = DirVector.y - TriChordVector.y;
	WantedVector.z = DirVector.z - TriChordVector.z;
	////DebugPrintf("W x = %f y = %f z = %f\n", WantedVector.x, WantedVector.y, WantedVector.z);

	// angle between trichord vector and wanted vector, == 1.0F when aligned perfectly
	Cos = DotProduct( &DirVector, &TriChordVector );
	////DebugPrintf("Cos = %f\n", Cos);

	return false;

	// left/right angle
	xAngle = (float) acos( WantedVector.x );
	xAngle = 90.0F - R2D( xAngle );
	if( WantedVector.z < 0.0F )
		xAngle = 180.0F - xAngle;
	if( xAngle > 180.0F )
		xAngle -= 360.0F;
	////DebugPrintf("X Angle %f\n", xAngle);

	// up/down angle
	yAngle = (float) acos( WantedVector.y );
	yAngle = 90.0F - R2D( yAngle );
	yAngle *= -1.0F;
	if( yAngle > 180.0F )
		yAngle -= 360.0F;
	////DebugPrintf("Y Angle %f\n", yAngle);

	if( xAngle > 0.0F )
		BOTAI_SetAction( &bot.yaw, 1.0F, "MoveToTargetNew() yaw right" );
	else if( xAngle < 0.0F )
		BOTAI_SetAction( &bot.yaw, -1.0F, "MoveToTargetNew() yaw left" );

	if( yAngle < 0.0F && ( WantedVector.z > 0.0F ))
		BOTAI_SetAction( &bot.pitch, -1.0F, "MoveToTargetNew() pitch up" );
	else if( yAngle > 0.0F && ( WantedVector.z > 0.0F ))
		BOTAI_SetAction( &bot.pitch, 1.0F, "MoveToTargetNew() pitch down" );


	// Reached Target
	if(Ships[WhoIAm].Object.Pos.x < TPos->x + TOL && Ships[WhoIAm].Object.Pos.x > TPos->x - TOL
			&& Ships[WhoIAm].Object.Pos.y < TPos->y + TOL && Ships[WhoIAm].Object.Pos.y > TPos->y - TOL
			&& Ships[WhoIAm].Object.Pos.z < TPos->z + TOL && Ships[WhoIAm].Object.Pos.z > TPos->z - TOL)
		return true;
	else
		return false;
}

/* (buggy) always triple chords up/right/forward to target */
bool BOTAI_MoveToTarget(VECTOR * TPos)
{
	VECTOR NormVector;
	VECTOR DirVector;
	float xAngle;
	float yAngle;

	//DebugPrintf("moving to %f %f %f\n", TPos->x, TPos->y, TPos->z);


	// set slide movements and check if at target pos
	if(BOTAI_SlideToTarget(TPos))
		return true;

	// adjust the aim

	// -- Calculate angles
	NormVector.x = TPos->x - Ships[WhoIAm].Object.Pos.x;
	NormVector.y = TPos->y - Ships[WhoIAm].Object.Pos.y;
	NormVector.z = TPos->z - Ships[WhoIAm].Object.Pos.z;
	ApplyMatrix( &Ships[WhoIAm].Object.FinalInvMat, &NormVector, &DirVector );
	NormaliseVector( &DirVector );
	////DebugPrintf("x = %f y = %f z = %f\n", DirVector.x, DirVector.y, DirVector.z);

	// left/right angle
	xAngle = (float) acos( DirVector.x );
	xAngle = 90.0F - R2D( xAngle );
	if( DirVector.z < 0.0F )
		xAngle = 180.0F - xAngle;
	if( xAngle > 180.0F )
		xAngle -= 360.0F;
	////DebugPrintf("X Angle %f\n", xAngle);

	// up/down angle
	yAngle = (float) acos( DirVector.y );
	yAngle = 90.0F - R2D( yAngle );
	yAngle *= -1.0F;
	if( yAngle > 180.0F )
		yAngle -= 360.0F;
	////DebugPrintf("Y Angle %f\n", yAngle);

	// -- angle aim
	// sliding right
	if( bot.right == 1.0F)
	{
		// aim left
		if(xAngle < 30.0F)
			BOTAI_SetAction( &bot.yaw, -1.0F, "MoveToTarget() yaw left" );
		// aim right
		else if(xAngle > 40.0F)
			BOTAI_SetAction( &bot.yaw, 1.0F, "MoveToTarget() yaw right" );
	}

	// sliding left
	else if( bot.right == -1.0F)
	{
		// aim left
		if(xAngle < -40.0F)
			BOTAI_SetAction( &bot.yaw, -1.0F, "MoveToTarget() yaw left" );
		// aim right
		else if(xAngle > -30.0F)
			BOTAI_SetAction( &bot.yaw, 1.0F, "MoveToTarget() yaw right" );
	}

	// sliding up
	if(bot.up == 1.0F)
	{
		// aim up
		if(yAngle < -40.0F)
			BOTAI_SetAction( &bot.pitch, -1.0F, "MoveToTarget() pitch up" );
		// aim down
		else if(yAngle > -30.0F)
			BOTAI_SetAction( &bot.pitch, 1.0F, "MoveToTarget() pitch down" );
	}

	// sliding down
	else if(bot.up == -1.0F)
	{
		// aim up
		if(yAngle > 30.0F)
			BOTAI_SetAction( &bot.pitch, -1.0F, "MoveToTarget() pitch up" );
		// aim down
		else if(yAngle < 40.0F)
			BOTAI_SetAction( &bot.pitch, 1.0F, "MoveToTarget() pitch down" );
	}

	return false;
}

/* Slides to target pos without aiming at it; returns true if reached target */
bool BOTAI_SlideToTarget(VECTOR * TPos)
{
	VECTOR NormVector;
	VECTOR DirVector;
	float xAngle;
	float yAngle;

	//DebugPrintf("sliding to target\n");

	// -- Calculate angles
	NormVector.x = TPos->x - Ships[WhoIAm].Object.Pos.x;
	NormVector.y = TPos->y - Ships[WhoIAm].Object.Pos.y;
	NormVector.z = TPos->z - Ships[WhoIAm].Object.Pos.z;
	ApplyMatrix( &Ships[WhoIAm].Object.FinalInvMat, &NormVector, &DirVector );
	NormaliseVector( &DirVector );
	////DebugPrintf("x = %f y = %f z = %f\n", DirVector.x, DirVector.y, DirVector.z);

	// left/right angle
	xAngle = (float) acos( DirVector.x );
	xAngle = 90.0F - R2D( xAngle );
	if( DirVector.z < 0.0F )
		xAngle = 180.0F - xAngle;
	if( xAngle > 180.0F )
		xAngle -= 360.0F;
	////DebugPrintf("X Angle %f\n", xAngle);

	// up/down angle
	yAngle = (float) acos( DirVector.y );
	yAngle = 90.0F - R2D( yAngle );
	yAngle *= -1.0F;
	if( yAngle > 180.0F )
		yAngle -= 360.0F;
	////DebugPrintf("Y Angle %f\n", yAngle);
	////DebugPrintf("z %f\n", DirVector.z);


	// -- execute slide movements
	// sliding right
	if(xAngle > 0.0F && DistWall[0] > 100.0F)
		BOTAI_SetAction( &bot.right, 1.0F, "SlideToTarget() right" );
	// sliding left
	else if(xAngle < 0.0F && DistWall[1] > 100.0F)
		BOTAI_SetAction( &bot.right, -1.0F, "SlideToTarget() left" );

	// sliding up
	if(DistWall[2] > 100.0F) 
	{
		if(yAngle < 0.0F )
			BOTAI_SetAction( &bot.up, 1.0F, "SlideToTarget() up" );
	}
	// sliding down
	if(DistWall[3] > 100.0F) 
	{
		if(yAngle > 0.0F)
			BOTAI_SetAction( &bot.up, -1.0F, "SlideToTarget() down" );
	}

	// forward
	if(DirVector.z > 0.0F && DistWall[8] > 100.0F) 
		BOTAI_SetAction( &bot.forward, 1.0F, "SlideToTarget() forward" );
	// backward
	else if(DirVector.z < 0.0F && DistWall[9] > 100.0F )
		BOTAI_SetAction( &bot.forward, -1.0F, "SlideToTarget() reverse" );


	// Reached Target
	if(Ships[WhoIAm].Object.Pos.x < TPos->x + TOL && Ships[WhoIAm].Object.Pos.x > TPos->x - TOL
			&& Ships[WhoIAm].Object.Pos.y < TPos->y + TOL && Ships[WhoIAm].Object.Pos.y > TPos->y - TOL
			&& Ships[WhoIAm].Object.Pos.z < TPos->z + TOL && Ships[WhoIAm].Object.Pos.z > TPos->z - TOL)
		return true;
	else
		return false;
}

bool BOTAI_ShootEnemyMines()
{
	int i;
	float dist = 0.0F;
	float TeamDist = 0.0F;
	float nearest = BIGDISTANCE;
	float mindist = BIGDISTANCE;

	TargetMine = -1;

	// search all secondary bullets
	for(i = 0; i< MAXSECONDARYWEAPONBULLETS; i++)
	{
		// for mines that aren't my own
		if(SecBulls[i].SecType == SEC_MINE && SecBulls[i].Owner !=WhoIAm && SecBulls[i].Used)
		{
			// that i can see
			if( BOTAI_ClearLOS( &Ships[WhoIAm].Object.Pos, Ships[WhoIAm].Object.Group, &SecBulls[i].Pos ))
			{
				// find the nearest
				dist = DistanceVector2Vector(&Ships[WhoIAm].Object.Pos, &SecBulls[i].Pos);
				if(dist < nearest)
				{
					TargetMine = i;
					nearest = dist;
				}
			}
		}
	}

	// if i have a mine target
	if(TargetMine > -1)
	{
		// don't hit any team members 
		if( TeamGame )
		{
			// i will hit team mate with the bullet
			if( BOTAI_FriendlyFireCheck() )
				return false;

			// team mate will endure shockwave damage
			for(i = 0; i < MAX_PLAYERS; i++)
			{
				if(TeamNumber[WhoIAm] != TeamNumber[i])
					continue;

				TeamDist = DistanceVector2Vector(&Ships[i].Object.Pos, &SecBulls[TargetMine].Pos);
				if( TeamDist < dist )
					return false;
			}
		}

		// aim at mine
		BOTAI_AimAtTarget(&Ships[WhoIAm].Object.FinalInvMat, &Ships[WhoIAm].Object.Pos, &SecBulls[TargetMine].Pos);

		// try not to get too close to mines
		switch(SecBulls[TargetMine].Weapon)
		{
			// ideal safest distance
			case QUANTUMMINE:
				dist = (BALL_RADIUS * QUANTUM_SHOCKWAVE) + SHIP_RADIUS;
				break;
			default:
				dist = (BALL_RADIUS * PURGE_SHOCKWAVE) + SHIP_RADIUS;
				break;
		}

		// however, if i only have short range weapons then i need to be a bit closer to hit it
		BOTAI_SelectWeapon( true );
		switch( Ships[WhoIAm].Primary )
		{
			case PYROLITE_RIFLE: 
			case SUSS_GUN:
				if( dist > 1300.0F )
					dist = 1300.0F;
				mindist = dist;
				break;
		}

		//DebugPrintf("i have a mine target at %f distance\n", nearest);
		//DebugPrintf("min dist = %f\n", (BALL_RADIUS * QUANTUM_SHOCKWAVE) + SHIP_RADIUS);

		// back up to safest distance or until i hit a wall
		if( nearest < dist && DistWall[9] > 100.0F )
			BOTAI_SetAction( &bot.forward, -1.0F, "ShootEnemyMines() reverse" );
		// too far away from the mine with the current weapon i have so move forward
		else if( nearest > mindist && DistWall[8] > 100.0F )
			BOTAI_SetAction( &bot.forward, 1.0F, "ShootEnemyMines() forward" );
		// at an ok distance 
		else
		{
			// release trojax
			if(Ships[WhoIAm].Primary == TROJAX && PowerLevel > 10.0F)
				BOTAI_SetAction( &bot.fire_primary, 0.0F, "ShootEnemyMines() fire primary off" );
			// shoot primary
			else
				BOTAI_SetAction( &bot.fire_primary, 1.0F, "ShootEnemyMines() fire primary" );
		}
		return true;
	}
	else
		return false;
}

void BOTAI_GetNearestPickup()
{
	int i;
	float dist;
	float nearestDist=BIGDISTANCE;

	GettingPickup = -1;

	// for all pickups
	for(i=0; i<MAXPICKUPS; i++)
	{
		// that are enabled
		if(Pickups[i].Type == (u_int16_t) -1)
			continue;

		// that i can see
		if( BOTAI_ClearLOS( &Ships[WhoIAm].Object.Pos, Ships[WhoIAm].Object.Group, &Pickups[i].Pos ))
		{
			// and that i can collect
			if( BOTAI_CanICollectPickup(i) )
			{
				dist = DistanceVector2Vector(&Ships[WhoIAm].Object.Pos, &Pickups[i].Pos);
				// find the nearest
				if(dist < nearestDist)
				{
					nearestDist = dist;
					GettingPickup = i;
				}
			}
		}
	}
}

bool BOTAI_CollectNearestPickup(bool SlideOnly)
{
	// Pickup identified
	if(GettingPickup > -1)
	{
		// Make sure we search for nodes after
		CurrentNode = -1;

		// if we lost LOS 
		if(!BOTAI_ClearLOS( &Ships[WhoIAm].Object.Pos, Ships[WhoIAm].Object.Group, &Pickups[GettingPickup].Pos ) || Pickups[GettingPickup].Type == (u_int16_t) -1)
		{
			DebugPrintf("lost pickup los %s\n", WepMessages[ Pickups[GettingPickup].Type ]);
			GettingPickup = -1;
			return false;
		}

		// otherwise, move to the target and collect
		if(!SlideOnly)
		{
			DebugPrintf("moving to pickup %s\n", WepMessages[ Pickups[GettingPickup].Type ]);
			if( BOTAI_MoveToTarget(&Pickups[GettingPickup].Pos) )
				GettingPickup = -1;
		}
		// try and slide near it 
		else
		{
			DebugPrintf("sliding to pickup %s\n", WepMessages[ Pickups[GettingPickup].Type ]);
			if( BOTAI_SlideToTarget(&Pickups[GettingPickup].Pos) )
				GettingPickup = -1;
		}

		return true;
	}
	else
		return false;
}

float BOTAI_DistanceToWall( bool u, bool d, bool l, bool r, bool f, bool b )
{
	float dist;
	VECTOR temp;
	NORMAL FaceNormal;
	VECTOR Pos_New;
	VECTOR ImpactPoint;
	u_int16_t ImpactGroup;
	VECTOR Slide;

	// calculate the desired vector 
	Slide.x = 0.0F;
	Slide.y = 0.0F;
	Slide.z = 0.0F;

	if(r) Slide.x = 1.0F;	
	else if(l) Slide.x = -1.0F;		
	if(u) Slide.y = 1.0F;	
	else if(d) Slide.y = -1.0F;	
	if(f) Slide.z = 1.0F;	
	else if(b) Slide.z = -1.0F;

	ApplyMatrix( &Ships[WhoIAm].Object.FinalMat, &Slide, &temp );

	temp.x *= MaxColDistance;
	temp.y *= MaxColDistance;
	temp.z *= MaxColDistance;

	// get collision details	
	BackgroundCollide( &MCloadheadert0, &Mloadheader, &Ships[WhoIAm].Object.Pos, Ships[WhoIAm].Object.Group, &temp, &ImpactPoint, &ImpactGroup, &FaceNormal, &Pos_New, false, NULL );

	// return the distance
	dist = DistanceVector2Vector(&Ships[WhoIAm].Object.Pos, &ImpactPoint);

	// if movement will put me in a gravgon we want to avoid this
	Pos_New = Ships[WhoIAm].Object.Pos;
	BOTAI_LookAheadPos( 10.0F, &Pos_New, Slide );
	if( BOTAI_CheckForGravgons( &Pos_New ) ) 
		dist = 1.0F;

	return dist;
}

void BOTAI_LookAheadPos( float Time, VECTOR * NewPos, VECTOR SlideDirection )
{
	VECTOR Move_Off;

	ApplyMatrix( &Ships[WhoIAm].Object.FinalMat, &SlideDirection, &Move_Off );
	NewPos->x += Move_Off.x * MaxMoveSpeed * Time;
	NewPos->y += Move_Off.y * MaxMoveSpeed * Time;
	NewPos->z += Move_Off.z * MaxMoveSpeed * Time;
}

void BOTAI_LookAhead( float Accuracy, VECTOR * SPos, BYTE TargetID, VECTOR * NewPos, float SpeedOfBullet )
{
	float Time;
	float Distance;

	Distance = DistanceVector2Vector( &Ships[TargetID].Object.Pos, SPos );
	Time = ( Distance / SpeedOfBullet );

	// only predict forward if they have nitro
	if(Ships[TargetID].Object.Flags & SHIP_Turbo)
		Accuracy = fabs(Accuracy);

	NewPos->x = Ships[TargetID].Object.Pos.x + ( Ships[TargetID].Move_Off.x * Time * Accuracy );
	NewPos->y = Ships[TargetID].Object.Pos.y + ( Ships[TargetID].Move_Off.y * Time * Accuracy );
	NewPos->z = Ships[TargetID].Object.Pos.z + ( Ships[TargetID].Move_Off.z * Time * Accuracy );
}

bool BOTAI_AimAtTarget( MATRIX * InvMat , VECTOR * SPos, VECTOR * TPos )
{
	VECTOR WantedDir;
	VECTOR TempDir;
	float Angle;
	float OnTarget = true;

	WantedDir.x = ( TPos->x - SPos->x );
	WantedDir.y = ( TPos->y - SPos->y );
	WantedDir.z = ( TPos->z - SPos->z );
	ApplyMatrix( InvMat, &WantedDir, &TempDir );

	NormaliseVector( &TempDir );
	Angle = (float) acos( TempDir.x );
	Angle = 90.0F - R2D( Angle );

	if( TempDir.z < 0.0F )
		Angle = 180.0F - Angle;

	if( Angle > 180.0F )
		Angle -= 360.0F;

	if( Angle > 0.0F )
	{
		BOTAI_SetAction( &bot.yaw, 1.0F, "AimAtTarget() yaw right" );
		if( Angle > 3.0F )
			OnTarget = false;
	}
	else if( Angle < 0.0F )
	{
		BOTAI_SetAction( &bot.yaw, -1.0F, "AimAtTarget() yaw left" );
		if( Angle < -3.0F )
			OnTarget = false;
	}

	Angle = (float) acos( TempDir.y );
	Angle = 90.0F - R2D( Angle );
	Angle *= -1.0F;

	if( Angle > 180.0F )
		Angle -= 360.0F;

	if( Angle < 0.0F && ( TempDir.z > 0.0F ))
	{
		BOTAI_SetAction( &bot.pitch, -1.0F, "AimAtTarget() pitch up" );
		if(Angle < -3.0F)
			OnTarget = false;
	}
	else if( Angle > 0.0F && ( TempDir.z > 0.0F ))
	{
		BOTAI_SetAction( &bot.pitch, 1.0F, "AimAtTarget() pitch down" );
		if(Angle > 3.0F)
			OnTarget = false;
	}

	return OnTarget;
}

bool BOTAI_ClearLOS( VECTOR * SPos, u_int16_t Group , VECTOR * Pos )
{
	VECTOR Dir;
	VECTOR Int_Point;
	u_int16_t Int_Group;
	NORMAL Int_Normal;
	VECTOR TempVector;

	Dir.x = Pos->x - SPos->x;
	Dir.y = Pos->y - SPos->y;
	Dir.z = Pos->z - SPos->z;

	return !BackgroundCollide( &MCloadheadert0, &Mloadheader, SPos, Group, &Dir, &Int_Point, &Int_Group, &Int_Normal, &TempVector, true, NULL );
}

bool BOTAI_ClearLOSNonZero( OBJECT * SObject, VECTOR * Pos , float radius )
{
	VECTOR Dir;
	BGOBJECT * BGObject;

	Dir.x = Pos->x - SObject->Pos.x;
	Dir.y = Pos->y - SObject->Pos.y;
	Dir.z = Pos->z - SObject->Pos.z;

	return !WouldObjectCollide( SObject, &Dir, radius, &BGObject );
}

void BOTAI_GetSHIPTarget()
{
	float dist;
	float nearest = BIGDISTANCE;
	int	i;

	TargetShipID = -1;

	// for each player
	for( i = 0; i < MAX_PLAYERS; i++ )
	{
		// that's not me, and must be alive
		if( i != WhoIAm && Ships[i].enable && (Ships[i].Object.Mode == NORMAL_MODE) 
		  ) // && ( !(Ships[i].Object.Flags & SHIP_Stealth) || (Ships[i].Object.Flags & SHIP_Litup) ) )
		  {
			  // and not on my team
			  if(TeamGame && TeamNumber[WhoIAm] == TeamNumber[i])
				  continue;

			  // and that i can hear
			  if( !SoundInfo[Ships[WhoIAm].Object.Group][Ships[i].Object.Group] )
			  {
				  // find the closest
				  dist = DistanceVector2Vector( &Ships[i].Object.Pos, &Ships[WhoIAm].Object.Pos );
				  if( Ships[i].Object.Noise != 0.0F )
					  dist *= ( 1.0F * ( 0.2F * Ships[i].Object.Noise ) );

				  if( dist < nearest )
				  {
					  TargetShipID = i;
					  nearest = dist;         
				  }
			  }
		  }
	}
}

bool BOTAI_InViewCone( VECTOR * Pos, MATRIX * Mat, VECTOR * TPos, float ViewConeCos )
{
	float Cos;
	VECTOR NormVector;
	VECTOR Dir;

	if( ViewConeCos == 1.0F )
		return true;

	Dir.x = TPos->x - Pos->x;
	Dir.y = TPos->y - Pos->y;
	Dir.z = TPos->z - Pos->z;
	NormVector = Dir;
	NormaliseVector( &NormVector );
	ApplyMatrix( Mat, &Forward, &Dir );

	Cos = DotProduct( &NormVector, &Dir );
	if( Cos > ViewConeCos )
		return true;

	return false;
}

bool BOTAI_FriendlyFireCheck()
{
	int i;
	VECTOR Move_Dir;
	VECTOR TempVector;

	// no need to check if not a team game
	if(!TeamGame)
		return false;

	// calculate my direction vector
	ApplyMatrix( &Ships[WhoIAm].Object.Mat, &Forward, &Move_Dir );
	NormaliseVector( &Move_Dir );

	// for each player
	for(i = 0; i < MAX_PLAYERS; i++)	
	{
		// that is enabled
		if( i != WhoIAm && Ships[i].enable && Ships[i].Object.Mode == NORMAL_MODE )
		{
			// and on my team
			if(TeamNumber[WhoIAm] != TeamNumber[i])
				continue;

			// are we within visible params?
			if( RaytoSphere2(&Ships[i].Object.Pos, SHIP_RADIUS, &Ships[WhoIAm].Object.Pos, &Move_Dir , &TempVector , &TempVector ) )
				return true;
		}
	}

	return false;
}

bool BOTAI_CanICollectPickup( u_int16_t i )
{
	int16_t	PickupEnable = false;

	if( i != (u_int16_t) -1 )
	{
		switch( Pickups[i].Type )
		{
			case PICKUP_Trojax: 
				if( !PrimaryWeaponsGot[ TROJAX ] )
					PickupEnable = true;
				break;

			case PICKUP_Pyrolite:
				if( !PrimaryWeaponsGot[ PYROLITE_RIFLE ] )
					PickupEnable = true;
				break;

			case PICKUP_Transpulse:
				if( !PrimaryWeaponsGot[ TRANSPULSE_CANNON ] )
					PickupEnable = true;
				break;

			case PICKUP_SussGun:
				if( !PrimaryWeaponsGot[ SUSS_GUN ] )
					PickupEnable = true;
				break;

			case PICKUP_Laser:
				if( !PrimaryWeaponsGot[ LASER ] )
					PickupEnable = true;
				break;

			case PICKUP_Mugs:
				if( SecondaryAmmo[ MUGMISSILE ] < 10 )
					PickupEnable = true;
				break;

			case PICKUP_HeatseakerPickup:
				if( SecondaryAmmo[ SOLARISMISSILE ] < 10 )
					PickupEnable = true;
				break;

			case PICKUP_Thief:
				if( !SecondaryAmmo[ THIEFMISSILE ] )
					PickupEnable = true;
				break;

			case PICKUP_Scatter:
				if( SecondaryAmmo[ SCATTERMISSILE ] < 3 )
					PickupEnable = true;
				break;

			case PICKUP_Gravgon:
				if( !SecondaryAmmo[ GRAVGONMISSILE ] )
					PickupEnable = true;
				break;

			case PICKUP_Launcher:
				if( SecondaryAmmo[ MULTIPLEMISSILE ] < 100 )
					PickupEnable = true;
				break;

			case PICKUP_TitanStar:
				if( SecondaryAmmo[ TITANSTARMISSILE ] < 3 )
					PickupEnable = true;
				break;

			case PICKUP_PurgePickup:
				if( SecondaryAmmo[ PURGEMINE ] < 10 )
					PickupEnable = true;
				break;

			case PICKUP_PinePickup:
				if( SecondaryAmmo[ PINEMINE ] < 6 )
					PickupEnable = true;
				break;

			case PICKUP_QuantumPickup:
				if( !SecondaryAmmo[ QUANTUMMINE ] )
					PickupEnable = true;
				break;

			case PICKUP_SpiderPod:
				if( SecondaryAmmo[ SPIDERMINE ] < 6 )
					PickupEnable = true;
				break;

			case PICKUP_GeneralAmmo:
				if( GeneralAmmo <  1000.0F ) // MAXGENERALAMMO )
					PickupEnable = true;
				break;

			case PICKUP_PyroliteAmmo:
				if( PrimaryWeaponsGot[ PYROLITE_RIFLE ] && PyroliteAmmo < 1000.0F ) // MAXPYROLITEAMMO )
					PickupEnable = true;
				break;

			case PICKUP_SussGunAmmo:
				if( PrimaryWeaponsGot[ SUSS_GUN ] && SussGunAmmo < 1000.0F ) //MAXSUSSGUNAMMO )
					PickupEnable = true;
				break;

			case PICKUP_PowerPod:
				if( Ships[ WhoIAm].Object.PowerLevel < 2 )
					PickupEnable = true;
				break;

			case PICKUP_GoldenPowerPod:
				if( Ships[ WhoIAm ].SuperNashramTimer == 0.0F )
					PickupEnable = true;
				break;

			case PICKUP_Inv:
				if( Ships[ WhoIAm ].InvulTimer == 0.0F )
					PickupEnable = true;
				break;

			case PICKUP_Nitro:
				if( NitroFuel < MAX_NITRO_FUEL )
					PickupEnable = true;
				break;

			case PICKUP_Mantle:
				if( Ships[ WhoIAm ].StealthTime == 0.0F )
					PickupEnable = true;
				break;

			case PICKUP_Shield:
				if( Ships[ WhoIAm ].Object.Shield < MAX_SHIELD )
					PickupEnable = true;
				break;

			case PICKUP_Flag:
			case PICKUP_Bounty:
				PickupEnable = true;
				break;

			case PICKUP_Orb:
				if( NumOrbs < MAXMULTIPLES )
					PickupEnable = true;
				break;		 
		}

		if( PickupEnable )
			return true;
		else
			return false;
	}
	else
		return false;
}

bool BOTAI_WillHomingMissileHit(VECTOR * MyPos)
{
	float Cos;
	float Angle;
	VECTOR DirVector;
	VECTOR TmpVec;
	QUATLERP qlerp;
	SECONDARYWEAPONBULLET MissCopy = SecBulls[ HomingMissile ];

	// direction vector from missile to me
	DirVector.x = MyPos->x - MissCopy.Pos.x;
	DirVector.y = MyPos->y - MissCopy.Pos.y;
	DirVector.z = MyPos->z - MissCopy.Pos.z;
	NormaliseVector( &DirVector );

	// angle difference between the missile's current vector and wanted vector
	Cos = DotProduct( &DirVector, &MissCopy.DirVector );

	// set the parameters to perform a linear interpolation on two quaternions
	QuatFrom2Vectors( &qlerp.end, &Forward, &DirVector );
	qlerp.start	= MissCopy.DirQuat;				
	qlerp.crnt	= &MissCopy.DirQuat;	
	qlerp.dir	= QuatDotProduct( &qlerp.start, &qlerp.end );

	// bound angle difference
	if( Cos < -1.0F ) Cos = -1.0F;
	else if ( Cos > 1.0F ) Cos = 1.0F;

	// get angle difference in radians
	Angle = (float) acos( Cos );

	// calculate the amount of angle to turn 
	if( Angle ) qlerp.time = ( ( D2R( MissCopy.TurnSpeed ) * framelag ) / Angle );
	else qlerp.time = 1.0F;
	if( qlerp.time > 1.0F ) qlerp.time = 1.0F;

	// perform quat interpolation
	QuatInterpolate( &qlerp );
	QuatToMatrix( &MissCopy.DirQuat, &MissCopy.Mat );
	ApplyMatrix( &MissCopy.Mat, &Forward, &MissCopy.DirVector );
	ApplyMatrix( &MissCopy.Mat, &SlideUp, &MissCopy.UpVector );

	// will missile hit?
	if(RaytoSphere2(MyPos, SHIP_RADIUS, &MissCopy.Pos, &MissCopy.DirVector, &TmpVec, &TmpVec ))
	{
		DebugPrintf("homing missile will hit\n");
		return true;
	}
	else
	{
		DebugPrintf("safe from homing missile\n");
		return false;
	}
}

void BOTAI_AvoidHomingMissiles()
{
	VECTOR NormVector;
	VECTOR DirVector;
	float xAngle;
	float yAngle;

	if(HomingMissile > -1)
	{
		// -- decide what movement to perform
		// new missile
		//if(!MissileAvoidanceSet)
		{
			//DebugPrintf("set details for missile %d\n", HomingMissile);
			MissileAvoidanceSet = true;
			NormVector.x = SecBulls[HomingMissile].Pos.x - Ships[WhoIAm].Object.Pos.x;
			NormVector.y = SecBulls[HomingMissile].Pos.y - Ships[WhoIAm].Object.Pos.y;
			NormVector.z = SecBulls[HomingMissile].Pos.z - Ships[WhoIAm].Object.Pos.z;
			ApplyMatrix( &Ships[WhoIAm].Object.FinalInvMat, &NormVector, &DirVector );
			NormaliseVector( &DirVector );
			MissileOrigDirVector = DirVector;
		}


		// left/right angle
		xAngle = (float) acos( MissileOrigDirVector.x );
		xAngle = 90.0F - R2D( xAngle );
		if( MissileOrigDirVector.z < 0.0F )
			xAngle = 180.0F - xAngle;
		if( xAngle > 180.0F )
			xAngle -= 360.0F;
		//DebugPrintf("x = %f\n", xAngle);

		// up/down angle
		yAngle = (float) acos( MissileOrigDirVector.y );
		yAngle = 90.0F - R2D( yAngle );
		yAngle *= -1.0F;
		if( yAngle > 180.0F )
			yAngle -= 360.0F;
		//DebugPrintf("y = %f\n", yAngle);

		// -- execute slide movements

		// target is directly in front or behind me
		if((xAngle > -22.5F && xAngle < 22.5F) || (xAngle < -157.5F || xAngle > 157.5F))
		{
			// slide either left or right, doesn't matter
			BOTAI_SetAction( &bot.right, 1.0F, "AvoidHomingMissiles() right" );
		}
		// target is directly to the side of me
		else if((xAngle > 67.5F && xAngle < 112.5F) || (xAngle < -67.5F && xAngle > -112.5F))
		{
			// forward or backward, doesn't matter
			BOTAI_SetAction( &bot.forward, 1.0F, "AvoidHomingMissiles() forward" );
		}

		// target is to my front left 
		else if(xAngle > -67.5F && xAngle < -22.5F)
		{
			// move forward right 
			BOTAI_SetAction( &bot.forward, 1.0F, "AvoidHomingMissiles() forward" );
			BOTAI_SetAction( &bot.right, 1.0F, "AvoidHomingMissiles() right" );
		}

		// target is to my rear left 
		else if(xAngle > -157.5F && xAngle < -112.5F)
		{
			// move back left
			BOTAI_SetAction( &bot.forward, -1.0F, "AvoidHomingMissiles() reverse" );
			BOTAI_SetAction( &bot.forward, -1.0F, "AvoidHomingMissiles() left" );
		}

		// target is to my front right
		else if(xAngle > 22.5F && xAngle < 67.5F)
		{
			// move front left 
			BOTAI_SetAction( &bot.forward, 1.0F, "AvoidHomingMissiles() forward" );
			BOTAI_SetAction( &bot.right, -1.0F, "AvoidHomingMissiles() left" );
		}

		// target is to my rear right 
		else if(xAngle > 112.5F && xAngle < 157.5F)
		{
			// move back right 
			BOTAI_SetAction( &bot.forward, -1.0F, "AvoidHomingMissiles() reverse" );
			BOTAI_SetAction( &bot.right, 1.0F, "AvoidHomingMissiles() right" );
		}

		if(yAngle > 0.0F)
			BOTAI_SetAction( &bot.up, -1.0F, "AvoidHomingMissiles() down" );
		else
			BOTAI_SetAction( &bot.up, 1.0F, "AvoidHomingMissiles() up" );
	}
}

void BOTAI_UpdateSensors()
{
	int CurrentDeaths = GetTotalDeaths(WhoIAm);
	int CurrentKills = GetTotalKills(WhoIAm);
	int i;

	// i died or killed someone
	if( CurrentDeaths > PrevDeaths || CurrentKills > PrevKills )
	{
		// mix up the route
		if(Random_Range(2) > 0)
			ReverseNetwork = true;
		else
			ReverseNetwork = false;

		// i died, reset flags 
		if( CurrentDeaths > PrevDeaths )
		{
			//DebugPrintf("i died\n");
			CurrentNode = -1;
			GettingPickup = -1;
			IFiredTitan = -1.0F;
			TargetMine = -1;
			HomingMissile = -1;
			PrevHomingMissile = -1;
			MissileAvoidanceSet = false;
			PrevHull = MAX_HULL+1.0F;
			PrevShield = MAX_SHIELD+1.0F;
		}

		PrevDeaths = CurrentDeaths;
		PrevKills = CurrentKills;
	}

	// i got hit
	if( Ships[WhoIAm].Object.Hull < PrevHull || Ships[WhoIAm].Object.Shield < PrevShield )
	{
		PrevHull = Ships[WhoIAm].Object.Hull;
		PrevShield = Ships[WhoIAm].Object.Shield;

		// debugging stuff
		if(!initialised)
			initialised = true;
		else
			DEBUG_AVOIDANCE = false;
	}

	// -- update sensors
	// up, down, left, right, forward, backward
	DistWall[0] = BOTAI_DistanceToWall(false,false,false,true,false,false); // right
	DistWall[1] = BOTAI_DistanceToWall(false,false,true,false,false,false); // left
	DistWall[2] = BOTAI_DistanceToWall(true,false,false,false,false,false); // up 
	DistWall[3] = BOTAI_DistanceToWall(false,true,false,false,false,false); // down
	DistWall[4] = BOTAI_DistanceToWall(true,false,false,true,false,false); // up right
	DistWall[5] = BOTAI_DistanceToWall(true,false,true,false,false,false); // up left
	DistWall[6] = BOTAI_DistanceToWall(false,true,false,true,false,false); // down right
	DistWall[7] = BOTAI_DistanceToWall(false,true,true,false,false,false); // down left
	DistWall[8] = BOTAI_DistanceToWall(false,false,false,false,true,false); // forward
	DistWall[9] = BOTAI_DistanceToWall(false,false,false,false,false,true); // backward
	DistWall[10] = BOTAI_DistanceToWall(false,false,false,true,false,true); // right backward
	DistWall[11] = BOTAI_DistanceToWall(false,false,true,false,false,true); // left backward
	DistWall[12] = BOTAI_DistanceToWall(true,false,false,false,false,true); // up backward
	DistWall[13] = BOTAI_DistanceToWall(false,true,false,false,false,true); // down backward
	DistWall[14] = BOTAI_DistanceToWall(true,false,false,true,false,true); // up right backward
	DistWall[15] = BOTAI_DistanceToWall(true,false,true,false,false,true); // up left backward
	DistWall[16] = BOTAI_DistanceToWall(false,true,false,true,false,true); // down right backward
	DistWall[17] = BOTAI_DistanceToWall(false,true,true,false,false,true); // down left backward
	DistWall[18] = BOTAI_DistanceToWall(false,false,false,true,true,false); // right forward
	DistWall[19] = BOTAI_DistanceToWall(false,false,true,false,true,false); // left forward
	DistWall[20] = BOTAI_DistanceToWall(true,false,false,false,true,false); // up forward
	DistWall[21] = BOTAI_DistanceToWall(false,true,false,false,true,false); // down forward
	DistWall[22] = BOTAI_DistanceToWall(true,false,false,true,true,false); // up right forward
	DistWall[23] = BOTAI_DistanceToWall(true,false,true,false,true,false); // up left forward
	DistWall[24] = BOTAI_DistanceToWall(false,true,false,true,true,false); // down right forward
	DistWall[25] = BOTAI_DistanceToWall(false,true,true,false,true,false); // down left forward

	AllSensorsClear = true;
	WhenWillHitSlide[0] = BOTAI_WhenWillBulletHitSlide(false,false,false,true,false,false); // r
	WhenWillHitSlide[1] = BOTAI_WhenWillBulletHitSlide(false,false,true,false,false,false); // l
	WhenWillHitSlide[2] = BOTAI_WhenWillBulletHitSlide(true,false,false,false,false,false); // u
	WhenWillHitSlide[3] = BOTAI_WhenWillBulletHitSlide(false,true,false,false,false,false); // d
	WhenWillHitSlide[4] = BOTAI_WhenWillBulletHitSlide(true,false,false,true,false,false); // u r
	WhenWillHitSlide[5] = BOTAI_WhenWillBulletHitSlide(true,false,true,false,false,false); // u l
	WhenWillHitSlide[6] = BOTAI_WhenWillBulletHitSlide(false,true,false,true,false,false); // d r
	WhenWillHitSlide[7] = BOTAI_WhenWillBulletHitSlide(false,true,true,false,false,false); // d l
	WhenWillHitSlide[8] = BOTAI_WhenWillBulletHitSlide(false,false,false,false,true,false); // f
	WhenWillHitSlide[9] = BOTAI_WhenWillBulletHitSlide(false,false,false,false,false,true); // b
	WhenWillHitSlide[10] = BOTAI_WhenWillBulletHitSlide(false,false,false,true,false,true); // r b
	WhenWillHitSlide[11] = BOTAI_WhenWillBulletHitSlide(false,false,true,false,false,true); // l b
	WhenWillHitSlide[12] = BOTAI_WhenWillBulletHitSlide(true,false,false,false,false,true); // u b
	WhenWillHitSlide[13] = BOTAI_WhenWillBulletHitSlide(false,true,false,false,false,true); // d b
	WhenWillHitSlide[14] = BOTAI_WhenWillBulletHitSlide(true,false,false,true,false,true); // u r b
	WhenWillHitSlide[15] = BOTAI_WhenWillBulletHitSlide(true,false,true,false,false,true); // u l b
	WhenWillHitSlide[16] = BOTAI_WhenWillBulletHitSlide(false,true,false,true,false,true); // d r b
	WhenWillHitSlide[17] = BOTAI_WhenWillBulletHitSlide(false,true,true,false,false,true); // d l b
	WhenWillHitSlide[18] = BOTAI_WhenWillBulletHitSlide(false,false,false,true,true,false); // r f
	WhenWillHitSlide[19] = BOTAI_WhenWillBulletHitSlide(false,false,true,false,true,false); // l f
	WhenWillHitSlide[20] = BOTAI_WhenWillBulletHitSlide(true,false,false,false,true,false); // u f
	WhenWillHitSlide[21] = BOTAI_WhenWillBulletHitSlide(false,true,false,false,true,false); // d f
	WhenWillHitSlide[22] = BOTAI_WhenWillBulletHitSlide(true,false,false,true,true,false); // u r f
	WhenWillHitSlide[23] = BOTAI_WhenWillBulletHitSlide(true,false,true,false,true,false); // u l f
	WhenWillHitSlide[24] = BOTAI_WhenWillBulletHitSlide(false,true,false,true,true,false); // d r f
	WhenWillHitSlide[25] = BOTAI_WhenWillBulletHitSlide(false,true,true,false,true,false); // d l f

	// time since i last fired titan
	// used so i don't move into the blast
	IFiredTitan -= framelag;

	BOTAI_GetNearestPickup();
	FriendlyFire = BOTAI_FriendlyFireCheck();
	BOTAI_GetSHIPTarget();

	//DebugPrintf("r: %.1f l: %.1f u: %.1f d: %.1f f: %.1f b: %.1f\n", DistWall[0], DistWall[1], DistWall[2], DistWall[3], DistWall[8], DistWall[9]);
	//DebugPrintf("ur: %.1f ul: %.1f dr: %.1f dl: %.1f\n", DistWall[22], DistWall[23], DistWall[24], DistWall[25]);
	//DebugPrintf("%f %f %f\n", Ships[WhoIAm].Object.Pos.x, Ships[WhoIAm].Object.Pos.y, Ships[WhoIAm].Object.Pos.z); 
	//DebugPrintf("Level: %s\n", ShortLevelNames[LevelNum]);
	//DebugPrintf("nearest node = %d\n", BOTAI_GetNearestNode(&Ships[WhoIAm].Object));

	// if there is a target
	if(TargetShipID > - 1)
	{
		// distance to target
		TargetShipDistance = DistanceVector2Vector(&Ships[WhoIAm].Object.Pos, &Ships[TargetShipID].Object.Pos);

		// health of target
		TargetShipHealth = PlayerHealths[TargetShipID].Shield + PlayerHealths[TargetShipID].Hull;
	}
}

void BOTAI_AvoidBullets()
{
	int i = 0;
	float dist = 0.0F;
	float longest = 0.0F;

	// bullet will hit us if we stay in current position
	if(BOTAI_WhenWillBulletHitMe(&Ships[WhoIAm].Object.Pos) > -1.0F)
	{
		if(AvoidMove < 0)
		{
			// find perfect spot
			for(i = 0; i < NUM_SENSORS; i++)
			{
				if(WhenWillHitSlide[i] > -1.0F)
					DebugPrintf("will hit %d\n", i);

				// nothing will hit and there's space to move
				if(WhenWillHitSlide[i] < 0.0F && DistWall[i] > MIN_WALLDIST)	
				{
					// move to the one with most room
					if(DistWall[i] > dist)
					{	
						//DebugPrintf("using %d instead of %d because %f > %f\n", i, AvoidMove, DistWall[i], dist);
						AvoidMove = i;// BOTAI_CenterCheck(i);
						DebugPrintf("using %d\n", AvoidMove);
						dist = DistWall[i];
					}
				}
			}

			// couldn't find perfect so try safest
			if(AvoidMove < 0)
			{
				for(i = 0; i < NUM_SENSORS; i++)
				{
					// something will hit but i have space and some time
					if(WhenWillHitSlide[i] > longest && DistWall[i] > MIN_WALLDIST)
					{
						AvoidMove = i;
						longest = WhenWillHitSlide[i];
					}
				}
				//if(DEBUG_AVOIDANCE)
				DebugPrintf("couldn't find perfect spot: using %d\n", AvoidMove);
			}
			//else if(DEBUG_AVOIDANCE)
			DebugPrintf("found perfect spot: %d\n", AvoidMove);
		}

		// carry out the movement
		switch(AvoidMove)
		{
			case -1: 
				BOTAI_SetAction( &bot.forward, 1.0F, "AvoidBullets() forward" );
				BOTAI_SetAction( &bot.right, 1.0F, "AvoidBullets() right" );
				BOTAI_SetAction( &bot.up, 1.0F, "AvoidBullets() up" );
				break;
			case 0:
				BOTAI_SetAction( &bot.right, 1.0F, "AvoidBullets() right" );
				break;
			case 1: 
				BOTAI_SetAction( &bot.right, -1.0F, "AvoidBullets() left" );
				break;
			case 2: 
				BOTAI_SetAction( &bot.up, 1.0F, "AvoidBullets() up" );
				break;
			case 3: 
				BOTAI_SetAction( &bot.up, -1.0F, "AvoidBullets() down" );
				break;
			case 4:
				BOTAI_SetAction( &bot.right, 1.0F, "AvoidBullets() right" );
				BOTAI_SetAction( &bot.up, 1.0F, "AvoidBullets() up" );
				break;
			case 5: 
				BOTAI_SetAction( &bot.right, -1.0F, "AvoidBullets() left" );
				BOTAI_SetAction( &bot.up, 1.0F, "AvoidBullets() up" );
				break;
			case 6:
				BOTAI_SetAction( &bot.right, 1.0F, "AvoidBullets() right" );
				BOTAI_SetAction( &bot.up, -1.0F, "AvoidBullets() down" );
				break;
			case 7:
				BOTAI_SetAction( &bot.right, -1.0F, "AvoidBullets() left" );
				BOTAI_SetAction( &bot.up, -1.0F, "AvoidBullets() down" );
				break;
			case 8: 
				BOTAI_SetAction( &bot.forward, 1.0F, "AvoidBullets() forward" );
				break;
			case 9:
				BOTAI_SetAction( &bot.forward, -1.0F, "AvoidBullets() reverse" );
				break;
			case 10:
				BOTAI_SetAction( &bot.right, 1.0F, "AvoidBullets() right" );
				BOTAI_SetAction( &bot.forward, -1.0F, "AvoidBullets() reverse" );
				break;
			case 11:
				BOTAI_SetAction( &bot.forward, -1.0F, "AvoidBullets() reverse" );
				BOTAI_SetAction( &bot.right, -1.0F, "AvoidBullets() left" );
				break;
			case 12:
				BOTAI_SetAction( &bot.forward, -1.0F, "AvoidBullets() reverse" );
				BOTAI_SetAction( &bot.up, 1.0F, "AvoidBullets() up" );
				break;
			case 13:
				BOTAI_SetAction( &bot.forward, -1.0F, "AvoidBullets() reverse" );
				BOTAI_SetAction( &bot.up, -1.0F, "AvoidBullets() down" );
				break;
			case 14:
				BOTAI_SetAction( &bot.up, 1.0F, "AvoidBullets() up" );
				BOTAI_SetAction( &bot.right, 1.0F, "AvoidBullets() right" );
				BOTAI_SetAction( &bot.forward, -1.0F, "AvoidBullets() reverse" );
				break;
			case 15:
				BOTAI_SetAction( &bot.up, 1.0F, "AvoidBullets() up" );
				BOTAI_SetAction( &bot.right, -1.0F, "AvoidBullets() left" );
				BOTAI_SetAction( &bot.forward, -1.0F, "AvoidBullets() reverse" );
				break;
			case 16:
				BOTAI_SetAction( &bot.up, -1.0F, "AvoidBullets() down" );
				BOTAI_SetAction( &bot.right, 1.0F, "AvoidBullets() right" );
				BOTAI_SetAction( &bot.forward, -1.0F, "AvoidBullets() reverse" );
				break;
			case 17:
				BOTAI_SetAction( &bot.up, -1.0F, "AvoidBullets() down" );
				BOTAI_SetAction( &bot.right, -1.0F, "AvoidBullets() left" );
				BOTAI_SetAction( &bot.forward, -1.0F, "AvoidBullets() reverse" );
				break;
			case 18:
				BOTAI_SetAction( &bot.right, 1.0F, "AvoidBullets() right" );
				BOTAI_SetAction( &bot.forward, 1.0F, "AvoidBullets() forward" );
				break;
			case 19:
				BOTAI_SetAction( &bot.right, -1.0F, "AvoidBullets() left" );
				BOTAI_SetAction( &bot.forward, 1.0F, "AvoidBullets() forward" );
				break;
			case 20:
				BOTAI_SetAction( &bot.up, 1.0F, "AvoidBullets() up" );
				BOTAI_SetAction( &bot.forward, 1.0F, "AvoidBullets() forward" );
				break;
			case 21:
				BOTAI_SetAction( &bot.up, -1.0F, "AvoidBullets() down" );
				BOTAI_SetAction( &bot.forward, 1.0F, "AvoidBullets() forward" );
				break;
			case 22:
				BOTAI_SetAction( &bot.up, 1.0F, "AvoidBullets() up" );
				BOTAI_SetAction( &bot.right, 1.0F, "AvoidBullets() right" );
				BOTAI_SetAction( &bot.forward, 1.0F, "AvoidBullets() forward" );
				break;
			case 23:
				BOTAI_SetAction( &bot.up, 1.0F, "AvoidBullets() up" );
				BOTAI_SetAction( &bot.right, -1.0F, "AvoidBullets() left" );
				BOTAI_SetAction( &bot.forward, 1.0F, "AvoidBullets() forward" );
				break;
			case 24:
				BOTAI_SetAction( &bot.up, -1.0F, "AvoidBullets() down" );
				BOTAI_SetAction( &bot.right, 1.0F, "AvoidBullets() right" );
				BOTAI_SetAction( &bot.forward, 1.0F, "AvoidBullets() forward" );
				break;
			case 25:
				BOTAI_SetAction( &bot.up, -1.0F, "AvoidBullets() down" );
				BOTAI_SetAction( &bot.right, -1.0F, "AvoidBullets() left" );
				BOTAI_SetAction( &bot.forward, 1.0F, "AvoidBullets() forward" );
				break;

			default: DebugPrintf("invalid move: %d\n", AvoidMove); // should never get here
					 break;
		}
	}
	else
		AvoidMove = -1;

	if(AvoidMove < 0 && DEBUG_AVOIDANCE)
		DebugPrintf("safe......................\n");
}

int BOTAI_CenterCheck( int move )
{
	int a = 0;
	int b = 0;

	switch(move)
	{
		case 0: case 1: a = 0; b = 1; break;
		case 2: case 3: a = 2; b = 3; break;
		case 4: case 7: a = 4; b = 7; break;
		case 5: case 6: a = 5; b = 6; break;
		case 8: case 9: a = 8; b = 9; break;
		case 10: case 19: a = 10; b = 19; break;
		case 11: case 18: a = 11; b = 18; break;
		case 12: case 21: a = 12; b = 21; break;
		case 13: case 20: a = 13; b = 20; break;
		case 14: case 25: a = 14; b = 25; break;
		case 15: case 24: a = 15; b = 24; break;
		case 16: case 23: a = 16; b = 23; break;
		case 17: case 22: a = 17; b = 22; break;
	}

	if( DistWall[a] < DistWall[b] + 100.0F && DistWall[a] > DistWall[b] - 100.0F )
	{
		if( WhenWillHitSlide[a] < 0.0F && WhenWillHitSlide[b] < 0.0F )	
			return a;
	}

	return move;
}

void BOTAI_SelectWeapon( bool MineTarget )
{
	if( MineTarget )
	{
		if( PrimaryWeaponsGot[ LASER ] && !LaserOverheated) 	
			Ships[WhoIAm].Primary = LASER;
		else if( GeneralAmmo > 0.0F )
			Ships[WhoIAm].Primary = PULSAR;
		else if( PrimaryWeaponsGot[ PYROLITE_RIFLE ] && PyroliteAmmo > 0.0F )
			Ships[WhoIAm].Primary = PYROLITE_RIFLE;
		else if( PrimaryWeaponsGot[ SUSS_GUN ] && SussGunAmmo > 0.0F )
			Ships[WhoIAm].Primary = SUSS_GUN;
		else
			Ships[WhoIAm].Primary = PULSAR;
	}

	else if(TargetShipID > -1)
	{
		// target is weak and i have lasers, finish him!
		if(TargetShipHealth < 64 && PrimaryWeaponsGot[ LASER ] && !LaserOverheated)
			Ships[WhoIAm].Primary = LASER;
		// use pyro if they're close
		else if(PrimaryWeaponsGot[ PYROLITE_RIFLE ] && TargetShipDistance < 1000.0F)
			Ships[WhoIAm].Primary = PYROLITE_RIFLE;
		// use lasers if they're far away or if they're closer and i don't have trojax
		else if( (PrimaryWeaponsGot[ LASER ] && !LaserOverheated && TargetShipDistance > 1500.0F)
				|| (PrimaryWeaponsGot[ LASER ] && !LaserOverheated && !PrimaryWeaponsGot[ TROJAX ]) )
			Ships[WhoIAm].Primary = LASER;
		// use trojax
		else if(PrimaryWeaponsGot[ TROJAX ])
			Ships[WhoIAm].Primary = TROJAX;
		// last choice pulsars
		else
			Ships[WhoIAm].Primary = PULSAR;
	}
}

void BOTAI_AimAtTargetShip()
{
	VECTOR NewPos;

	if(TargetShipID > -1)
	{
		// make a linear prediction
		BOTAI_LookAhead( accuracy, &Ships[WhoIAm].Object.Pos, TargetShipID, &NewPos, PrimaryWeaponAttribs[ Ships[ WhoIAm ].Primary ].Speed[ Ships[ WhoIAm ].Object.PowerLevel ]);

		// aim at the target
		if(BOTAI_AimAtTarget( &Ships[WhoIAm].Object.FinalInvMat, &Ships[WhoIAm].Object.Pos, &NewPos ))
			accuracy = Random_Range_Float(2.0F) - 1.0F;
	}
}

void BOTAI_ShootAtTargetShip()
{
	if(TargetShipID > -1)
	{
		// if clear line of sight, shoot primary at target
		if( BOTAI_ClearLOS(&Ships[WhoIAm].Object.Pos, Ships[WhoIAm].Object.Group, &Ships[TargetShipID].Object.Pos))
		{
			// reset node network flag
			CurrentNode = -1;

			// fire primary if i wont hit any team members
			if(!FriendlyFire) 
			{		
				// tap shot trojax
				if(Ships[WhoIAm].Primary == TROJAX && PowerLevel > 10.0F)
					BOTAI_SetAction( &bot.fire_primary, 0.0F, "ShootAtTargetShip() fire primary off" );
				// shoot primary
				else
					BOTAI_SetAction( &bot.fire_primary, 1.0F, "ShootAtTargetShip() fire primary" );
			}
		}

		// shoot any missiles we have
		if(!FriendlyFire)
			BOTAI_FireMissiles();	
	}
}

bool BOTAI_SafeToMove()
{
	if(AllSensorsClear && AvoidMove < 0 && HomingMissile < 0 && IFiredTitan < 0.0F)
		return true;
	else
	{
		if(!AllSensorsClear)
			DebugPrintf("not all clear\n");
		DebugPrintf("avoid = %d\n", AvoidMove);
		return false;
	}
}

bool BOTAI_MaintainDistanceToTargetShip()
{
	float DesiredDistance;

	if( TargetShipID < 0 )
		return false;

	// if i can see the target ship
	if( BOTAI_ClearLOS(&Ships[WhoIAm].Object.Pos, Ships[WhoIAm].Object.Group, &Ships[TargetShipID].Object.Pos) )
	{
		// set the distance wanted for short range weapons
		if(Ships[WhoIAm].Primary == PYROLITE_RIFLE || Ships[WhoIAm].Primary == SUSS_GUN)
			DesiredDistance = 800.0F;
		// all other weapons
		//else if( BOTAI_ComparativeEnemyStrength() > 0 ) // get in their face if we're stronger
		//	DesiredDistance = 500.0F;
		else
			DesiredDistance = 1500.0F;

		// too close so move back
		if(TargetShipDistance < DesiredDistance && DistWall[9] > 200.0F)
			BOTAI_SetAction( &bot.forward, -1.0F, "MaintainDistanceToTargetShip() reverse" );
		// too far so move forward
		else if (TargetShipDistance > DesiredDistance && DistWall[8] > 200.0F)
			BOTAI_SetAction( &bot.forward, 1.0F, "MaintainDistanceToTargetShip() forward" );

		// move right towards the middle
		if( DistWall[0] > DistWall[1] && DistWall[0] > 100.0F )
			BOTAI_SetAction( &bot.right, 1.0F, "MaintainDistanceToTargetShip() right" );
		// move left towards the middle
		else if( DistWall[1] > 100.0F )
			BOTAI_SetAction( &bot.right, -1.0F, "MaintainDistanceToTargetShip() left" );

		// move up towards the middle
		if( DistWall[2] > DistWall[3] && DistWall[2] > 100.0F )
			BOTAI_SetAction( &bot.up, 1.0F, "MaintainDistanceToTargetShip() up" );
		// move down towards the middle
		else if( DistWall[3] > 100.0F )
			BOTAI_SetAction( &bot.up, -1.0F, "MaintainDistanceToTargetShip() down" );

		return true;
	}
	else
		return false;
}

bool BOTAI_CheckForGravgons( VECTOR * Pos )
{
	u_int16_t i;
	float DistToGravgon;

	for( i = FirstModelUsed; i != (u_int16_t) -1; i = Models[i].Prev )
	{
		if( Models[i].Func == MODFUNC_Scale )
		{
			DistToGravgon = DistanceVector2Vector( Pos, &Models[i].Pos );

			if( DistToGravgon < ( BALL_RADIUS * Models[i].MaxScale ) + ( SHIP_RADIUS * 8.0F ) )
				return true;
		}
	}
	return false;
}

int BOTAI_ComparativeEnemyStrength()
{
	int MyStrength = 0;
	int EnemyStrength = 0;

	if( TargetShipID < 0 )
		return 0;
	else
	{
		MyStrength = BOTAI_ShipStrength(WhoIAm);
		EnemyStrength =  BOTAI_ShipStrength(TargetShipID);
		DebugPrintf("my strength = %d enemy strength = %d\n", MyStrength, EnemyStrength);
		return MyStrength - EnemyStrength;
	}
}

// rewrite to account for the ship inventory as it collects weapons
int BOTAI_ShipStrength( int i )
{
	int Strength = 0;
	int p;
	float dist;

	if( TargetShipID < 0 )
		return 0;

	if( Ships[i].InvulTimer > 0.0F )
		return 1000000;

	if( Ships[i].Object.Flags & SHIP_Scattered_Bit )
		return 10;

	dist = DistanceVector2Vector( &Ships[WhoIAm].Object.Pos, &Ships[TargetShipID].Object.Pos );

	p = Ships[i].Object.PowerLevel + 1;

	switch( Ships[i].Primary )
	{
		case TROJAX: 
			if( dist > 1500 ) Strength += 70*p;
			else Strength += 90*p;
			break;

		case PYROLITE_RIFLE:
			if( dist < 1000 ) Strength += 100*p;
			else Strength += 10*p;
			break;

		case TRANSPULSE_CANNON:
			if( dist > 1000 ) Strength += 70*p;
			else Strength += 40*p;
			break;

		case SUSS_GUN:
			if( dist > 1000 ) Strength += 10*p;
			else Strength += 40*p;
			break;

		case LASER:
			if( dist > 1500 ) Strength += 80*p;
			else Strength += 60*p;
			break;

		case PULSAR:
			if( dist > 1500 ) Strength += 60*p;
			else Strength += 50*p;
			break;
	}

	if( Ships[i].SuperNashramTimer == 0.0F )
		Strength *= 3;
	else if( Ships[i].Object.Flags & SHIP_Turbo_Bit )
		Strength *= 2;
  
	// secondary code currently useless; only triggered AFTER enemy player has fired the missile
  /*  switch( Ships[i].Secondary )
	{
		case MUGMISSILE:
		case THIEFMISSILE:
			if( dist > 1000 ) Strength += 30;
			else Strength += 60;
			break;

		case SOLARISMISSILE:
		case MULTIPLEMISSILE:
			if( dist > 1500 ) Strength += 150;
			else Strength += 210;
			break;

		case SCATTERMISSILE:
			if( dist > 1500 ) Strength += 180;
			else Strength += 240;
			break;

		case GRAVGONMISSILE:
			Strength += 30;
			break;

		case TITANSTARMISSILE:
			if( dist > 2000 ) Strength += 120;
			else Strength += 300;
			break;
	} */
	if( i == WhoIAm )
		Strength = (int) (Strength * (Ships[WhoIAm].Object.Hull+Ships[WhoIAm].Object.Shield) / (MAX_HULL+MAX_SHIELD));
	else
		Strength = (int) (Strength * (PlayerHealths[i].Hull+PlayerHealths[i].Shield) / (MAX_HULL+MAX_SHIELD));
	return Strength;
}

#endif // LUA_BOT
