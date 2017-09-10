#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "ObjectDefines.h"
#include "GridDefines.h"
#include "GridNotifiers.h"
#include "SpellMgr.h"
#include "GridNotifiersImpl.h"
#include "Cell.h"
#include "CellImpl.h"
#include "Unit.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"

#include "SmartAI.h"

SmartAI::SmartAI(Creature* c) : CreatureAI(c)
{
    // copy script to local (protection for table reload)

    mIsCharmed = false;
    mWayPoints = nullptr;
    mEscortState = SMART_ESCORT_NONE;
    mCurrentWPID = 0; //first wp id is 1 !!
    mWPReached = false;
    mOOCReached = false;
    mWPPauseTimer = 0;
    mLastWP = nullptr;

    mCanRepeatPath = false;

    // spawn in run mode
    // me->SetWalk(false); //disabled this, movement generator may have altered this already
    mRun = false;
    mEvadeDisabled = false;

    mLastOOCPos = me->GetPosition();

    mCanAutoAttack = true;
    mCanCombatMove = true;

    mForcedPaused = false;
    mLastWPIDReached = 0;

    mEscortQuestID = 0;

    mDespawnTime = 0;
    mDespawnState = 0;

    mEscortInvokerCheckTimer = 1000;
    mFollowGuid = 0;
    mFollowDist = 0;
    mFollowAngle = 0;
    mFollowCredit = 0;
    mFollowArrivedEntry = 0;
    mFollowCreditType = 0;
    mFollowArrivedTimer = 0;
    mInvincibilityHpLevel = 0;

    mJustReset = false;
    _gossipReturn = false;
}

bool SmartAI::IsAIControlled() const
{
    if (me->IsControlledByPlayer())
            return false;
    if (mIsCharmed)
            return false;
    return true;
}

void SmartAI::UpdateDespawn(const uint32 diff)
{
    if (mDespawnState <= 1 || mDespawnState > 3)
        return;

    if (mDespawnTime < diff)
    {
        if (mDespawnState == 2)
        {
			me->SetVisible(false);
            mDespawnTime = 1000;
            mDespawnState++;
        }
        else
            me->ForcedDespawn();
    } else mDespawnTime -= diff;
}

std::shared_ptr<WayPoint> SmartAI::GetNextWayPoint()
{
    if (!mWayPoints || mWayPoints->empty())
        return nullptr;

    mCurrentWPID++;
    WPPath::const_iterator itr = mWayPoints->find(mCurrentWPID);
    if (itr != mWayPoints->end())
    {
        mLastWP = (*itr).second;
        if (mLastWP->id != mCurrentWPID)
        {
            TC_LOG_ERROR("FIXME","SmartAI::GetNextWayPoint: Got not expected waypoint id %u, expected %u", mLastWP->id, mCurrentWPID);
        }
        return (*itr).second;
    }
    return nullptr;
}

void SmartAI::StartPath(bool run, uint32 path, bool repeat, Unit* /*invoker*/)
{
    if (me->IsInCombat())// no wp movement in combat
    {
        TC_LOG_ERROR("FIXME","SmartAI::StartPath: Creature entry %u wanted to start waypoint movement while in combat, ignoring.", me->GetEntry());
        return;
    }
    if (HasEscortState(SMART_ESCORT_ESCORTING))
        StopPath();
    if (path)
        if (!LoadPath(path))
            return;
    if (!mWayPoints || mWayPoints->empty())
        return;

    AddEscortState(SMART_ESCORT_ESCORTING);
    me->SetEscorted(true);
    mCanRepeatPath = repeat;

    SetRun(run);

    if (auto wp = GetNextWayPoint())
    {
        mLastOOCPos = me->GetPosition();
        MovePointInPath(run, wp->id, wp->x, wp->y, wp->z);
        GetScript()->ProcessEventsFor(SMART_EVENT_WAYPOINT_START, nullptr, wp->id, GetScript()->GetPathId());
    }
}

bool SmartAI::LoadPath(uint32 entry)
{
    if (HasEscortState(SMART_ESCORT_ESCORTING))
        return false;
    mWayPoints = sSmartWaypointMgr->GetPath(entry);
    if (!mWayPoints)
    {
        GetScript()->SetPathId(0);
        return false;
    }
    GetScript()->SetPathId(entry);
    return true;
}

