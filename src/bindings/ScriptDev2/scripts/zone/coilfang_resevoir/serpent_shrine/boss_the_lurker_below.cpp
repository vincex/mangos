/* Copyright (C) 2006 - 2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_the_lurker_below
SD%Complete: BOH!
SDComment: ...
SDCategory: Coilfang Revoirsor, Serpentshrine Caverns
EndScriptData */

#include "precompiled.h"
#include "def_serpent_shrine.h"

enum
{
    SAY_SPOUT                       = -1549000,

    SPELL_SPOUT                     = 37433,
    SPELL_SPOUT_ANIM                = 42835,
    SPELL_SPOUT_BREATH              = 37431,
    SPELL_GEYSER                    = 37478,
    SPELL_WHIRL                     = 37660,
    SPELL_WATERBOLT                 = 37138,
    SPELL_SUBMERGE                  = 37550,
	SPELL_EMERGE                    = 20568,	//Ragnaros' Emerge :P 
    
    SPOUT_DIST                      = 100,

	MOBID_COILFANG_GUARDIAN         = 21873,
	MOBID_COILFANG_AMBUSHER         = 21865,

	//Ambusher spells
	SPELL_MULTISHOT	                = 37790,
	SPELL_SHOOT                     = 37770,
	//Guardian spells
	SPELL_ARCINGSMASH               = 38761, // Wrong SpellId. Can't find the right one.
	SPELL_HAMSTRING	                = 26211,
};

 #define PI 3.1415
 #define FISHING             33095

enum RotationType
{
    NOROTATE = 0,
    CLOCKWISE = 1,
    COUNTERCLOCKWISE = 2,
};

float AddPos[9][3] = 
{
    {2.855381, -459.823914, -19.182686},	//MOVE_AMBUSHER_1 X, Y, Z
    {12.4, -466.042267, -19.182686},		//MOVE_AMBUSHER_2 X, Y, Z
    {51.366653, -460.836060, -19.182686},	//MOVE_AMBUSHER_3 X, Y, Z
    {62.597980, -457.433044, -19.182686},	//MOVE_AMBUSHER_4 X, Y, Z
    {77.607452, -384.302765, -19.182686},	//MOVE_AMBUSHER_5 X, Y, Z
    {63.897900, -378.984924, -19.182686},	//MOVE_AMBUSHER_6 X, Y, Z
    {34.447250, -387.333618, -19.182686},	//MOVE_GUARDIAN_1 X, Y, Z
    {14.388216, -423.468018, -19.625271},	//MOVE_GUARDIAN_2 X, Y, Z
    {42.471519, -445.115295, -19.769423}	//MOVE_GUARDIAN_3 X, Y, Z
};

