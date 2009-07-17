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
SDName: Boss_Najentus_GDR
SD%Complete: 0
SDComment: Boss GDR apertura instance T5
SDCategory: Custom, GDR
EndScriptData */

#include "precompiled.h"

enum
{
	SAY_AGGRO                       = -2000000,  // Now, what's this? surely an EPIC FAIL!
    SAY_PHASE2                      = -2000001,  // This isn't enough for me! come on, insects!
    SAY_PHASE3                      = -2000002,  // The play is now to an end. Let's finish this pathetic show...
    SAY_ENRAGE                      = -2000003,  // For my masters! Is time to DIE! 
    SAY_DEATH                       = -2000004,  // Rosik. (yellow)
    SAY_SLAY1                       = -2000005,  // Ahahahah you deserve it, you little scum!"
	SAY_SLAY2                       = -2000006,  // Ahhh, your death is pure joy to me...
	SAY_SUMMONS1                    = -2000007,  // Rise now, the master needs you! Rise from the surface of these watery deeps, and kill!
	SAY_SUMMONS2                    = -2000008,  // Taste the power of water! Drown them all!!!
	SAY_INTRO_1                     = -2000009,  // L'ora è finalmente giunta! 
	SAY_INTRO_2                     = -2000010,  // Riacquisteremo il posto che ci spetta di diritto da secoli! 
	SAY_INTRO_3                     = -2000011,  // Qualunque essere sentirà questo evento sulla propria pelle! 
	SAY_INTRO_4                     = -2000012,  // Ovunque, finalmente, ogni creatura dovrà inchinarsi alla superiorità e alla forza della nostra razza, ognuno si piegherà all'immenso potere di Lord Illidan e della venerabile Lady Vashj! 
	SAY_INTRO_5                     = -2000013,  // Lunga vita ai Padroni!
	
	// Dopo discorso(cambio model) e poco prima enrage
	SPELL_EARTH_QUAKE               = 42499,
	SPELL_EARTH_QUAKE_VISUAL        = 36455,
	SPELL_CLOUD						= 42500,
	
	// Sempre
    SPELL_REFLECT                   = 31534, // 25-30 secondi
    SPELL_POISON_AOE                = 38655, // 35-40 secondi
	
	// Prima fase(@100 - 75%) - Seconda Fase
    SPELL_WATERBOLT                 = 31012, // 15-20 secondi
	
	//Seconda (@75% - 50%) - Terza Fase
    NPC_SUMMONS                     = 17917, // 5 summon ogni 120 secondi
	MAX_NPC_SUMMON                  = 8,
	
	//Terza fase (@50% - 25%)
	SPELL_CHARGE                    = 39574, // Random Target - 30-35 secondi  
    SPELL_WHIRLWIND                 = 37641, // 20-25 secondi
	
	//Quarta fase(@25% - 0%)
    SPELL_ENRAGE                    = 41447, 
	SPELL_CHAIN_LIGHT               = 44318, // Random Target - 20-30

	//Modello
	MODEL_NAGA                      = 21174,
    MODEL_ELF                       = 20033
	
};

struct MANGOS_DLL_DECL boss_najentus_gdr_AI : public ScriptedAI
{
    boss_najentus_gdr_AI(Creature* pCreature) : ScriptedAI(pCreature)   {Reset(); }

    uint32 reflect_Timer;
    uint32 poison_aoe_Timer;
    uint32 summons_Timer;
	uint32 waterbolt_Timer;
    uint32 charge_Timer;
	uint32 whirl_Timer;
	uint32 chain_Timer;
    uint32 eventProgress_Timer;
	
	uint8 phase;
	uint8 intro;
	bool Init;

    void Reset()
    {
        reflect_Timer = 25000 + rand()%5000;
		poison_aoe_Timer = 40000 - rand()%5000;
		waterbolt_Timer = 15000 + rand()%5000;
        summons_Timer = 5000; 
        charge_Timer = 25000;
		whirl_Timer = 10000;
		chain_Timer = 5000;
		eventProgress_Timer = 8000;	
		
		phase = 1;
		
		m_creature->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_FROST, true);
		