void SmartAI::PausePath(uint32 delay, bool forced)
{
    if (!HasEscortState(SMART_ESCORT_ESCORTING))
        return;
    if (HasEscortState(SMART_ESCORT_PAUSED))
    {
        TC_LOG_ERROR("misc", "SmartAI::PausePath: Creature entry %u wanted to pause waypoint movement while already paused, ignoring.", me->GetEntry());
        return;
    }
    mForcedPaused = forced;
    mLastOOCPos = me->GetPosition();
    AddEscortState(SMART_ESCORT_PAUSED);
    mWPPauseTimer = delay;
    if (forced)
    {
        SetRun(mRun);
        me->StopMoving();//force stop
        me->GetMotionMaster()->MoveIdle();//force stop
    }
    GetScript()->ProcessEventsFor(SMART_EVENT_WAYPOINT_PAUSED, nullptr, mLastWP->id, GetScript()->GetPathId());
}

void SmartAI::StopPath(uint32 DespawnTime, uint32 quest, bool fail)
{
    if (!HasEscortState(SMART_ESCORT_ESCORTING))
        return;

    if (quest)
        mEscortQuestID = quest;
    SetDespawnTime(DespawnTime);
    //mDespawnTime = DespawnTime;

    mLastOOCPos = me->GetPosition();
    me->StopMoving();//force stop
    me->GetMotionMaster()->MoveIdle();
    GetScript()->ProcessEventsFor(SMART_EVENT_WAYPOINT_STOPPED, nullptr, mLastWP->id, GetScript()->GetPathId());
    EndPath(fail);
}

void SmartAI::EndPath(bool fail, bool died)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_WAYPOINT_ENDED, nullptr, mLastWP->id, GetScript()->GetPathId());

    RemoveEscortState(SMART_ESCORT_ESCORTING | SMART_ESCORT_PAUSED | SMART_ESCORT_RETURNING);
    me->SetEscorted(false);
    mWayPoints = nullptr;
    mCurrentWPID = 0;
    mWPPauseTimer = 0;
    mLastWP = nullptr;

    if (mCanRepeatPath && !died)
    {
        if (IsAIControlled())
            StartPath(mRun, GetScript()->GetPathId(), true);
    } else
        GetScript()->SetPathId(0);
    
    ObjectVector const* targets = GetScript()->GetStoredTargetVector(SMART_ESCORT_TARGETS, *me);
    if (targets && mEscortQuestID)
    {
        if (targets->size() == 1 && GetScript()->IsPlayer((*targets->begin())))
        {
            Player* player = (*targets->begin())->ToPlayer();
            if (!fail && player->IsAtGroupRewardDistance(me) && !player->GetCorpse())
                player->GroupEventHappens(mEscortQuestID, me);
           
            if (fail && player->GetQuestStatus(mEscortQuestID) == QUEST_STATUS_INCOMPLETE)
                player->FailQuest(mEscortQuestID);

            if (Group* group = player->GetGroup())
            {
                for (GroupReference* groupRef = group->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
                {
                    Player* groupGuy = groupRef->GetSource();

                    if (!fail && groupGuy->IsAtGroupRewardDistance(me) && !groupGuy->GetCorpse())
                        groupGuy->AreaExploredOrEventHappens(mEscortQuestID);
                    if (fail && groupGuy->GetQuestStatus(mEscortQuestID) == QUEST_STATUS_INCOMPLETE)
                        groupGuy->FailQuest(mEscortQuestID);
                }
            }
        }else
        {
            for (auto & target : *targets)
            {
                if (GetScript()->IsPlayer(target))
                {
                    Player* player = target->ToPlayer();
                    if (!fail && player->IsAtGroupRewardDistance(me) && !player->GetCorpse())
                        player->AreaExploredOrEventHappens(mEscortQuestID);
                    if (fail && player->GetQuestStatus(mEscortQuestID) == QUEST_STATUS_INCOMPLETE)
                        player->FailQuest(mEscortQuestID);
                }
            }
        }
    }
    if (mDespawnState == 1)
        StartDespawn();
}

