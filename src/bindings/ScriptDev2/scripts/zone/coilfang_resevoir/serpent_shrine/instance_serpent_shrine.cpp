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
SDName: Instance_Serpent_Shrine
SD%Complete: 90
SDComment:
SDCategory: Coilfang Resevoir, Serpent Shrine Cavern
EndScriptData */

#include "precompiled.h"
#include "def_serpent_shrine.h"

/* Serpentshrine cavern encounters:
0 - Hydross The Unstable event
1 - Leotheras The Blind Event
2 - The Lurker Below Event
3 - Fathom-Lord Karathress Event
4 - Morogrim Tidewalker Event
5 - Lady Vashj Event
6 - Lurker Event (platform)
*/

//Lurker Event defines
#define SCALDING_WATER				37284
#define MOB_COILFANG_FRENZY			21508
#define MOB_COILFANG_FRENZY_CORPSE	21689
const int MAX_HONOR_GUARD	= 6;

const int ENCOUNTERS    = 7;
const int MAX_GENERATOR = 4;

struct MANGOS_DLL_DECL instance_serpentshrine_cavern : public ScriptedInstance
{
    instance_serpentshrine_cavern(Map* pMap) : ScriptedInstance(pMap) { Initialize(); };

    uint64 m_uiLurker;
    uint64 m_uiSharkkis;
    uint64 m_uiTidalvess;
    uint64 m_uiCaribdis;
    uint64 m_uiLadyVashj;
    uint64 m_uiKarathress;
    uint64 m_uiKarathressEvent_Starter;
    uint64 m_uiLeotheras;
    uint64 m_uiLeotherasEvent_Starter;
    uint64 m_uiSpellBinder[3];

    uint32 m_uiShieldGenerator[MAX_GENERATOR];
    uint32 m_uiEncounter[ENCOUNTERS];

	std::set<uint64> m_uiHonorGuardGUID;
	uint32 WaterTimer;
	uint32 EventTimer;

    void Initialize()
    {
        memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        memset(&m_auiShieldGenerator, 0, sizeof(m_auiShieldGenerator));

        m_uiLurker = 0;
        m_uiSharkkis = 0;
        m_uiTidalvess = 0;
        m_uiCaribdis = 0;
        m_uiLadyVashj = 0;
        m_uiKarathress = 0;
        m_uiKarathressEvent_Starter = 0;
        m_uiLeotheras = 0;
        m_uiLeotherasEvent_Starter = 0;
        m_uiSpellBinder[0]=0;
        m_uiSpellBinder[1]=0;
        m_uiSpellBinder[2]=0;

        //Lurker Event
        WaterTimer = 1000;
        EventTimer = 5000;
        m_uiHonorGuardGUID.clear();
    }

