// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"

qboolean	G_SpawnString( const char *key, const char *defaultString, char **out ) {
	int		i;

	if ( !level.spawning ) {
		*out = (char *)defaultString;
//		G_Error( "G_SpawnString() called while not spawning" );
	}

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		if ( !Q_stricmp( key, level.spawnVars[i][0] ) ) {
			*out = level.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

qboolean	G_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atof( s );
	return present;
}

qboolean	G_SpawnInt( const char *key, const char *defaultString, int *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean	G_SpawnVector( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	Q_sscanf( s, "%f %f %f", &out[0], &out[1], &out[2] );
	return present;
}



//
// fields are needed for spawning from the entity string
//
typedef enum {
	F_INT, 
	F_FLOAT,
	F_LSTRING,			// string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,			// string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
	F_ENTITY,			// index on disk, pointer in memory
	F_ITEM,				// index on disk, pointer in memory
	F_CLIENT,			// index on disk, pointer in memory
	F_IGNORE
} fieldtype_t;

typedef struct
{
	const char *name;
	int		ofs;
	fieldtype_t	type;
	//int		flags;
} field_t;

const field_t fields[] = {
	{"classname", FOFS(classname), F_LSTRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"model", FOFS(model), F_LSTRING},
	{"model2", FOFS(model2), F_LSTRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"target", FOFS(target), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"message", FOFS(message), F_LSTRING},
	{"team", FOFS(team), F_LSTRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"random", FOFS(random), F_FLOAT},
	{"count", FOFS(count), F_INT},
	{"health", FOFS(health), F_INT},
	{"light", 0, F_IGNORE},
	{"dmg", FOFS(damage), F_INT},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"angle", FOFS(s.angles), F_ANGLEHACK},
	{"targetShaderName", FOFS(targetShaderName), F_LSTRING},
	{"targetShaderNewName", FOFS(targetShaderNewName), F_LSTRING},

	{NULL}
};


typedef struct {
	char	*name;
	void	(*spawn)(gentity_t *ent);
} spawn_t;

void SP_info_player_start (gentity_t *ent);
void SP_info_player_deathmatch (gentity_t *ent);
void SP_info_player_intermission (gentity_t *ent);
void SP_info_firstplace(gentity_t *ent);
void SP_info_secondplace(gentity_t *ent);
void SP_info_thirdplace(gentity_t *ent);
void SP_info_podium(gentity_t *ent);

void SP_func_plat (gentity_t *ent);
void SP_func_static (gentity_t *ent);
void SP_func_rotating (gentity_t *ent);
void SP_func_bobbing (gentity_t *ent);
void SP_func_pendulum( gentity_t *ent );
void SP_func_button (gentity_t *ent);
void SP_func_door (gentity_t *ent);
void SP_func_train (gentity_t *ent);
void SP_func_timer (gentity_t *self);

void SP_trigger_always (gentity_t *ent);
void SP_trigger_multiple (gentity_t *ent);
void SP_trigger_push (gentity_t *ent);
void SP_trigger_teleport (gentity_t *ent);
void SP_trigger_hurt (gentity_t *ent);

void SP_target_remove_powerups( gentity_t *ent );
void SP_target_give (gentity_t *ent);
void SP_target_delay (gentity_t *ent);
void SP_target_speaker (gentity_t *ent);
void SP_target_print (gentity_t *ent);
void SP_target_laser (gentity_t *self);
void SP_target_character (gentity_t *ent);
void SP_target_score( gentity_t *ent );
void SP_target_teleporter( gentity_t *ent );
void SP_target_relay (gentity_t *ent);
void SP_target_kill (gentity_t *ent);
void SP_target_position (gentity_t *ent);
void SP_target_location (gentity_t *ent);
void SP_target_push (gentity_t *ent);

void SP_light (gentity_t *self);
void SP_info_null (gentity_t *self);
void SP_info_notnull (gentity_t *self);
void SP_info_camp (gentity_t *self);
void SP_path_corner (gentity_t *self);

void SP_misc_teleporter_dest (gentity_t *self);
void SP_misc_model(gentity_t *ent);
void SP_misc_portal_camera(gentity_t *ent);
void SP_misc_portal_surface(gentity_t *ent);

void SP_shooter_rocket( gentity_t *ent );
void SP_shooter_plasma( gentity_t *ent );
void SP_shooter_grenade( gentity_t *ent );

void SP_team_CTF_redplayer( gentity_t *ent );
void SP_team_CTF_blueplayer( gentity_t *ent );

void SP_team_CTF_redspawn( gentity_t *ent );
void SP_team_CTF_bluespawn( gentity_t *ent );


void SP_team_blueobelisk( gentity_t *ent );
void SP_team_redobelisk( gentity_t *ent );
void SP_team_neutralobelisk( gentity_t *ent );

void SP_item_botroam( gentity_t *ent ) {};

spawn_t	spawns[] = {
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{"info_player_start", SP_info_player_start},
	{"info_player_deathmatch", SP_info_player_deathmatch},
	{"info_player_intermission", SP_info_player_intermission},
	{"info_null", SP_info_null},
	{"info_notnull", SP_info_notnull},		// use target_position instead
	{"info_camp", SP_info_camp},

	{"func_plat", SP_func_plat},
	{"func_button", SP_func_button},
	{"func_door", SP_func_door},
	{"func_static", SP_func_static},
	{"func_rotating", SP_func_rotating},
	{"func_bobbing", SP_func_bobbing},
	{"func_pendulum", SP_func_pendulum},
	{"func_train", SP_func_train},
	{"func_group", SP_info_null},
	{"func_timer", SP_func_timer},			// rename trigger_timer?

	// Triggers are brush objects that cause an effect when contacted
	// by a living player, usually involving firing targets.
	// While almost everything could be done with
	// a single trigger class and different targets, triggered effects
	// could not be client side predicted (push and teleport).
	{"trigger_always", SP_trigger_always},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_push", SP_trigger_push},
	{"trigger_teleport", SP_trigger_teleport},
	{"trigger_hurt", SP_trigger_hurt},

	// targets perform no action by themselves, but must be triggered
	// by another entity
	{"target_give", SP_target_give},
	{"target_remove_powerups", SP_target_remove_powerups},
	{"target_delay", SP_target_delay},
	{"target_speaker", SP_target_speaker},
	{"target_print", SP_target_print},
	{"target_laser", SP_target_laser},
	{"target_score", SP_target_score},
	{"target_teleporter", SP_target_teleporter},
	{"target_relay", SP_target_relay},
	{"target_kill", SP_target_kill},
	{"target_position", SP_target_position},
	{"target_location", SP_target_location},
	{"target_push", SP_target_push},

	{"light", SP_light},
	{"path_corner", SP_path_corner},

	{"misc_teleporter_dest", SP_misc_teleporter_dest},
	{"misc_model", SP_misc_model},
	{"misc_portal_surface", SP_misc_portal_surface},
	{"misc_portal_camera", SP_misc_portal_camera},

	{"shooter_rocket", SP_shooter_rocket},
	{"shooter_grenade", SP_shooter_grenade},
	{"shooter_plasma", SP_shooter_plasma},

	{"team_CTF_redplayer", SP_team_CTF_redplayer},
	{"team_CTF_blueplayer", SP_team_CTF_blueplayer},

	{"team_CTF_redspawn", SP_team_CTF_redspawn},
	{"team_CTF_bluespawn", SP_team_CTF_bluespawn},


	{"team_redobelisk", SP_team_redobelisk},
	{"team_blueobelisk", SP_team_blueobelisk},
	{"team_neutralobelisk", SP_team_neutralobelisk},

	{"item_botroam", SP_item_botroam},

	{0, 0}
};

#define ADJUST_AREAPORTAL() \
	if(ent->s.eType == ET_MOVER) \
	{ \
		trap_LinkEntity(ent);					\
		trap_AdjustAreaPortalState(ent, qtrue); \
	}

// Function to check for collision based on position
qboolean CheckCollision(gentity_t *ent, const char *classname1, const char *classname2) {
    gentity_t *other;
	int i;
    for (i = MAX_CLIENTS; i < level.num_entities; i++) {
        other = &g_entities[i];
        if (( !Q_stricmp(other->classname, classname1) || !Q_stricmp(other->classname, classname2) ) &&
            VectorCompare(ent->s.origin, other->s.origin)) {
            return qtrue;
        }
    }
    return qfalse;
}

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_CallSpawn( gentity_t *ent ) {
	spawn_t	*s;
	gitem_t	*item;
	int			i, j, k;
	int			otherIsInTaitems, compIsInTaitems, num_taitems;
	gentity_t	*comp;
	gentity_t	*other;
	const char *checkClassname1, *checkClassname2;

	//FIXME:
	// Check for overlapping specified items

	const char *taitems[] = {
		"weapon_chaingun",
		"weapon_nailgun",
		"weapon_prox_launcher",
		"ammo_belt",
		"ammo_mines",
		"ammo_nails",
		"holdable_invulnerability",
		"holdable_kamikaze"
	};

	num_taitems = ARRAY_LEN(taitems);

	/* if ( g_disableHMG.integer ) {
		if ( !Q_stricmp( ent->classname, "weapon_hmg" )) {
			ent->classname = "weapon_shotgun";
		}
		if ( !Q_stricmp(ent->classname, "ammo_hmg" )) {
			ent->classname = "ammo_shells";
		}
	} */

	// Existing code with collision check
	if ( g_disableHMG.integer ) {
		if ( !Q_stricmp(ent->classname, "weapon_hmg") && CheckCollision(ent, "weapon_hmg", "weapon_shotgun") ) {
			G_FreeEntity(ent);
		}
		else if ( !Q_stricmp(ent->classname, "ammo_hmg") && CheckCollision(ent, "ammo_hmg", "ammo_shells") ) {
			G_FreeEntity(ent);
		}
	} else {
		if ( !Q_stricmp(ent->classname, "weapon_shotgun") && CheckCollision(ent, "weapon_shotgun", "weapon_hmg") ) {
			G_FreeEntity(ent);
		}
		else if ( !Q_stricmp(ent->classname, "ammo_shells") && CheckCollision(ent, "ammo_shells", "ammo_hmg") ) {
			G_FreeEntity(ent);
		}
	}

	if ( g_taitems.integer ) {
		//FIXME:
		for (i = MAX_CLIENTS; i < level.num_entities; i++) {
			other = &g_entities[i];

			// Check if 'other' is in taitems
			otherIsInTaitems = 0;
			for (k = 0; k < num_taitems; k++) {
				if (!Q_stricmp(other->classname, taitems[k])) {
					otherIsInTaitems = 1;
					break;
				}
			}

			if (otherIsInTaitems) {
				for (j = MAX_CLIENTS; j < level.num_entities; j++) {
					if (i == j) {
						continue;
					}

					comp = &g_entities[j];

					if (VectorCompare(other->r.currentOrigin, comp->r.currentOrigin)) {
						// Check if 'comp' is not in taitems before removing it
						compIsInTaitems = 0;
						for (k = 0; k < num_taitems; k++) {
							if (!Q_stricmp(comp->classname, taitems[k])) {
								compIsInTaitems = 1;
								break;
							}
						}

						if (!g_disableHMG.integer && !Q_stricmp(comp->classname, "weapon_hmg") || !Q_stricmp(comp->classname, "ammo_hmg")) {
							ADJUST_AREAPORTAL();
							G_FreeEntity( other );
						}
						else if (!compIsInTaitems) {
							ADJUST_AREAPORTAL();
							G_FreeEntity(comp);
						}
					}
				}
			}
		}
	} else {
		for (i = MAX_CLIENTS; i < level.num_entities; i++) {
			other = &g_entities[i];
			for ( j = 0; j < num_taitems; j++ ) {
				if (!Q_stricmp(other->classname, taitems[j])) {
					ADJUST_AREAPORTAL();
					G_FreeEntity(other);
					break;
				}
			}
		}
	}

	// check ammopack settings
	if (g_ammopack.integer) {
		if (!Q_stricmp(ent->classname, "ammo_pack")) {
			//pass add it
		} else if (!Q_stricmpn(ent->classname, "ammo_", 5)) {
			// ignore other types of ammo
			ADJUST_AREAPORTAL();
			G_FreeEntity( ent );
			return;
		}
	} else {
		if (!Q_stricmp(ent->classname, "ammo_pack")) {
			ADJUST_AREAPORTAL();
			G_FreeEntity( ent );
			return;
		}
	}

	// Check for overlapping armor items
    if (!Q_stricmp(ent->classname, "item_armor_combat")) {
        for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
			for ( j = MAX_CLIENTS; j < level.num_entities; j++ ) {
				comp = &g_entities[j];
				other = &g_entities[i];
            	if (other == comp) {
                	continue;
            	}
            	if (!Q_stricmp(other->classname, "item_armor_jacket") || !Q_stricmp(other->classname, "item_armor_green")) {
                	// Disable item_armor_combat if item_armor_jacket exists at the same coordinates
					if (VectorCompare(comp->r.currentOrigin, other->r.currentOrigin)) {
                    	ADJUST_AREAPORTAL();
                    	G_FreeEntity(ent);
                    	return;
                	}
            	}
			}
        }
    }

	// enable/disable runes
	if (!g_runes.integer) {
		if (!Q_stricmp(ent->classname, "item_guard") || !Q_stricmp(ent->classname, "item_doubler") || !Q_stricmp(ent->classname, "item_ammoregen") || !Q_stricmp(ent->classname, "item_scout")) {
			ADJUST_AREAPORTAL();
			G_FreeEntity( ent );
			return;
		}
	}

	if ( !ent->classname ) {
		G_Printf ("G_CallSpawn: NULL classname\n");
		return qfalse;
	}

	if (!Q_stricmp( ent->classname, "item_armor_green" )) {
		ent->classname = "item_armor_jacket";
	}

	// check item spawn functions
	for ( item=bg_itemlist+1 ; item->classname ; item++ ) {
		if ( !strcmp(item->classname, ent->classname) ) {
			G_SpawnItem( ent, item );
			return qtrue;
		}
	}

	// check normal spawn functions
	for ( s=spawns ; s->name ; s++ ) {
		if ( !strcmp(s->name, ent->classname) ) {
			// found it
			s->spawn(ent);
			return qtrue;
		}
	}
	G_Printf ("%s doesn't have a spawn function\n", ent->classname);
	return qfalse;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString( const char *string ) {
	char	*newb, *new_p;
	int		i,l;
	
	l = (int)strlen(string) + 1;

	newb = G_Alloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( i=0 ; i< l ; i++ ) {
		if (string[i] == '\\' && i < l-1) {
			i++;
			if (string[i] == 'n') {
				*new_p++ = '\n';
			} else {
				*new_p++ = '\\';
			}
		} else {
			*new_p++ = string[i];
		}
	}
	
	return newb;
}




/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void G_ParseField( const char *key, const char *value, gentity_t *ent ) {
	const field_t *f;
	byte	*b;
	float	v;
	vec3_t	vec;

	for ( f=fields ; f->name ; f++ ) {
		if ( !Q_stricmp(f->name, key) ) {
			// found it
			b = (byte *)ent;

			switch( f->type ) {
			case F_LSTRING:
				*(char **)(b+f->ofs) = G_NewString (value);
				break;
			case F_VECTOR:
				Q_sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float *)(b+f->ofs))[0] = vec[0];
				((float *)(b+f->ofs))[1] = vec[1];
				((float *)(b+f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*(int *)(b+f->ofs) = atoi(value);
				break;
			case F_FLOAT:
				*(float *)(b+f->ofs) = atof(value);
				break;
			case F_ANGLEHACK:
				v = atof(value);
				((float *)(b+f->ofs))[0] = 0;
				((float *)(b+f->ofs))[1] = v;
				((float *)(b+f->ofs))[2] = 0;
				break;
			default:
			case F_IGNORE:
				break;
			}
			return;
		}
	}
}

/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
void G_SpawnGEntityFromSpawnVars( void ) {
	gentity_t	*ent;
	int			i;
	char		*s, *value, *gametypeName;
	static char *gametypeNames[] = {"ffa", "tournament", "single", "team", "ctf", "oneflag", "obelisk", "harvester", "teamtournament"};
	
	// get the next free entity
	ent = G_Spawn();

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		G_ParseField( level.spawnVars[i][0], level.spawnVars[i][1], ent );
	}

	// check for "notsingle" flag
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		G_SpawnInt( "notsingle", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	}
	// check for "notteam" flag (GT_FFA, GT_TOURNAMENT, GT_SINGLE_PLAYER)
	if ( g_gametype.integer >= GT_TEAM ) {
		G_SpawnInt( "notteam", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	} else {
		G_SpawnInt( "notfree", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	}


	G_SpawnInt( "notta", "0", &i );
	if ( i ) {
		G_FreeEntity( ent );
		return;
	}

	G_SpawnInt( "notq3a", "0", &i );
	if ( i ) {
		G_FreeEntity( ent );
		return;
	}


	if( G_SpawnString( "gametype", NULL, &value ) ) {
		if( g_gametype.integer >= GT_FFA && g_gametype.integer < GT_MAX_GAME_TYPE ) {
			gametypeName = gametypeNames[g_gametype.integer];

			s = strstr( value, gametypeName );
			if( !s ) {
				G_FreeEntity( ent );
				return;
			}
		}
	}

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) ) {
		G_FreeEntity( ent );
	}
}