struct MANGOS_DLL_DECL boss_the_lurker_belowAI : public Scripted_NoMovementAI
{
    boss_the_lurker_belowAI(Creature *pCreature) : Scripted_NoMovementAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        SpellEntry *TempSpell = (SpellEntry*)GetSpellStore()->LookupEntry(SPELL_SPOUT_ANIM);
        if(TempSpell)
        {
            TempSpell->Effect[0] = 0;//remove all spell effect, only anim is needed
            TempSpell->Effect[1] = 0;
            TempSpell->Effect[2] = 0;
        }
        Reset();
    }

    ScriptedInstance* m_pInstance;
    
    uint8 RotType;
    
    uint32 SpoutTimer;
    uint32 SpoutAnimTimer;
    uint32 RotTimer;
    uint32 PhaseTimer;
    uint32 GeyserTimer;
    uint32 WaterboltTimer;
    uint32 WhirlTimer;
    uint32 WaterTimer;
    
    double SpoutAngle;
    bool Spawned;
    bool Submerged;
    Creature* Summoned;

    void Reset()
    {
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        m_creature->SetFlag(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
        //m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        m_creature->setFaction(35);
        
        WhirlTimer     = rand()%5000 + 15000;
        PhaseTimer     = 120000;
        SpoutTimer     = 45000;
        GeyserTimer    = rand()%5000 + 15000;
        WaterboltTimer = 3000;
        WaterTimer     = 1000;
        SpoutAnimTimer = 1000;
        RotTimer       = 20000;
        SpoutAngle     = 0;

        Submerged      = false;
        Spawned        = false;     

        RotType        = NOROTATE;

        if (m_pInstance)
            m_pInstance->SetData(TYPE_THELURKER_EVENT, NOT_STARTED);
    }

    void Aggro(Unit *who)
    {
         m_creature->SetInCombatWithZone();

        //DoScriptText(SAY_AGGRO, m_creature);
        if (m_pInstance)
            m_pInstance->SetData(TYPE_THELURKER_EVENT, IN_PROGRESS);
    }

    void JustDied(Unit* Killer)
    {
        //DoScriptText(SAY_DEATH, m_creature);
        if (m_pInstance)
            m_pInstance->SetData(TYPE_THELURKER_EVENT, DONE);
    }

    void CheckWaterAggro()
	{
	    if(m_pInstance->GetData(TYPE_THELURKER_EVENT) == NOT_STARTED) // se boss non killato proseguo
        {
            Map *pMap = m_creature->GetMap();
            if (!pMap->IsDungeon())
                return;
            Map::PlayerList const &PlayerList = pMap->GetPlayers();
            if (PlayerList.isEmpty())
	            return;
            for(Map::PlayerList::const_iterator i = PlayerList.begin();i != PlayerList.end(); ++i)
            {
                if(Player* pPlayer = i->getSource())
                {
                    if (!pPlayer->GetSession() || !pPlayer->isAlive())
                        continue;
                    if (pPlayer->GetDistance(m_creature) < 1.5 && pPlayer->IsInWater()) //swim in center for pop the boss
                    {
                        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        m_creature->RemoveFlag(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
                        //DoScriptText(SAY_EMERGE, m_creature);
                        DoCast(m_creature, SPELL_EMERGE);
                        m_creature->setFaction(14);
                    }
                }
            }
        }
    }

    void Rotate(const uint32 diff)  //20secs for 360turn
    {
		if(RotType==NOROTATE) 
		    return;

        m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0); //no target if rotating!
        if(RotType=CLOCKWISE)
        {
            SpoutAngle += (double)diff/20000*(double)M_PI*2;
            if (SpoutAngle >= M_PI*2) SpoutAngle = 0;
        }
        else // COUNTERCLOCKWISE
        {
            SpoutAngle -= (double)diff/20000*(double)M_PI*2;
            if (SpoutAngle <= 0)SpoutAngle = M_PI*2;    
        }
        m_creature->SetOrientation(SpoutAngle);
        m_creature->StopMoving();
        
        if(RotTimer<diff)//end rotate
        {
            RotType = NOROTATE;//set norotate state
            RotTimer=20000;
            m_creature->InterruptNonMeleeSpells(false);
            WhirlTimer = 2000; //whirl directly after spout ends
            return;
        }else RotTimer-=diff;

        if(SpoutAnimTimer<diff)
        {
            DoCast(m_creature,SPELL_SPOUT_ANIM,true);
            SpoutAnimTimer = 1000;
        }else SpoutAnimTimer-=diff;
        
        Map *pMap = m_creature->GetMap();
        if (pMap->IsDungeon() && m_pInstance->GetData(TYPE_THELURKER_EVENT) == IN_PROGRESS)
        {
            Map::PlayerList const &PlayerList = pMap->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            {
                Player *pPlayer = i->getSource();
                if(pPlayer && pPlayer->isAlive() && m_creature->HasInArc((double)diff/20000*(double)M_PI*2,pPlayer) && m_creature->GetDistance(pPlayer) <= SPOUT_DIST && !pPlayer->IsInWater())
                    DoCast(pPlayer,SPELL_SPOUT,true);//only knock back palyers in arc, in 100yards, not in water
             }
        }
    }    
    
    void StartRotate(Unit* pVictim)
    {
        switch (rand()%2)
        {
            case 0: RotType = CLOCKWISE; break;
            case 1: RotType = COUNTERCLOCKWISE; break;
        }
        RotTimer=20000;
        if(pVictim)
            SpoutAngle = m_creature->GetAngle(pVictim);
        //DoCast(m_creature,SPELL_SPOUT_BREATH);//take breath anim
        DoScriptText(SAY_SPOUT, m_creature);
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target or check aggro in water
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim() ) //verificare la seconda condizione per il rotate
        {
            if (WaterTimer < diff)  // "aggro boss"
            {
			    CheckWaterAggro();
                WaterTimer = 1000;
            } else WaterTimer -= diff;
			return;
        }
        
		Rotate(diff);//always check rotate
        if(!Submerged)
        {
            if(PhaseTimer < diff)
            {
                m_creature->InterruptNonMeleeSpells(false);
                //DoScriptText(SAY_SUBMERGE, m_creature);
                DoCast(m_creature,SPELL_SUBMERGE);
                PhaseTimer = 60000;
                Submerged = true;
            }else PhaseTimer -= diff;

            if(RotType == NOROTATE)
            {
                if(SpoutTimer < diff)
                {
                    if(m_creature->getVictim())
                        StartRotate(m_creature->getVictim());//start spout and random rotate
                    SpoutTimer= 45000;
                    return;
                }else SpoutTimer -= diff;
                 
                //WhirlTimer directly after a Spout and at random times
                if(WhirlTimer < diff)
                {
                    DoCast(m_creature,SPELL_WHIRL);
                    WhirlTimer = rand()%5000 + 15000;
                    WaterboltTimer += 5000;//add 5secs to waterbolt timer, to add some time to run back to boss
                }else WhirlTimer -= diff;

                if(GeyserTimer < diff)
                {
                    GeyserTimer = rand()%15000 + 10000;
                    if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 1))
                        DoCast(pTarget,SPELL_GEYSER);
                }else GeyserTimer -= diff;

                if(WaterboltTimer < diff)
                {
                    int i = 0;
                    bool meleeTarget = false;
                    Unit* pTarget = SelectUnit(SELECT_TARGET_TOPAGGRO,0);
                    if (!pTarget)
                        pTarget = m_creature->getVictim();
                    while (pTarget)
                    {
                        if( m_creature->IsWithinDistInMap(pTarget, 8))
                        {
                            meleeTarget = true;
                            break;
                        }
                        pTarget = SelectUnit(SELECT_TARGET_TOPAGGRO,i);
                        i++;
                    }

                    if(!meleeTarget)
                    {
                        DoCast(m_creature->getVictim(),SPELL_WATERBOLT);
                        WaterboltTimer = 3000;
                    } //else DoMeleeAttackIfReady();
                }else WaterboltTimer -= diff;
                
                DoMeleeAttackIfReady();
            } 
        } 
		else  //Submerged for 60 sec
		{
            if(PhaseTimer < diff)
            {
                Submerged = false;
                m_creature->RemoveAllAuras();
                m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                m_creature->RemoveFlag(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
                m_creature->RemoveFlag(UNIT_FIELD_BYTES_1,9);
                DoCast(m_creature, SPELL_EMERGE);
                Spawned = false;
                SpoutTimer = 5000;
                PhaseTimer = 120000;
                return;
            } else PhaseTimer -= diff;
            
            if(!Spawned)
            {
                m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                SummonAdd(MOBID_COILFANG_AMBUSHER,AddPos[0][0],AddPos[0][1],AddPos[0][2]);
                SummonAdd(MOBID_COILFANG_AMBUSHER,AddPos[1][0],AddPos[1][1],AddPos[1][2]);
                SummonAdd(MOBID_COILFANG_AMBUSHER,AddPos[2][0],AddPos[2][1],AddPos[2][2]);
                SummonAdd(MOBID_COILFANG_AMBUSHER,AddPos[3][0],AddPos[3][1],AddPos[3][2]);
                SummonAdd(MOBID_COILFANG_AMBUSHER,AddPos[4][0],AddPos[4][1],AddPos[4][2]);
                SummonAdd(MOBID_COILFANG_AMBUSHER,AddPos[5][0],AddPos[5][1],AddPos[5][2]);
                SummonAdd(MOBID_COILFANG_GUARDIAN,AddPos[6][0],AddPos[6][1],AddPos[6][2]);
                SummonAdd(MOBID_COILFANG_GUARDIAN,AddPos[7][0],AddPos[7][1],AddPos[7][2]);
                SummonAdd(MOBID_COILFANG_GUARDIAN,AddPos[8][0],AddPos[8][1],AddPos[8][2]);
                Spawned = true;
            }
        }
	}

	void SummonAdd(uint32 entry, float x, float y, float z)
	{
		Summoned = m_creature->SummonCreature(entry, x, y, z, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 0);
		
		if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
			Summoned->AI()->AttackStart(pTarget);
	}
};