    bool IsEncounterInProgress()
    {
        for(uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            if (m_auiEncounter[i] == IN_PROGRESS)
                return true;

        return false;
    }

    void notifyFish() 
    {
        Map::PlayerList const& PlayerList = instance->GetPlayers();
        if (PlayerList.isEmpty())
            return;
        for(Map::PlayerList::const_iterator i = PlayerList.begin();i != PlayerList.end(); ++i)
            if(Player* pPlayer = i->getSource())
            {
                pPlayer->GetSession()->SendNotification("The water started to boil...");
                pPlayer->GetSession()->SendNotification("...The Lurker Below is waiting...");
            }
        //summonare una ventina di pesci oppure scorrere i mob spawnati(e renderli friendly, intargettabili
        // oppure gestire una pool di pesci
    }

    //Lurker Event instance Update
    void Update(uint32 diff)
    {
        if(GetData(TYPE_THELURKER_EVENT) == DONE) // se lurker ucciso, non faccio nulla
            return;
        if (EventTimer < diff) //lurker platform check
        {
            if (!m_uiHonorGuardGUID.empty())  //nel caso di mob non spawnati
            {
                uint32 Dead=0;  //contatore honor guard morti
                for(std::set<uint64>::iterator i = m_uiHonorGuardGUID.begin(); i != m_uiHonorGuardGUID.end(); ++i)
                    if (Creature* HonorGuard = instance->GetCreature(*i))
                        if (HonorGuard->isDead())
                            Dead++;	

                if (Dead==MAX_HONOR_GUARD && GetData(TYPE_PLATFORM_EVENT)!=DONE)   //se gli honor guard sono tutti e 6 morti setto l'evento piattaforme fatto
                {
                        SetData(TYPE_PLATFORM_EVENT, DONE);
                        notifyFish(); //informo tutti i player che l'acqua bolle e rendo visibili i pesci morti( o li spawno?)
                }
                else if (Dead==0)  //se non ho killato i 6 mob(o sono respawnati tutti e 6) imposto a not_started l'evento piattaforme
                    SetData(TYPE_PLATFORM_EVENT, NOT_STARTED);
            }
            EventTimer = 5000;
        }else EventTimer -=diff;

        if (WaterTimer < diff) //water event
        {
            Map::PlayerList const& PlayerList = instance->GetPlayers();
            if (PlayerList.isEmpty())
   	            return;

            for(Map::PlayerList::const_iterator i = PlayerList.begin();i != PlayerList.end(); ++i)
            {
                if(Player* pPlayer = i->getSource())
                {
                    if (pPlayer->IsInWater())   //se il pg è in acqua
                    {
                        if(GetData(TYPE_PLATFORM_EVENT) != DONE) // se l'evento piattaforme non è stato fatto summono i piranha
                        {	
                            Creature* Fishy = pPlayer->SummonCreature(MOB_COILFANG_FRENZY,pPlayer->GetPositionX(),pPlayer->GetPositionY(),pPlayer->GetPositionZ(),pPlayer->GetOrientation(),TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,15000);
                            if (Fishy) 
                                Fishy->AI()->AttackStart(pPlayer);
                        }
                        else if (!pPlayer->HasAura(SCALDING_WATER,0))             //se il pg è in acqua ma l'evento piattaforme è fatto 
                                pPlayer->CastSpell(pPlayer,SCALDING_WATER,true);  // e il pg non ha l'aura dell'acqua che bolle gliela casto
                    }
                    else if (pPlayer->HasAura(SCALDING_WATER,0) && !pPlayer->HasMovementFlag(MOVEMENTFLAG_JUMPING)) // se il pg non è in acqua e ha l'aura la tolgo
                            pPlayer->RemoveAurasDueToSpell(SCALDING_WATER);												// ma solo se non sta saltando fuori dall'acqua		
                }
            }			
            WaterTimer = 1000;
        }else WaterTimer -= diff;
    }

    void OnCreatureCreate(Creature* pCreature)
    {
        switch(pCreature->GetEntry())
        {
            case 21212: m_uiLadyVashj  = pCreature->GetGUID(); break;
            case 21214: m_uiKarathress = pCreature->GetGUID(); break;
            case 21966: m_uiSharkkis   = pCreature->GetGUID(); break;
            case 21965: m_uiTidalvess  = pCreature->GetGUID(); break;
            case 21964: m_uiCaribdis   = pCreature->GetGUID(); break;
            case 21217: m_uiLurker     = pCreature->GetGUID(); break;
			case 21215: m_uiLeotheras  = pCreature->GetGUID(); break;
            case 21218: m_uiHonorGuardGUID.insert(pCreature->GetGUID()); break; // dovrebbero essere 6 come le piattaforme attorno a lurker
            case 21806: for(int i=0; i<3; i++)  // dovrebbero essere 3 vicino leotheras
                            if(m_uiSpellBinder[i]==0) 
                            {
                                m_uiSpellBinder[i]=pCreature->GetGUID();
                                break; 
                            }
                            break;
        }
    }

    void SetData64(uint32 uiType, uint64 uiData)
    {
        if (uiType == DATA_KARATHRESS_STARTER)
            m_uiKarathressEvent_Starter = uiData;
        if (uiType == DATA_LEOTHERAS)
            m_uiLeotheras = uiData;
        if (uiType == DATA_LEOTHERAS_EVENT_STARTER)
            m_uiLeotherasEvent_Starter = uiData;
    }

    uint64 GetData64(uint32 uiIdentifier)
    {
        switch(uiIdentifier)
        {
            case DATA_SHARKKIS:
                return m_uiSharkkis;
            case DATA_TIDALVESS:
                return m_uiTidalvess;
            case DATA_CARIBDIS:
                return m_uiCaribdis;
            case DATA_LADYVASHJ:
                return m_uiLadyVashj;
            case DATA_LEOTHERAS:
                return m_uiLeotheras;
            case DATA_LEOTHERAS_EVENT_STARTER:
                return m_uiLeotherasEvent_Starter;
            case DATA_KARATHRESS:
                return m_uiKarathress;
            case DATA_KARATHRESS_STARTER:
                return m_uiKarathressEvent_Starter;
            case DATA_SPELLBINDER_1:
                return m_uiSpellBinder[0];
            case DATA_SPELLBINDER_2:
                return m_uiSpellBinder[1];
            case DATA_SPELLBINDER_3:
                return m_uiSpellBinder[2];
        }
        return 0;
    }

    void SetData(uint32 uiType, uint32 uiData)
    {
        switch(uiType)
        {
            case TYPE_HYDROSS_EVENT:
                m_auiEncounter[0] = uiData;
                break;
            case TYPE_LEOTHERAS_EVENT:
                m_auiEncounter[1] = uiData;
                break;
            case TYPE_THELURKER_EVENT:
                if (m_uiEncounter[2] == DONE) break;
                m_auiEncounter[2] = uiData;
                break;
            case TYPE_KARATHRESS_EVENT:
                m_auiEncounter[3] = uiData;
                break;
            case TYPE_MOROGRIM_EVENT:
                m_auiEncounter[4] = uiData;
                break;
            case TYPE_LADYVASHJ_EVENT:
                if (uiData == NOT_STARTED)
                    memset(&m_auiShieldGenerator, 0, sizeof(m_auiShieldGenerator));
                m_auiEncounter[5] = uiData;
                break;
            case TYPE_PLATFORM_EVENT:
                m_auiEncounter[6] = uiData;
                break;
            case TYPE_SHIELDGENERATOR1:
                m_auiShieldGenerator[0] = uiData;
                break;
            case TYPE_SHIELDGENERATOR2:
                m_auiShieldGenerator[1] = uiData;
                break;
            case TYPE_SHIELDGENERATOR3:
                m_auiShieldGenerator[2] = uiData;
                break;
            case TYPE_SHIELDGENERATOR4:
                m_auiShieldGenerator[3] = uiData;
                break;
        }
    }

    uint32 GetData(uint32 uiType)
    {
        switch(uiType)
        {
            case TYPE_HYDROSS_EVENT:
                return m_uiEncounter[0];

            case TYPE_LEOTHERAS_EVENT:
                return m_auiEncounter[1];

            case TYPE_THELURKER_EVENT:
                return m_auiEncounter[2];

            case TYPE_KARATHRESS_EVENT:
                return m_auiEncounter[3];

            case TYPE_MOROGRIM_EVENT:
                return m_auiEncounter[4];

            case TYPE_LADYVASHJ_EVENT:
                return m_auiEncounter[5];

            case TYPE_PLATFORM_EVENT:
                return m_auiEncounter[6];

            case TYPE_SHIELDGENERATOR1:
                return m_auiShieldGenerator[0];

            case TYPE_SHIELDGENERATOR2:
                return m_auiShieldGenerator[1];

            case TYPE_SHIELDGENERATOR3:
                return m_auiShieldGenerator[2];

            case TYPE_SHIELDGENERATOR4:
                return m_auiShieldGenerator[3];

            case TYPE_VASHJ_PHASE3_CHECK:
                for(uint8 i = 0; i < MAX_GENERATOR; ++i)
                {
                    if (m_auiShieldGenerator[i] != DONE)
                        return NOT_STARTED;
                }
                return DONE;
        }

        return 0;
    }
};

InstanceData* GetInstanceData_instance_serpentshrine_cavern(Map* pMap)
{
    return new instance_serpentshrine_cavern(pMap);
}

void AddSC_instance_serpentshrine_cavern()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "instance_serpent_shrine";
    newscript->GetInstanceData = &GetInstanceData_instance_serpentshrine_cavern;
    newscript->RegisterSelf();
}
