
#include "PointMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "World.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "Player.h"
#include "CreatureGroups.h"

template<class T>
void PointMovementGenerator<T>::LaunchMove(T* owner)
{
    Movement::MoveSplineInit init(owner);
    init.MoveTo(G3D::Vector3(_destination.GetPositionX(), _destination.GetPositionY(), _destination.GetPositionZ()), _generatePath, _forceDestination);
    if (_speed > 0.0f)
        init.SetVelocity(_speed);

    if (_destination.GetOrientation() != 0.0f)
        init.SetFacing(_destination.GetOrientation());

    init.Launch();
}

//----- Point Movement Generator
template<class T>
bool PointMovementGenerator<T>::DoInitialize(T* owner)
{
    if (!owner->IsStopped())
        owner->StopMoving();

    owner->AddUnitState(UNIT_STATE_ROAMING|UNIT_STATE_ROAMING_MOVE);

    if (_movementId == EVENT_CHARGE_PREPATH)
        return true;

    //if cannot move : init generator but don't move for now
    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || owner->IsMovementPreventedByCasting())
        return true;

    LaunchMove(owner);
    return true;
}

template<class T>
bool PointMovementGenerator<T>::DoUpdate(T* owner, uint32 /*diff*/)
{
    if (!owner)
        return false;

    if (_movementId == EVENT_CHARGE_PREPATH)
        return !owner->movespline->Finalized();

    if (owner->HasUnitState(UNIT_STATE_NOT_MOVE) || owner->IsMovementPreventedByCasting())
    {
        owner->ClearUnitState(UNIT_STATE_ROAMING_MOVE);
        return true;
    }

    owner->AddUnitState(UNIT_STATE_ROAMING_MOVE);

    if (_recalculateSpeed && !owner->movespline->Finalized())
    {
        _recalculateSpeed = false;
        LaunchMove(owner);
    }

    return !owner->movespline->Finalized();
}

template<class T>
void PointMovementGenerator<T>::DoFinalize(T* unit)
{
    if (unit->HasUnitState(UNIT_STATE_CHARGING))
        unit->ClearUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);

    if (unit->movespline->Finalized())
        MovementInform(unit);
}

template<class T>
void PointMovementGenerator<T>::DoReset(T* unit)
{
    if (!unit->IsStopped())
        unit->StopMoving();

    unit->AddUnitState(UNIT_STATE_ROAMING|UNIT_STATE_ROAMING_MOVE);
}

template<class T>
void PointMovementGenerator<T>::MovementInform(T* /*unit*/) { }

template <> void PointMovementGenerator<Creature>::MovementInform(Creature* unit)
{
    if (unit->AI())
        unit->AI()->MovementInform(POINT_MOTION_TYPE, _movementId);
}

template bool PointMovementGenerator<Player>::DoInitialize(Player*);
template bool PointMovementGenerator<Creature>::DoInitialize(Creature*);
template void PointMovementGenerator<Player>::DoFinalize(Player*);
template void PointMovementGenerator<Creature>::DoFinalize(Creature*);
template void PointMovementGenerator<Player>::DoReset(Player*);
template void PointMovementGenerator<Creature>::DoReset(Creature*);
template bool PointMovementGenerator<Player>::DoUpdate(Player*, uint32);
template bool PointMovementGenerator<Creature>::DoUpdate(Creature*, uint32);

void AssistanceMovementGenerator::Finalize(Unit* unit, bool /* premature */)
{
    unit->ToCreature()->SetNoCallAssistance(false);
    unit->ToCreature()->CallAssistance();
    if (unit->IsAlive())
        unit->GetMotionMaster()->MoveSeekAssistanceDistract(sWorld->getConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_DELAY));
}

bool EffectMovementGenerator::Update(Unit* unit, uint32)
{
    return !unit->movespline->Finalized();
}

void EffectMovementGenerator::Finalize(Unit* unit, bool /* premature */)
{
    if (unit->GetTypeId() != TYPEID_UNIT)
        return;

    // Need restore previous movement since we have no proper states system
    if (unit->IsAlive() && !unit->HasUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING))
    {
        if (Unit* victim = unit->GetVictim())
            unit->GetMotionMaster()->MoveChase(victim);
        else
            unit->GetMotionMaster()->Initialize();
    }

    if (unit->ToCreature()->AI())
        unit->ToCreature()->AI()->MovementInform(EFFECT_MOTION_TYPE, m_Id);
}