void SmartAI::ResumePath()
{
    SetRun(mRun);
    if (mLastWP)
        MovePointInPath(mRun, mLastWP->id, mLastWP->x, mLastWP->y, mLastWP->z);
}

void SmartAI::MovePointInPath(bool run, uint32 id, float x, float y, float z, float o)
{
    me->GetMotionMaster()->MovePoint(id, x, y, z);

    if (me->GetFormation() && me->GetFormation()->getLeader() == me)
        me->GetFormation()->LeaderMoveTo(x, y, z, run);
}

void SmartAI::ReturnToLastOOCPos()
{
    if (!IsAIControlled())
        return;

    SetRun(mRun);
    MovePointInPath(mRun, SMART_ESCORT_LAST_OOC_POINT, mLastOOCPos.GetPositionX(), mLastOOCPos.GetPositionY(), mLastOOCPos.GetPositionZ(), mLastOOCPos.GetOrientation() );
}

void SmartAI::UpdatePath(const uint32 diff)
{
    if (!HasEscortState(SMART_ESCORT_ESCORTING))
        return;
    if (mEscortInvokerCheckTimer < diff)
    {
        if (!IsEscortInvokerInRange())
        {
            StopPath(mDespawnTime, mEscortQuestID, true);
        }
        mEscortInvokerCheckTimer = 1000;
    }
    else mEscortInvokerCheckTimer -= diff;
    // handle pause
    if (HasEscortState(SMART_ESCORT_PAUSED))
    {
        if (mWPPauseTimer < diff)
        {
            if (!me->IsInCombat() && !HasEscortState(SMART_ESCORT_RETURNING) && (mWPReached || mLastWPIDReached == SMART_ESCORT_LAST_OOC_POINT || mForcedPaused))
            {
                GetScript()->ProcessEventsFor(SMART_EVENT_WAYPOINT_RESUMED, nullptr, mLastWP->id, GetScript()->GetPathId());
                RemoveEscortState(SMART_ESCORT_PAUSED);
                if (mForcedPaused)// if paused between 2 wps resend movement
                {
                    ResumePath();
                    mWPReached = false;
                    mForcedPaused = false;
                }
                if (mLastWPIDReached == SMART_ESCORT_LAST_OOC_POINT)
                    mWPReached = true;
            }
            mWPPauseTimer = 0;
        }
        else {
            mWPPauseTimer -= diff;
        }
    }
    if (HasEscortState(SMART_ESCORT_RETURNING))
    {
        if (mWPReached)//reached OOC WP
        {
            RemoveEscortState(SMART_ESCORT_RETURNING);
            if (!HasEscortState(SMART_ESCORT_PAUSED))
                ResumePath();
            mWPReached = false;
        }
    }
    if ((!me->HasReactState(REACT_PASSIVE) && me->IsInCombat()) || HasEscortState(SMART_ESCORT_PAUSED | SMART_ESCORT_RETURNING))
        return;
    // handle next wp
    if (mWPReached)//reached WP
    {
        mWPReached = false;
        if (mCurrentWPID == GetWPCount())
        {
            EndPath();
        }
        else if (auto wp = GetNextWayPoint())
        {
            SetRun(mRun);
            MovePointInPath(mRun, wp->id, wp->x, wp->y, wp->z);
        }
    }
}

void SmartAI::UpdateAI(const uint32 diff)
{
    GetScript()->OnUpdate(diff);
    UpdatePath(diff);
    UpdateDespawn(diff);

    /// @todo move to void
    if (mFollowGuid)
    {
        if (mFollowArrivedTimer < diff)
        {
            if (me->FindNearestCreature(mFollowArrivedEntry, INTERACTION_DISTANCE, true))
            {
                StopFollow(true);
                return;
            }

            mFollowArrivedTimer = 1000;
        }
        else
            mFollowArrivedTimer -= diff;
    }

    if (!IsAIControlled())
        return;

    if (!UpdateVictim())
        return;

    if (mCanAutoAttack)
        DoMeleeAttackIfReady();
}

