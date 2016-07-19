/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "CBasePlayer.h"

#include "gamerules/GameRules.h"

extern int gmsgFlashlight;
extern int gmsgScoreInfo;

// 
// PlayerUse - handles USE keypress
//
void CBasePlayer::PlayerUse()
{
	if( IsObserver() )
		return;

	// Was use pressed or released?
	if( !( ( pev->button | m_afButtonPressed | m_afButtonReleased ) & IN_USE ) )
		return;

	// Hit Use on a train?
	if( m_afButtonPressed & IN_USE )
	{
		if( m_pTank != NULL )
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use( this, this, USE_OFF, 0 );
			m_pTank = NULL;
			return;
		}
		else
		{
			if( m_afPhysicsFlags & PFLAG_ONTRAIN )
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
			else
			{	// Start controlling the train!
				CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );

				if( pTrain && !( pev->button & IN_JUMP ) && FBitSet( pev->flags, FL_ONGROUND ) && ( pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE ) && pTrain->OnControls( this ) )
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed( pTrain->pev->speed, pTrain->pev->impulse );
					m_iTrain |= TRAIN_NEW;
					EMIT_SOUND( ENT( pev ), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM );
					return;
				}
			}
		}
	}

	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	Vector		vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;

	UTIL_MakeVectors( pev->v_angle );// so we know which way we are facing

	while( ( pObject = UTIL_FindEntityInSphere( pObject, pev->origin, PLAYER_USE_SEARCH_RADIUS ) ) != NULL )
	{

		if( pObject->ObjectCaps() & ( FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE ) )
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = ( VecBModelOrigin( pObject ) - ( pev->origin + pev->view_ofs ) );

			// This essentially moves the origin of the target to the corner nearest the player to test to see 
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox( vecLOS, pObject->pev->size * 0.5 );

			flDot = DotProduct( vecLOS, gpGlobals->v_forward );
			if( flDot > flMaxDot )
			{// only if the item is in front of the user
				pClosest = pObject;
				flMaxDot = flDot;
				//				ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
			}
			//			ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
		}
	}
	pObject = pClosest;

	// Found an object
	if( pObject )
	{
		//!!!UNDONE: traceline here to prevent USEing buttons through walls			
		int caps = pObject->ObjectCaps();

		if( m_afButtonPressed & IN_USE )
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM );

		if( ( ( pev->button & IN_USE ) && ( caps & FCAP_CONTINUOUS_USE ) ) ||
			( ( m_afButtonPressed & IN_USE ) && ( caps & ( FCAP_IMPULSE_USE | FCAP_ONOFF_USE ) ) ) )
		{
			if( caps & FCAP_CONTINUOUS_USE )
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use( this, this, USE_SET, 1 );
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if( ( m_afButtonReleased & IN_USE ) && ( pObject->ObjectCaps() & FCAP_ONOFF_USE ) )	// BUGBUG This is an "off" use
		{
			pObject->Use( this, this, USE_SET, 0 );
		}
	}
	else
	{
		if( m_afButtonPressed & IN_USE )
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM );
	}
}

void CBasePlayer::Jump()
{
	if( FBitSet( pev->flags, FL_WATERJUMP ) )
		return;

	if( pev->waterlevel >= WATERLEVEL_WAIST )
	{
		return;
	}

	// jump velocity is sqrt( height * gravity * 2)

	// If this isn't the first frame pressing the jump button, break out.
	if( !FBitSet( m_afButtonPressed, IN_JUMP ) )
		return;         // don't pogo stick

	if( !( pev->flags & FL_ONGROUND ) || !pev->groundentity )
	{
		return;
	}

	// many features in this function use v_forward, so makevectors now.
	UTIL_MakeVectors( pev->angles );

	// ClearBits(pev->flags, FL_ONGROUND);		// don't stairwalk

	SetAnimation( PLAYER_JUMP );

	if( m_fLongJump &&
		( pev->button & IN_DUCK ) &&
		( pev->flDuckTime > 0 ) &&
		pev->velocity.Length() > 50 )
	{
		SetAnimation( PLAYER_SUPERJUMP );
	}

	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	CBaseEntity* pGround = Instance( pev->groundentity );
	if( pGround && ( pGround->pev->flags & FL_CONVEYOR ) )
	{
		//Note: basevelocity is set by the physics code. It accounts for conveyors. - Solokiller
		pev->velocity = pev->velocity + pev->basevelocity;
	}
}

void CBasePlayer::Duck()
{
	if( pev->button & IN_DUCK )
	{
		if( m_IdealActivity != ACT_LEAP )
		{
			SetAnimation( PLAYER_WALK );
		}
	}
}

void CBasePlayer::Pain()
{
	//sound randomizer
	const float flRndSound = RANDOM_FLOAT( 0, 1 );

	if( flRndSound <= 0.33 )
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM );
	else if( flRndSound <= 0.66 )
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM );
	else
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM );
}

void CBasePlayer::DeathSound()
{
	// water death sounds
	/*
	if (pev->waterlevel == WATERLEVEL_HEAD)
	{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE);
	return;
	}
	*/

	// temporarily using pain sounds for death sounds
	switch( RANDOM_LONG( 1, 5 ) )
	{
	case 1:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM );
		break;
	case 3:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM );
		break;
	}

	// play one of the suit death alarms
	EMIT_GROUPNAME_SUIT( ENT( pev ), "HEV_DEAD" );
}

bool CBasePlayer::IsOnLadder() const
{
	return ( pev->movetype == MOVETYPE_FLY );
}

bool CBasePlayer::FlashlightIsOn() const
{
	return FBitSet( pev->effects, EF_DIMLIGHT ) != 0;
}

