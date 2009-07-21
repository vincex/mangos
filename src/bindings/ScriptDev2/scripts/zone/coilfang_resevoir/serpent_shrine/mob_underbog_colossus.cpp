/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: mob_underbog_colossus
SD%Complete: 99%
SDComment: Underbog Colossus in SSC
SDCategory: SSC trash
EndScriptData */

#include "precompiled.h"
#include "sc_creature.h"

enum
{
    //aoe
    SPELL_ACID_GEYSER                 = 38971,
    SPELL_ENRAGE                      = 39031,
    SPELL_INFECTION	                  = 43586,//39032,
    
    //direct
    SPELL_PARASITE                    = 39044,
    SPELL_SPORE_QUAKE                 = 38976,
    SPELL_ATROPHIC_BLOW	              = 39015,
    
    // On Die
    NPC_LURKER                        =	22347,
    NPC_RAGER                         = 22352,  
    SPELL_REFRESHING_MIST             = 38730,
    SPELL_TOXIC_POOL                  = 38718,
    
    //amplify damage
    SPELL_AMPLIFY_NATURE              = 38086,
    SPELL_AMPLIFY_ARCANE              = 38087,
    SPELL_AMPLIFY_FIRE                = 38088,
    SPELL_AMPLIFY_FROST               = 38089,
    SPELL_AMPLIFY_HOLY                = 38090,
    SPELL_AMPLIFY_SHADOW              = 38092
};


struct MANGOS_DLL_DECL mob_underbog_colossusAI : public ScriptedAI
{
    mob_underbog_colossusAI(Creature *pCreature) : ScriptedAI(pCreature) 
    {
        Reset();
        SetupSpells();
    }
    
    uint32 Spell_1_Timer;
	uint32 Spell_2_Timer;
	uint32 Spell_1;
	uint32 Spell_2;

    void Reset()
    {
		Spell_1_Timer = 15000+rand()%7000;
		Spell_2_Timer = 15000+rand()%7000;
        AmplifyDamage();
    }

    void SetupSpells()
	{
		switch (rand()%3)
		{
			case 0: 
    				Spell_1 = SPELL_SPORE_QUAKE;
					Spell_2 = SPELL_INFECTION;
                    break;
			case 1: 
					Spell_1 = SPELL_ENRAGE;
					Spell_2 = SPELL_ATROPHIC_BLOW;
                    break;
			case 2: 
					Spell_1 = SPELL_ACID_GEYSER;
					Spell_2 = SPELL_PARASITE;
		}
	}

    void AmplifyDamage()
    {
        switch (rand()%6)
        {
            case 0: 
    			DoCast(m_creature, SPELL_AMPLIFY_NATURE);
                break;
            case 1:
                DoCast(m_creature, SPELL_AMPLIFY_ARCANE);
                break;
            case 2:
                DoCast(m_creature, SPELL_AMPLIFY_FIRE);
                break;			
            case 3:
                DoCast(m_creature, SPELL_AMPLIFY_FROST);
                break;
            case 4:
                DoCast(m_creature, SPELL_AMPLIFY_HOLY);
                break;
            case 5:
                DoCast(m_creature, SPELL_AMPLIFY_SHADOW);
        }
    }
    
	void JustDied(Unit *victim)
    {	
		switch (rand()%5)
		{
			case 0: // Nothing
                break;
			case 1: // Toxic Pool
                DoCast(m_creature, SPELL_TOXIC_POOL, true);
                break;
            case 2: // 2 Colossus Lurker
    			for (uint8 i=0;i<2;i++)
				    DoSpawnCreature(NPC_LURKER, 5, 5, 0, 0,TEMPSUMMON_TIMED_DESPAWN, 30000);
                break;
            case 3: // 5+ Colossus Rager  
                for (uint8 i=0;i<5+rand()%8;i++)
                    DoSpawnCreature(NPC_RAGER, 5, 5, 0, 0,TEMPSUMMON_TIMED_DESPAWN, 30000);
                break;
            case 4:
				for (uint8 i=0;i<2+rand()%3;i++)
					//DoSpawnCreature(MOB_REFRESHING_MIST, 0,0,0,0,TEMPSUMMON_CORPSE_DESPAWN, 26000);
                    if(Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM,0))
                       DoCast(pTarget, SPELL_REFRESHING_MIST, true);
                break;
		}
	}
    
    void JustSummoned(Creature* pSummoned)
    {
        if(Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM,0))
            pSummoned->AI()->AttackStart(pTarget);
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;        

        //Spell Timer 1
        if (Spell_1_Timer < diff)
        {
			DoCast(m_creature, Spell_1, true);//Spell 1 is always self cast (aoe)
            Spell_1_Timer = 20000+rand()%10000;;
        }else Spell_1_Timer -= diff;

		//Spell Timer 2
        if (Spell_2_Timer < diff)
        {
			if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM,0))
				DoCast(target, Spell_2, true);//Spell 2 is random targeted
            Spell_2_Timer = 20000+rand()%10000;;
        }else Spell_2_Timer -= diff;       

        DoMeleeAttackIfReady();
    } 
};

CreatureAI* GetAI_mob_underbog_colossus(Creature *_Creature)
{
    return new mob_underbog_colossusAI (_Creature);
}

void AddSC_mob_underbog_colossus()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "mob_underbog_colossus";
    newscript->GetAI = &GetAI_mob_underbog_colossus;
    newscript->RegisterSelf();
}