bool SmartAI::IsEscortInvokerInRange()
{
    if (ObjectVector const* targets = GetScript()->GetStoredTargetVector(SMART_ESCORT_TARGETS, *me))
    {
        if (targets->size() == 1 && GetScript()->IsPlayer((*targets->begin())))
        {
            Player* player = (*targets->begin())->ToPlayer();
            if (me->GetDistance(player) <= SMART_ESCORT_MAX_PLAYER_DIST)
                        return true;

            if (Group* group = player->GetGroup())
            {
                for (GroupReference* groupRef = group->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
                {
                    Player* groupGuy = groupRef->GetSource();

                    if (groupGuy->IsInMap(player) && me->GetDistance(groupGuy) <= SMART_ESCORT_MAX_PLAYER_DIST)
                        return true;
                }
            }
        }else
        {
            for (auto & target : *targets)
            {
                if (GetScript()->IsPlayer(target))
                {
                    if (me->GetDistance(target->ToPlayer()) <= SMART_ESCORT_MAX_PLAYER_DIST)
                        return true;
                }
            }
        }
    }
    return true;//escort targets were not set, ignore range check
}

void SmartAI::MovepointReached(uint32 id)
{
    if (id != SMART_ESCORT_LAST_OOC_POINT && mLastWPIDReached != id)
        GetScript()->ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED, nullptr, id);

    mLastWPIDReached = id;
    mWPReached = true;
}

void SmartAI::MovementInform(uint32 MovementType, uint32 Data)
{
    if ((MovementType == POINT_MOTION_TYPE && Data == SMART_ESCORT_LAST_OOC_POINT) || MovementType == FOLLOW_MOTION_TYPE)
        me->ClearUnitState(UNIT_STATE_EVADE);

    GetScript()->ProcessEventsFor(SMART_EVENT_MOVEMENTINFORM, nullptr, MovementType, Data);
    if (MovementType != POINT_MOTION_TYPE || !HasEscortState(SMART_ESCORT_ESCORTING))
        return;

    MovepointReached(Data);
}

void SmartAI::RemoveAuras()
{
    Unit::AuraMap& auras = me->GetAuras();
    for(auto itr = auras.begin(); itr != auras.end();)
    {
        if (!itr->second->IsPassive() && itr->second->GetCasterGUID() != me->GetGUID())
            me->RemoveAura(itr);
        else
            itr++;
    }
    /*
    /// @fixme: duplicated logic in CreatureAI::_EnterEvadeMode (could use RemoveAllAurasExceptType)
    Unit::AuraApplicationMap& appliedAuras = me->GetAppliedAuras();
    for (Unit::AuraApplicationMap::iterator iter = appliedAuras.begin(); iter != appliedAuras.end();)
    {
        Aura const* aura = iter->second->GetBase();
        if (!aura->IsPassive() && !aura->HasEffectType(SPELL_AURA_CLONE_CASTER) && aura->GetCasterGUID() != me->GetGUID())
            me->RemoveAura(iter);
        else
            ++iter;
    }
    */
}

void SmartAI::EnterEvadeMode(EvadeReason why)
{
    if (mEvadeDisabled)
    {
        GetScript()->ProcessEventsFor(SMART_EVENT_EVADE);
        return;
    }

    if (!_EnterEvadeMode())
        return;

    GetScript()->ProcessEventsFor(SMART_EVENT_EVADE);//must be after aura clear so we can cast spells from db

    SetRun(mRun);
    
    if (HasEscortState(SMART_ESCORT_ESCORTING))
    {
        AddEscortState(SMART_ESCORT_RETURNING);
        ReturnToLastOOCPos();
    }
    else if (mFollowGuid)
    {
        if (Unit* target = ObjectAccessor::GetUnit(*me, mFollowGuid))
            me->GetMotionMaster()->MoveFollow(target, mFollowDist, mFollowAngle);
    }
    else if (!me->IsHomeless()) {
        me->GetMotionMaster()->MoveTargetedHome();
    }

    if (!HasEscortState(SMART_ESCORT_ESCORTING))//dont mess up escort movement after combat
        SetRun(mRun);
}

void SmartAI::MoveInLineOfSight(Unit* who)
{
    if (!who)
        return;

    GetScript()->OnMoveInLineOfSight(who);

    if (!IsAIControlled())
        return;

    CreatureAI::MoveInLineOfSight(who);
}