void CBasePlayer::FlashlightTurnOn()
{
	if( !g_pGameRules->FAllowFlashlight() )
	{
		return;
	}

	if( ( pev->weapons & ( 1 << WEAPON_SUIT ) ) )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM );
		SetBits( pev->effects, EF_DIMLIGHT );
		MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( m_iFlashBattery );
		MESSAGE_END();

		m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
	}
}

void CBasePlayer::FlashlightTurnOff()
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM );
	ClearBits( pev->effects, EF_DIMLIGHT );
	MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( m_iFlashBattery );
	MESSAGE_END();

	m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
}

void CBasePlayer::AddPoints( int score, const bool bAllowNegativeScore )
{
	// Positive score always adds
	if( score < 0 )
	{
		if( !bAllowNegativeScore )
		{
			if( pev->frags < 0 )		// Can't go more negative
				return;

			if( -score > pev->frags )	// Will this go negative?
			{
				score = -pev->frags;		// Sum will be 0
			}
		}
	}

	pev->frags += score;

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
	WRITE_BYTE( ENTINDEX( edict() ) );
		WRITE_SHORT( pev->frags );
		WRITE_SHORT( m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( g_pGameRules->GetTeamIndex( m_szTeamName ) + 1 );
	MESSAGE_END();
}


void CBasePlayer::AddPointsToTeam( int score, const bool bAllowNegativeScore )
{
	const int index = entindex();

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if( pPlayer && i != index )
		{
			if( g_pGameRules->PlayerRelationship( this, pPlayer ) == GR_TEAMMATE )
			{
				pPlayer->AddPoints( score, bAllowNegativeScore );
			}
		}
	}
}

void CBasePlayer::EnableControl( const bool fControl )
{
	if( !fControl )
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;
}

// Set the activity based on an event or current state
void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;
	float speed;
	char szAnim[ 64 ];

	speed = pev->velocity.Length2D();

	if( pev->flags & FL_FROZEN )
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	switch( playerAnim )
	{
	case PLAYER_JUMP:
		m_IdealActivity = ACT_HOP;
		break;

	case PLAYER_SUPERJUMP:
		m_IdealActivity = ACT_LEAP;
		break;

	case PLAYER_DIE:
		m_IdealActivity = ACT_DIESIMPLE;
		m_IdealActivity = GetDeathActivity();
		break;

	case PLAYER_ATTACK1:
		switch( m_Activity )
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_HOP:
		case ACT_LEAP:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
		break;
	case PLAYER_IDLE:
	case PLAYER_WALK:
		if( !FBitSet( pev->flags, FL_ONGROUND ) && ( m_Activity == ACT_HOP || m_Activity == ACT_LEAP ) )	// Still jumping
		{
			m_IdealActivity = m_Activity;
		}
		else if( pev->waterlevel > WATERLEVEL_FEET )
		{
			if( speed == 0 )
				m_IdealActivity = ACT_HOVER;
			else
				m_IdealActivity = ACT_SWIM;
		}
		else
		{
			m_IdealActivity = ACT_WALK;
		}
		break;
	}

	switch( m_IdealActivity )
	{
	case ACT_HOVER:
	case ACT_LEAP:
	case ACT_SWIM:
	case ACT_HOP:
	case ACT_DIESIMPLE:
	default:
		if( m_Activity == m_IdealActivity )
			return;
		m_Activity = m_IdealActivity;

		animDesired = LookupActivity( m_Activity );
		// Already using the desired animation?
		if( pev->sequence == animDesired )
			return;

		pev->gaitsequence = 0;
		pev->sequence = animDesired;
		pev->frame = 0;
		ResetSequenceInfo();
		return;

	case ACT_RANGE_ATTACK1:
		if( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
			strcpy( szAnim, "crouch_shoot_" );
		else
			strcpy( szAnim, "ref_shoot_" );
		strcat( szAnim, m_szAnimExtention );
		animDesired = LookupSequence( szAnim );
		if( animDesired == -1 )
			animDesired = 0;

		if( pev->sequence != animDesired || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		if( !m_fSequenceLoops )
		{
			pev->effects |= EF_NOINTERP;
		}

		m_Activity = m_IdealActivity;

		pev->sequence = animDesired;
		ResetSequenceInfo();
		break;

	case ACT_WALK:
		if( m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished )
		{
			if( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
				strcpy( szAnim, "crouch_aim_" );
			else
				strcpy( szAnim, "ref_aim_" );
			strcat( szAnim, m_szAnimExtention );
			animDesired = LookupSequence( szAnim );
			if( animDesired == -1 )
				animDesired = 0;
			m_Activity = ACT_WALK;
		}
		else
		{
			animDesired = pev->sequence;
		}
	}

	if( FBitSet( pev->flags, FL_DUCKING ) )
	{
		if( speed == 0 )
		{
			pev->gaitsequence = LookupActivity( ACT_CROUCHIDLE );
			// pev->gaitsequence	= LookupActivity( ACT_CROUCH );
		}
		else
		{
			pev->gaitsequence = LookupActivity( ACT_CROUCH );
		}
	}
	else if( speed > 220 )
	{
		pev->gaitsequence = LookupActivity( ACT_RUN );
	}
	else if( speed > 0 )
	{
		pev->gaitsequence = LookupActivity( ACT_WALK );
	}
	else
	{
		// pev->gaitsequence	= LookupActivity( ACT_WALK );
		pev->gaitsequence = LookupSequence( "deep_idle" );
	}


	// Already using the desired animation?
	if( pev->sequence == animDesired )
		return;

	//ALERT( at_console, "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	pev->sequence = animDesired;
	pev->frame = 0;
	ResetSequenceInfo();
}