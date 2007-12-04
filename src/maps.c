/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id$
 */

#include "g_local.h"

char *maps_list[] =
{
	// episode 1
	"e1m1",
	"e1m2",
	"e1m3",
	"e1m4",
	"e1m5",
	"e1m6",
	"e1m7",
	"e1m8",

	// episode 2
	"e2m1",
	"e2m2",
	"e2m3",
	"e2m4",
	"e2m5",
	"e2m6",
	"e2m7",

	// episode 3
	"e3m1",
	"e3m2",
	"e3m3",
	"e3m4",
	"e3m5",
	"e3m6",
	"e3m7",

	// episode 4
	"e4m1",
	"e4m2",
	"e4m3",
	"e4m4",
	"e4m5",
	"e4m6",
	"e4m7",
	"e4m8",

	// 
	"start",
	"end",

	// DM maps
	"dm1",
	"dm2",
	"dm3",
	"dm4",
	"dm5",
	"dm6"
};

int maps_cnt = sizeof ( maps_list ) / sizeof ( maps_list[0] );

void StuffCustomMaps( gedict_t *p )
{
	int i;
	char map_name[128], key[128];

	for( i = LOCALINFO_MAPS_LIST_START; i <= LOCALINFO_MAPS_LIST_END; i++ )
	{
		snprintf(key, sizeof(key), "%d", i);
		infokey( world, key, map_name, sizeof(map_name) );

		if ( strnull( map_name ) )
			break;

		stuffcmd(p, "alias %s cmd cm %d\n", map_name, i + (1 - LOCALINFO_MAPS_LIST_START));
	}
}

void StuffMainMaps( gedict_t *p )
{
	int i;

	for( i = 0; i < maps_cnt; i++ )
		stuffcmd(p, "alias %s cmd cm %d\n", maps_list[i], -(i + 1));
}

void StuffMaps( gedict_t *p )
{
	StuffMainMaps( p );
	StuffCustomMaps( p );

	G_sprint(p, 2, "Maps downloaded\n" );
}


char *GetMapName(int imp)
{
	int i;

	if ( imp < 0 )
	{
	 // potential from maps_list[]
		i = -imp - 1;

		if ( i >= 0 && i < maps_cnt )
			return maps_list[i];
	}
	else
	{
	 // potential custom map
		i = imp - (1 - LOCALINFO_MAPS_LIST_START);

	 	if ( i >= LOCALINFO_MAPS_LIST_START && i <= LOCALINFO_MAPS_LIST_END )
			return ezinfokey(world, va("%d", i));
	}

	return "";
}

void DoSelectMap(int iMap)
{
	char     *m;
	gedict_t *p;
	int 	 till;
	qboolean isVoted = false;

	if( (till = Q_rint( ( k_matchLess ? 15: 7 ) - g_globalvars.time)) > 0  ) {
		G_sprint(self, 2, "Wait %d second%s!\n", till, count_s(till) );
		return;
	}

	if ( k_matchLess ) {
		if ( cvar("k_no_vote_map") ) {
			G_sprint(self, 2, "Voting map is %s allowed\n", redtext("not"));
			return;
		}

		if ( match_in_progress != 2 )
			return; // u can select map in matchLess mode, but not in countdown
	}
	else if ( match_in_progress )
		return;

	if ( self->ct == ctSpec && !is_adm(self ) ) // only admined specs can select map
		return;

	if ( strnull( m = GetMapName( iMap ) ) )
		return;

	if( (    cvar( "k_lockmap" ) || cvar( "k_master" ) )
		  && !is_adm( self )
      ) {
		G_sprint(self, 2, "MAP IS LOCKED!\n"
						  "You are NOT allowed to change!\n");
		return;
	}

	if ( self->v.map == iMap ) {
		G_sprint(self, 2, "--- your vote is still good ---\n");
		return;
	}

	for ( p = world; (p = find_plr( p )); )
		if ( p->v.map == iMap ) {
			isVoted = true;
			break;
		}

	if( !get_votes( OV_MAP ) ) {
		G_bprint(2, "%s %s %s\n", self->s.v.netname, redtext("suggests map"),m);
	}
	else if ( isVoted ) {
		G_bprint(2, "%s %s %s %s %s\n", self->s.v.netname, redtext("agrees"),
			(CountPlayers() < 3 ? redtext("to") : redtext("on") ), redtext("map"), m);
	}
	else
		G_bprint(2, "%s %s %s\n", self->s.v.netname, redtext("would rather play on"), m);

	self->v.map = k_lastvotedmap = iMap;

	vote_check_map ();
}

void SelectMap()
{
	char	arg_1[1024];

	trap_CmdArgv( 1, arg_1, sizeof( arg_1 ) );

	DoSelectMap ( atoi( arg_1 ) );
}

int IsMapInCycle(char *map)
{
	char newmap[128] = {0}, mapid[128] = {0};
	int i;

	if ( strnull( map ) )
		return 0;

	for ( i = 0; i < 1000; i++ ) {
		snprintf(mapid, sizeof(mapid), "k_ml_%d", i);
		// huge for(), so using trap version instead of lazy but unsafe cvar_string()
		trap_cvar_string( mapid, newmap, sizeof(newmap) );

		if ( strnull( newmap ) ) // end of list
			return 0;

		if ( streq( map, newmap ) ) // ok map found in map list
			return i + 1; // i may be 0, so returning i + 1
	}

	return 0;
}

// map list have now next syntax:
// set k_ml_0 dm6
// set k_ml_1 dm4
// set k_ml_2 dm2
// so this mean we have rotation of maps dm6 dm4 dm2 dm6 dm4 dm2 ...

char *SelectMapInCycle(char *buf, int buf_size)
{
	char newmap[128] = {0}, mapid[128] = {0};
	int i;

	buf[0] = 0;

	if ( (i = IsMapInCycle( g_globalvars.mapname )) ) { // ok map found in map list, select next map

		snprintf( mapid, sizeof(mapid), "k_ml_%d", i >= 1000 ? 0 : i );
		trap_cvar_string( mapid, newmap, sizeof(newmap) );

	}
	else { // map not found in map cycle
		if ( (i = cvar( "_k_last_cycle_map" )) ) { // try to return to map list at proper point
    
			snprintf( mapid, sizeof(mapid), "k_ml_%d", i );
			trap_cvar_string( mapid, newmap, sizeof(newmap) );
    
			if( !IsMapInCycle( newmap ) )
				newmap[0] = 0; // seems map not in current cycle
		}
	}

	if ( strnull( newmap ) ) // last resort, trying get first entry in map list
		trap_cvar_string( "k_ml_0", newmap, sizeof(newmap) );

	strlcpy(buf, newmap, buf_size);

	return buf;
}