void SmartAI::JustRespawned()
{
    mDespawnTime = 0;
    mDespawnState = 0;
    mEscortState = SMART_ESCORT_NONE;
	me->SetVisible(true);
    if (me->GetFaction() != me->GetCreatureTemplate()->faction)
        me->RestoreFaction();
    GetScript()->ProcessEventsFor(SMART_EVENT_RESPAWN);
    mJustReset = true;
    JustReachedHome();
    mFollowGuid = 0;//do not reset follower on Reset(), we need it after combat evade
    mFollowDist = 0;
    mFollowAngle = 0;
    mFollowCredit = 0;
    mFollowArrivedTimer = 1000;
    mFollowArrivedEntry = 0;
    mFollowCreditType = 0;
}

int SmartAI::Permissible(const Creature* creature)
{
    if (creature->GetAIName() == SMARTAI_AI_NAME)
        return PERMIT_BASE_SPECIAL;
    return PERMIT_BASE_NO;
}

void SmartAI::JustReachedHome()
{
    GetScript()->OnReset();

    if (!mJustReset)
    {
        GetScript()->ProcessEventsFor(SMART_EVENT_REACHED_HOME);

        if (!UpdateVictim() && me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE && me->GetWaypointPathId())
            me->GetMotionMaster()->MovePath(me->GetWaypointPathId());
    }

    mJustReset = false;
}

void SmartAI::EnterCombat(Unit* enemy)
{
    if (IsAIControlled())
        me->InterruptNonMeleeSpells(false); // must be before ProcessEvents

    GetScript()->ProcessEventsFor(SMART_EVENT_AGGRO, enemy);

    if (!IsAIControlled())
        return;

    mLastOOCPos = me->GetPosition();
    SetRun(mRun);
    if (me->GetMotionMaster()->GetMotionSlotType(MOTION_SLOT_ACTIVE) == POINT_MOTION_TYPE)
        me->GetMotionMaster()->MovementExpired();
}

void SmartAI::JustDied(Unit* killer)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_DEATH, killer);
    if (HasEscortState(SMART_ESCORT_ESCORTING))
    {
        EndPath(true, true);
        me->StopMoving(); //force stop
        me->GetMotionMaster()->MoveIdle();
    }
}

void SmartAI::KilledUnit(Unit* victim)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_KILL, victim);
}

void SmartAI::AttackedUnitDied(Unit* attacked)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_ATTACKED_UNIT_DIED, attacked);
}

void SmartAI::JustSummoned(Creature* creature)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_SUMMONED_UNIT, creature);
}

void SmartAI::AttackStart(Unit* who)
{
    // xinef: dont allow charmed npcs to act on their own
    if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
    {
        if (who && mCanAutoAttack)
            me->Attack(who, true);
        return;
    }

    if(!me->IsInCombat())
        EnterCombat(who);

    if (who && me->Attack(who, me->IsWithinMeleeRange(who)))
    {
        if (mCanCombatMove)
        {
            SetRun(mRun);
            me->GetMotionMaster()->MoveChase(who);
        }
    }
}

void SmartAI::SpellHit(Unit* unit, const SpellInfo* spellInfo)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_SPELLHIT, unit, 0, 0, false, spellInfo);
}

void SmartAI::SpellHitTarget(Unit* target, const SpellInfo* spellInfo)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_SPELLHIT_TARGET, target, 0, 0, false, spellInfo);
}

void SmartAI::DamageTaken(Unit* doneBy, uint32& damage)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_DAMAGED, doneBy, damage);

    // don't allow players to use unkillable units
    if (!IsAIControlled() && mInvincibilityHpLevel && (damage >= me->GetHealth() - mInvincibilityHpLevel))
    {
        damage = 0;
        me->SetHealth(mInvincibilityHpLevel);
    }
}

void SmartAI::HealReceived(Unit* doneBy, uint32& addhealth)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_RECEIVE_HEAL, doneBy, addhealth);
}

void SmartAI::ReceiveEmote(Player* player, uint32 textEmote)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_RECEIVE_EMOTE, player, textEmote);
}

