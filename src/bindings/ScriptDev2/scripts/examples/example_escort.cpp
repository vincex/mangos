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
SDName: Example_Escort
SD%Complete: 100
SDComment: Script used for testing escortAI
SDCategory: Script Examples
EndScriptData */

#include "precompiled.h"
#include "../npc/npc_escortAI.h"

#define SAY_AGGRO1      -1999910
#define SAY_AGGRO2      -1999911
#define SAY_WP_1        -1999912
#define SAY_WP_2        -1999913
#define SAY_WP_3        -1999914
#define SAY_WP_4        -1999915
#define SAY_DEATH_1     -1999916
#define SAY_DEATH_2     -1999917
#define SAY_DEATH_3     -1999918
#define SAY_SPELL       -1999919
#define SAY_RAND_1      -1999920
#define SAY_RAND_2      -1999921

#define GOSSIP_ITEM_1   "Click to Test Escort(Attack, Run)"
#define GOSSIP_ITEM_2   "Click to Test Escort(NoAttack, Walk)"
#define GOSSIP_ITEM_3   "Click to Test Escort(NoAttack, Run)"

struct MANGOS_DLL_DECL example_escortAI : public npc_escortAI
{
    public:

        // CreatureAI functions
        example_escortAI(Creature* pCreature) : npc_escortAI(pCreature) {Reset();}

        uint32 DeathCoilTimer;
        uint32 ChatTimer;

        // Pure Virtual Functions
        void WaypointReached(uint32 i)
        {
            switch (i)
            {
                case 1:
                    DoScriptText(SAY_WP_1, m_creature);
                    break;

                case 3:
                {
                    DoScriptText(SAY_WP_2, m_creature);
                    Creature* temp = m_creature->SummonCreature(21878, m_creature->GetPositionX()+5, m_creature->GetPositionY()+7, m_creature->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 3000);
                    if (temp)
                        temp->AI()->AttackStart(m_creature);
                }
                break;

                case 4:
                {
                    Unit* temp = Unit::GetUnit(*m_creature, PlayerGUID);
                    if (temp)
                    {
                        //temp is the target of the text
                        DoScriptText(SAY_WP_3, m_creature, temp);
                        //temp is the source of the text
                        DoScriptText(SAY_WP_4, temp);
                    }
                }
                break;
            }
        }

        void Aggro(Unit* who)
        {
            if (IsBeingEscorted)
            {
                if (Unit* temp = Unit::GetUnit(*m_creature, PlayerGUID))
                    DoScriptText(SAY_AGGRO1, m_creature, temp);
            }
            else DoScriptText(SAY_AGGRO2, m_creature);
        }

        void Reset()
        {
            DeathCoilTimer = 4000;
            ChatTimer = 4000;
        }

        void JustDied(Unit* killer)
        {
            if (IsBeingEscorted)
            {
                Unit *pTemp = Unit::GetUnit(*m_creature,PlayerGUID);
                //killer = m_creature when player got to far from creature
                if (killer == m_creature)
                {
                    //This is actually a whisper. You control the text type in database
                    if (pTemp)
                        DoScriptText(SAY_DEATH_1, m_creature, pTemp);
                }
                else if (pTemp) DoScriptText(SAY_DEATH_2, m_creature, pTemp);
            }
            else DoScriptText(SAY_DEATH_3, m_creature);
        }

        void UpdateAI(const uint32 diff)
        {
            //Must update npc_escortAI
            npc_escortAI::UpdateAI(diff);

            //Combat check
            if (m_creature->isInCombat() && m_creature->getVictim())
            {
                if (DeathCoilTimer < diff)
                {
                    DoScriptText(SAY_SPELL, m_creature);
                    m_creature->CastSpell(m_creature->getVictim(), 33130, false);

                    DeathCoilTimer = 4000;
                }else DeathCoilTimer -= diff;
            }else
            {
                //Out of combat but being escorted
                if (IsBeingEscorted)
                    if (ChatTimer < diff)
                {
                    if (m_creature->HasAura(3593, 0))
                    {
                        DoScriptText(SAY_RAND_1, m_creature);
                        m_creature->CastSpell(m_creature, 11540, false);
                    }else
                    {
                        DoScriptText(SAY_RAND_2, m_creature);
                        m_creature->CastSpell(m_creature, 3593, false);
                    }

                    ChatTimer = 12000;
                }else ChatTimer -= diff;
            }
        }
};

CreatureAI* GetAI_example_escort(Creature* pCreature)
{
    example_escortAI* testAI = new example_escortAI(pCreature);

    testAI->AddWaypoint(0, 1231, -4419, 23);
    testAI->AddWaypoint(1, 1198, -4440, 23, 0);
    testAI->AddWaypoint(2, 1208, -4392, 23);
    testAI->AddWaypoint(3, 1231, -4419, 23, 5000);
    testAI->AddWaypoint(4, 1208, -4392, 23, 5000);

    return (CreatureAI*)testAI;
}

bool GossipHello_example_escort(Player* pPlayer, Creature* pCreature)
{
    pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetGUID());
    pCreature->prepareGossipMenu(pPlayer,0);

    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);

    pCreature->sendPreparedGossip(pPlayer);
    return true;
}

bool GossipSelect_example_escort(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
{
    example_escortAI* pEscortAI = dynamic_cast<example_escortAI*>(pCreature->AI());

    if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
    {
        pPlayer->CLOSE_GOSSIP_MENU();

        if (pEscortAI)
            pEscortAI->Start(true, true, pPlayer->GetGUID());

        return true;                                        // prevent mangos core handling
    }

    if (uiAction == GOSSIP_ACTION_INFO_DEF+2)
    {
        pPlayer->CLOSE_GOSSIP_MENU();

        if (pEscortAI)
            pEscortAI->Start(false, false, pPlayer->GetGUID());

        return true;                                        // prevent mangos core handling
    }

    if (uiAction == GOSSIP_ACTION_INFO_DEF+3)
    {
        pPlayer->CLOSE_GOSSIP_MENU();

        if (pEscortAI)
            pEscortAI->Start(false, true, pPlayer->GetGUID());

        return true;                                        // prevent mangos core handling
    }
    return false;
}

void AddSC_example_escort()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "example_escort";
    newscript->GetAI = &GetAI_example_escort;
    newscript->pGossipHello = &GossipHello_example_escort;
    newscript->pGossipSelect = &GossipSelect_example_escort;
    newscript->RegisterSelf();
}