CreatureAI* GetAI_boss_the_lurker_below(Creature *_Creature)
{
    return new boss_the_lurker_belowAI (_Creature);
}

void AddSC_boss_the_lurker_below()
{
	Script *newscript;
	
    newscript = new Script;
    newscript->Name = "boss_the_lurker_below";
    newscript->GetAI = &GetAI_boss_the_lurker_below;
    newscript->RegisterSelf();
}

struct MANGOS_DLL_DECL mob_coilfang_ambusherAI : public Scripted_NoMovementAI
{
    mob_coilfang_ambusherAI(Creature *pCreature) : Scripted_NoMovementAI(pCreature) {Reset();}

	uint32 MultiShotTimer;
	uint32 ShootBowTimer;
    
    void Reset()
    {
		MultiShotTimer = 10000;
		ShootBowTimer = 4000;
	}

    void Aggro(Unit *who) 
    {
    }

	void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim() )
            return;

		if(MultiShotTimer < diff)
		{
            if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                DoCast(pTarget ,SPELL_MULTISHOT);
			MultiShotTimer = 14000;
		}else MultiShotTimer -= diff;

		if(ShootBowTimer < diff)
		{
            DoCast(m_creature->getVictim(),SPELL_SHOOT);
			ShootBowTimer = 4000;
		}else ShootBowTimer -= diff;

        DoMeleeAttackIfReady();
	}
};

CreatureAI* GetAI_mob_coilfang_ambusher(Creature *_Creature)
{
    return new mob_coilfang_ambusherAI (_Creature);
}

void AddSC_mob_coilfang_ambusher()
{
	Script *newscript;
	
	newscript = new Script;
    newscript->Name = "mob_coilfang_ambusher";
    newscript->GetAI = &GetAI_mob_coilfang_ambusher;
    newscript->RegisterSelf();
}