void SmartAI::IsSummonedBy(Unit* summoner)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_JUST_SUMMONED, summoner);
}

void SmartAI::DamageDealt(Unit* doneTo, uint32& damage, DamageEffectType /*damagetype*/)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_DAMAGED_TARGET, doneTo, damage);
}

void SmartAI::SummonedCreatureDespawn(Creature* unit)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_SUMMON_DESPAWNED, unit);
}

void SmartAI::CorpseRemoved(uint32& respawnDelay)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_CORPSE_REMOVED, nullptr, respawnDelay);
}

void SmartAI::PassengerBoarded(Unit* who, int8 seatId, bool apply)
{
#ifndef LICH_KING
    TC_LOG_ERROR("misc","SmartAI::PassengerBoarded was called while core isn't compiled for LK");
#endif
    GetScript()->ProcessEventsFor(apply ? SMART_EVENT_PASSENGER_BOARDED : SMART_EVENT_PASSENGER_REMOVED, who, uint32(seatId), 0, apply);
}

void SmartAI::InitializeAI()
{
    GetScript()->OnInitialize(me);
    if (!me->IsDead())
    mJustReset = true;
    JustReachedHome();
    GetScript()->ProcessEventsFor(SMART_EVENT_RESPAWN);
}

void SmartAI::OnCharmed(Unit* charmer, bool apply)
{
    if (apply) // do this before we change charmed state, as charmed state might prevent these things from processing
    {
        if (HasEscortState(SMART_ESCORT_ESCORTING | SMART_ESCORT_PAUSED | SMART_ESCORT_RETURNING))
            EndPath(true);
        me->StopMoving();
    }
    mIsCharmed = apply;

    me->NeedChangeAI = true;

    if (!apply && !me->IsInEvadeMode())
    {
        if (mCanRepeatPath)
            StartPath(mRun, GetScript()->GetPathId(), true);
        else
            me->SetWalk(!mRun);

        if (charmer)
            AttackStart(charmer);
    }

    GetScript()->ProcessEventsFor(SMART_EVENT_CHARMED, nullptr, 0, 0, apply);
}

bool SmartAI::CanAIAttack(Unit const* /*who*/) const
{
    return !(me->HasReactState(REACT_PASSIVE));
}

void SmartAI::DoAction(int32 param)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_ACTION_DONE, nullptr, param);
}

uint32 SmartAI::GetData(uint32 /*id*/) const
{
    return 0;
}

void SmartAI::SetData(uint32 id, uint32 value, Unit* setter)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_DATA_SET, setter, id, value);
}

void SmartAI::SetGUID(uint64 /*guid*/, int32 /*id*/) { }

uint64 SmartAI::GetGUID(int32 /*id*/) const
{
    return 0;
}

void SmartAI::SetRun(bool run)
{
    me->SetWalk(!run);
    mRun = run;
}

void SmartAI::SetDisableGravity(bool fly)
{
    me->SetDisableGravity(fly);
}

void SmartAI::SetCanFly(bool fly)
{
    me->SetCanFly(fly);
}

void SmartAI::SetSwim(bool swim)
{
    if (swim)
        me->AddUnitMovementFlag(MOVEMENTFLAG_SWIMMING);
    else
        me->RemoveUnitMovementFlag(MOVEMENTFLAG_SWIMMING);
}

void SmartAI::SetEvadeDisabled(bool disable)
{
    mEvadeDisabled = disable;
}

bool SmartAI::GossipHello(Player* player)
{
    _gossipReturn = false;
    GetScript()->ProcessEventsFor(SMART_EVENT_GOSSIP_HELLO, player);
    return _gossipReturn;
}

bool SmartAI::GossipSelect(Player* player, uint32 sender, uint32 action)
{
    _gossipReturn = false;
    GetScript()->ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT, player, sender, action);
    return _gossipReturn;
}

bool SmartAI::GossipSelectCode(Player* /*player*/, uint32 /*sender*/, uint32 /*action*/, const char* /*code*/) 
{ 
    return false;
}

void SmartAI::QuestAccept(Player* player, Quest const* quest)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_ACCEPTED_QUEST, player, quest->GetQuestId());
}