		if(Init==true)
				m_creature->SetDisplayId(MODEL_NAGA);
		else
		{
			Init=false;
			intro=0;
			m_creature->SetDisplayId(MODEL_ELF);
			m_creature->setFaction(35);
			m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        	m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);		
		}			
		
		/*
		    "scarti di superficie a inquinare le nostre acque? cosa credete di fare? di fermarci? poveri idioti..." (engage combat) [english: "patethic fool. the tide will drown you all..."]
			dopo i primi 5 secondi dalla prima frase dice la seguente:
			"tutto qui quello che sapete fare? ahahah" [english: "is that all? how useless..."]
			durante tentativo orda SOLO UNa frase:
			"e voi? che divertenti..." [english: "That's amusing, what are you up to? ahahahaha"]*/
		
    }

    void KilledUnit(Unit *victim)
    {
        switch(rand()%2)
        {
            case 0: DoScriptText(SAY_SLAY1, m_creature); break;
            case 1: DoScriptText(SAY_SLAY2, m_creature); break;
        }
    }

	void JustSummoned(Creature* pSummoned)
    {
        uint32 uiEntry = pSummoned->GetEntry();
        if (uiEntry == NPC_SUMMONS)
        {
            if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 1))
                pSummoned->AI()->AttackStart(pTarget);
        }
	}
	
	void SpawnAdds()
    {
        for(uint8 i = 0; i < MAX_NPC_SUMMON; ++i)
		 if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 1))  //escluso target attuale
            m_creature->SummonCreature(NPC_SUMMONS,pTarget->GetPositionX(),pTarget->GetPositionY(),pTarget->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);
    }	
	
    void JustDied(Unit *victim)
    {
        DoScriptText(SAY_DEATH, m_creature);
    }

    void Aggro(Unit *who)
    {
        DoScriptText(SAY_AGGRO, m_creature);
		AttackStart(who);
		DoCast(who, SPELL_CHARGE);		
    }

    void UpdateAI(const uint32 diff)
    {
		if (!Init)
        {
            if (eventProgress_Timer < diff)
            {
                if (intro < 7)
                {
                    switch(intro)
                    {
                        case 1:
                            DoScriptText(SAY_INTRO_1, m_creature);  // L'ora è finalmente giunta! 
                            eventProgress_Timer = 5000;
                            break;
                        case 2:
                            DoScriptText(SAY_INTRO_2, m_creature); // Riacquisteremo il posto che ci spetta di diritto da secoli! 
                            eventProgress_Timer = 6000;
                            break;
                        case 3:
                            DoScriptText(SAY_INTRO_3, m_creature); // Qualunque essere sentirà questo evento sulla propria pelle! 
                            eventProgress_Timer = 7000;
                            break;
                        case 4:
                            DoScriptText(SAY_INTRO_4, m_creature); // Ovunque, finalmente, ogni creatura dovrà inchinarsi alla superiorità e alla forza della nostra razza, ognuno si piegherà all'immenso potere di Lord Illidan e della venerabile Lady Vashj! 
                            eventProgress_Timer = 12000;
                            break;
                        case 5:
						    DoScriptText(SAY_INTRO_5, m_creature); // Lunga vita ai Padroni!
						    eventProgress_Timer = 2000;
							break;
						case 6:  							
							DoCast(m_creature, SPELL_EARTH_QUAKE,true);			//Terremoto (rocce..)		
							DoCast(m_creature, SPELL_EARTH_QUAKE_VISUAL,true);  //Terremoto (shake screen)
							m_creature->SetDisplayId(MODEL_NAGA);
							m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        					m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
							m_creature->setFaction(14);
							DoCast(m_creature, SPELL_POISON_AOE);
							/*if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))  //escluso target attuale
                                DoCast(pTarget, SPELL_CHARGE);*/
								
							/*Unit* pTarget = m_creature->getVictim();
                            DoResetThreat();
                            AttackStart(pTarget);
                			m_creature->GetMotionMaster()->Clear();
                			m_creature->GetMotionMaster()->MoveChase(pTarget);
                			m_creature->AddThreat(pTarget, 0.0f);	*/
                            Init = true;
							break;
                    }
                    ++intro;
                }
            } else eventProgress_Timer -= diff;
        }
		
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;
			
        if (reflect_Timer < diff)  // Reflect Shield 25-30 secondi
        {
            DoCast(m_creature, SPELL_REFLECT);
            reflect_Timer = 25000 + rand()%5000;
        }else reflect_Timer -= diff;
		
		if (poison_aoe_Timer < diff)  // Poison Bolt Volley 35-40 secondi
        {
			DoCast(m_creature, SPELL_POISON_AOE);			
            poison_aoe_Timer = 40000 - rand()%5000;
        }else poison_aoe_Timer -= diff;
		
		if (phase == 1 || phase == 2)
		{	
            if (phase == 1) // Check Fase 2 = 75%
            {
                if ((m_creature->GetHealth()*100 / m_creature->GetMaxHealth()) <= 75) 
			    {
			        DoScriptText(SAY_PHASE2, m_creature);
			        phase = 2;
			        return;
			    }	
            }		
            if (waterbolt_Timer < diff)  // WaterBolt  15-20 secondi
            {
                if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))  //incluso target attuale
                    DoCast(pTarget, SPELL_WATERBOLT);
                waterbolt_Timer = 15000 + rand()%5000;
            }else waterbolt_Timer -= diff;		
        }
		
        if (phase == 2 || phase == 3)
        {
            if (phase == 2) // Check Fase 3 = 50%
            {
                if ((m_creature->GetHealth()*100 / m_creature->GetMaxHealth()) <= 50) 
			    {
			        DoScriptText(SAY_PHASE3, m_creature);
			        phase = 3;
                    return;
                }	
            }
		    if (summons_Timer < diff)  // Summons Water Elemental  2minuti e 30 secondi
            {
                switch(rand()%2)
                {
                    case 0: DoScriptText(SAY_SUMMONS1, m_creature); break;
                    case 1: DoScriptText(SAY_SUMMONS2, m_creature); break;
                }  
  				SpawnAdds(); // summona MAX_NPC_SUMMON water elemental su target random incluso target
				
                  summons_Timer = 120000 - rand()%25000 ;
            }else summons_Timer -= diff;		        
        }

        if (phase == 3)
        {
			if (((m_creature->GetHealth()*100)/ m_creature->GetMaxHealth()) <= 25)  // Check Fase 4 = 25% (enrage)
        	{
					DoScriptText(SAY_ENRAGE, m_creature);
					DoCast(m_creature, SPELL_EARTH_QUAKE,true);			//Terremoto (rocce..)		
					DoCast(m_creature, SPELL_EARTH_QUAKE_VISUAL,true);  //Terremoto (shake screen)
  				    DoCast(m_creature, SPELL_CLOUD,true);               //Tempesta
					DoCast(m_creature, SPELL_ENRAGE);
					phase = 4;
					return;
      		}			
			if (charge_Timer < diff)  // ChargeRandom  30-35 secondi
            {
                if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 1))  //escluso target attuale
                    DoCast(pTarget, SPELL_CHARGE);
				charge_Timer = 30000 + rand()%5000;
            }else charge_Timer -= diff;	            
        }
				
		if (phase == 4 || phase == 3)
		{

			if (whirl_Timer < diff)  // Whirlwind  20-25 secondi
            {
				DoCast(m_creature, SPELL_WHIRLWIND);
				whirl_Timer = 20000 + rand()%5000;
            }else whirl_Timer -= diff;			
			
			if (chain_Timer < diff)  // Chain Lightining  20-30 secondi
            {
				if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 1))  //escluso target attuale
					DoCast(pTarget, SPELL_CHAIN_LIGHT);
				chain_Timer = 20000 + rand()%10000;
            }else chain_Timer -= diff;				
		}
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_najentus_gdr(Creature* pCreature)
{
    return new boss_najentus_gdr_AI(pCreature);
}

void AddSC_boss_najentus_gdr()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_najentus_gdr";
    newscript->GetAI = &GetAI_boss_najentus_gdr;
    newscript->RegisterSelf();
}