/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken( const char *string ) {
	int		l;
	char	*dest;

	l = (int)strlen( string );
	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_VARS_CHARS" );
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l+1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( void ) {
	char		keyname[MAX_TOKEN_CHARS];
	char		com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		G_Error( "G_ParseSpawnVars: found %s when expecting {",com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {	
		// parse key
		if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' ) {
			break;
		}
		
		// parse value	
		if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}
		if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( keyname );
		level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

	return qtrue;
}



/*QUAKED worldspawn (0 0 0) ?

Every map should have exactly one worldspawn.
"music"		music wav file
"gravity"	800 is default gravity
"message"	Text to print during connection process
*/
void SP_worldspawn( void ) {
	char	*s;

	G_SpawnString( "classname", "", &s );
	if ( Q_stricmp( s, "worldspawn" ) ) {
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	// make some data visible to connecting client
	trap_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	trap_SetConfigstring( CS_LEVEL_START_TIME, va("%i", level.startTime ) );

	G_SpawnString( "music", "", &s );
	trap_SetConfigstring( CS_MUSIC, s );

	G_SpawnString( "message", "", &s );
	trap_SetConfigstring( CS_MESSAGE, s );				// map specific message

	trap_SetConfigstring( CS_MOTD, g_motd.string );		// message of the day

	G_SpawnString( "gravity", "800", &s );
	trap_Cvar_Set( "g_gravity", s );

	G_SpawnString( "enableDust", "0", &s );
	trap_Cvar_Set( "g_enableDust", s );

	G_SpawnString( "enableBreath", "0", &s );
	trap_Cvar_Set( "g_enableBreath", s );

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

	g_entities[ENTITYNUM_NONE].s.number = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].classname = "nothing";

	// see if we want a warmup time
	if ( /*g_restarted.integer ||*/ g_gametype.integer == GT_SINGLE_PLAYER ) {
		//trap_Cvar_Set( "g_restarted", "0" );
		level.warmupTime = 0;
		trap_SetConfigstring( CS_WARMUP, "" );
	} else {
		// assume that g_doWarmup is always 1
		level.warmupTime = -1;
		if ( g_warmup.integer > 0 ) {
			trap_SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
		} else {
			trap_SetConfigstring( CS_WARMUP, "" );
		}
		G_LogPrintf( "Warmup:\n" );
	}
}


/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString( void ) {
	// allow calls to G_Spawn*()
	level.spawning = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars() ) {
		G_Error( "SpawnEntities: no entities" );
	}
	SP_worldspawn();

	// parse ents
	while( G_ParseSpawnVars() ) {
		G_SpawnGEntityFromSpawnVars();
	}	

	level.spawning = qfalse;			// any future calls to G_Spawn*() will be errors
}