void SmartAI::QuestReward(Player* player, Quest const* quest, uint32 opt)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_REWARD_QUEST, player, quest->GetQuestId(), opt);
}

bool SmartAI::sOnDummyEffect(Unit* caster, uint32 spellId, uint32 effIndex)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_DUMMY_EFFECT, caster, spellId, effIndex);
    return true;
}

void SmartAI::SetCombatMove(bool on)
{
    if (mCanCombatMove == on)
        return;
    mCanCombatMove = on;
    if (!IsAIControlled())
        return;

    if (!HasEscortState(SMART_ESCORT_ESCORTING))
    {
        if (on && me->GetVictim())
        {
            if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
            {
                SetRun(mRun);
                me->GetMotionMaster()->MoveChase(me->GetVictim());
                me->CastStop();
            }
        }
        else if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != WAYPOINT_MOTION_TYPE)
        {
            if (me->HasUnitState(UNIT_STATE_CONFUSED_MOVE | UNIT_STATE_FLEEING_MOVE))
                return;

            me->GetMotionMaster()->MovementExpired();
            me->GetMotionMaster()->Clear(true);
            me->StopMoving();
            me->GetMotionMaster()->MoveIdle();
        }
    }
}

void SmartAI::SetFollow(Unit* target, float dist, float angle, uint32 credit, uint32 end, uint32 creditType)
{
    if (!target)
    {
        StopFollow(false);
        return;
    }

    mFollowGuid = target->GetGUID();
    mFollowDist = dist >= 0.0f ? dist : PET_FOLLOW_DIST;
    mFollowAngle = angle >= 0.0f ? angle : me->GetFollowAngle();
    mFollowArrivedTimer = 1000;
    mFollowCredit = credit;
    mFollowArrivedEntry = end;
    mFollowCreditType = creditType;
    SetRun(mRun);
    me->GetMotionMaster()->MoveFollow(target, mFollowDist, mFollowAngle);
}

void SmartAI::StopFollow(bool complete)
{
    mFollowGuid = 0;
    mFollowDist = 0;
    mFollowAngle = 0;
    mFollowCredit = 0;
    mFollowArrivedTimer = 1000;
    mFollowArrivedEntry = 0;
    mFollowCreditType = 0;
    SetDespawnTime(5000);
    me->StopMoving();
    me->GetMotionMaster()->MoveIdle();

    if (!complete)
        return;

    if (Player* player = ObjectAccessor::GetPlayer(*me, mFollowGuid))
    {
        if (!mFollowCreditType)
            player->RewardPlayerAndGroupAtEvent(mFollowCredit, me);
        else
            player->GroupEventHappens(mFollowCredit, me);
    }

    StartDespawn();
    GetScript()->ProcessEventsFor(SMART_EVENT_FOLLOW_COMPLETED);
}

void SmartAI::SetScript9(SmartScriptHolder& e, uint32 entry, Unit* invoker)
{
    if (invoker)
        GetScript()->mLastInvoker = invoker->GetGUID();
    GetScript()->SetScript9(e, entry);
}

/*
void SmartAI::sOnGameEvent(bool start, uint16 eventId)
{
    GetScript()->ProcessEventsFor(start ? SMART_EVENT_GAME_EVENT_START : SMART_EVENT_GAME_EVENT_END, NULL, eventId);
}*/

void SmartAI::OnSpellClick(Unit* clicker, bool& result)
{
    if (!result)
        return;

    GetScript()->ProcessEventsFor(SMART_EVENT_ON_SPELLCLICK, clicker);
}

void SmartAI::FriendlyKilled(Creature const* c, float range)
{
    GetScript()->ProcessEventsFor( SMART_EVENT_FRIENDLY_KILLED, (Unit*)c );
}

int SmartGameObjectAI::Permissible(const GameObject* g)
{
    if (g->GetAIName() == SMARTAI_GOBJECT_AI_NAME)
        return PERMIT_BASE_SPECIAL;

    return PERMIT_BASE_NO;
}

void SmartGameObjectAI::UpdateAI(uint32 diff)
{
    GetScript()->OnUpdate(diff);
}

void SmartGameObjectAI::InitializeAI()
{
    GetScript()->OnInitialize(me);
    GetScript()->ProcessEventsFor(SMART_EVENT_RESPAWN);
    //Reset();
}

void SmartGameObjectAI::Reset()
{
    GetScript()->OnReset();
}

// Called when a player opens a gossip dialog with the gameobject.
bool SmartGameObjectAI::GossipHello(Player* player)
{
    TC_LOG_DEBUG("scripts.ai","SmartGameObjectAI::GossipHello");
    _gossipReturn = false;
    GetScript()->ProcessEventsFor(SMART_EVENT_GOSSIP_HELLO, player, 0, 0, false, nullptr, me);
    return _gossipReturn;
}

bool SmartGameObjectAI::OnReportUse(Player* player)
{
    TC_LOG_DEBUG("scripts.ai", "SmartGameObjectAI::OnReportUse");
    _gossipReturn = false;
    GetScript()->ProcessEventsFor(SMART_EVENT_GOSSIP_HELLO, player, 1, 0, false, nullptr, me);
    return _gossipReturn;
}

// Called when a player selects a gossip item in the gameobject's gossip menu.
bool SmartGameObjectAI::GossipSelect(Player* player, uint32 sender, uint32 action)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT, player, sender, action, false, nullptr, me);
    return false;
}

// Called when a player selects a gossip with a code in the gameobject's gossip menu.
bool SmartGameObjectAI::GossipSelectCode(Player* /*player*/, uint32 /*sender*/, uint32 /*action*/, const char* /*code*/)
{
    return false;
}

// Called when a player accepts a quest from the gameobject.
void SmartGameObjectAI::QuestAccept(Player* player, Quest const* quest)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_ACCEPTED_QUEST, player, quest->GetQuestId(), 0, false, nullptr, me);
}

// Called when a player selects a quest reward.
void SmartGameObjectAI::QuestReward(Player* player, Quest const* quest, uint32 opt)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_REWARD_QUEST, player, quest->GetQuestId(), opt, false, nullptr, me);
}

// Called when the gameobject is destroyed (destructible buildings only).
void SmartGameObjectAI::Destroyed(Player* player, uint32 eventId)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_DEATH, player, eventId, 0, false, nullptr, me);
}

void SmartGameObjectAI::SetData(uint32 id, uint32 value, Unit* setter)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_DATA_SET, setter, id, value);
}

void SmartGameObjectAI::SetScript9(SmartScriptHolder& e, uint32 entry, Unit* invoker)
{
    if (invoker)
        GetScript()->mLastInvoker = invoker->GetGUID();
    GetScript()->SetScript9(e, entry);
}

void SmartGameObjectAI::OnGameEvent(bool start, uint16 eventId)
{
    GetScript()->ProcessEventsFor(start ? SMART_EVENT_GAME_EVENT_START : SMART_EVENT_GAME_EVENT_END, nullptr, eventId);
}

void SmartGameObjectAI::OnStateChanged(GOState state, Unit* unit)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_GO_STATE_CHANGED, unit, state);
}

void SmartGameObjectAI::OnLootStateChanged(LootState state, Unit* unit)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_GO_LOOT_STATE_CHANGED, unit, state);
}

void SmartGameObjectAI::EventInform(uint32 eventId)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_GO_EVENT_INFORM, nullptr, eventId);
}

void SmartGameObjectAI::SpellHit(Unit* unit, const SpellInfo* spellInfo)
{
    GetScript()->ProcessEventsFor(SMART_EVENT_SPELLHIT, unit, 0, 0, false, spellInfo);
}

class SmartTrigger : public AreaTriggerScript
{
    public:

        SmartTrigger() : AreaTriggerScript("SmartTrigger") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger) override
        {
            if (!player->IsAlive())
                return false;

            TC_LOG_DEBUG("scripts.ai", "AreaTrigger %u is using SmartTrigger script", trigger->id);
            SmartScript script;
            script.OnInitialize(nullptr, trigger);
            script.ProcessEventsFor(SMART_EVENT_AREATRIGGER_ONTRIGGER, player, trigger->id);
            return true;
        }
};

void AddSC_SmartScripts()
{
    new SmartTrigger();
}