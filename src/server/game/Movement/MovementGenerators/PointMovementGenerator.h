/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITY_POINTMOVEMENTGENERATOR_H
#define TRINITY_POINTMOVEMENTGENERATOR_H

#include "MovementGenerator.h"
#include "FollowerReference.h"

template<class T>
class PointMovementGenerator : public MovementGeneratorMedium< T, PointMovementGenerator<T> >
{
    public:
        //_o = 0 means no orientation used. Use values like 0.00001 for orientation 0.
        PointMovementGenerator(uint32 _id, float _x, float _y, float _z, float _o, bool _generatePath, float _speed = 0.0f, bool forceDestination = false) : _movementId(_id),
            _destination(_x, _y, _z, _o), _speed(_speed), _generatePath(_generatePath), _forceDestination(forceDestination), _recalculateSpeed(false) { }

        bool DoInitialize(T*);
        void DoFinalize(T*);
        void DoReset(T*);
        bool DoUpdate(T*, uint32);

        void MovementInform(T*);

        void UnitSpeedChanged() override { _recalculateSpeed = true; }

        MovementGeneratorType GetMovementGeneratorType() override { return POINT_MOTION_TYPE; }

        void GetDestination(float& x, float& y, float& z) const { x = _destination.GetPositionX(); y = _destination.GetPositionY(); z = _destination.GetPositionZ(); }

    private:
        uint32 _movementId;
        Position _destination;
        float _speed;
        bool _recalculateSpeed;

        bool _generatePath;
        bool _forceDestination;

        void LaunchMove(T*);
};

class AssistanceMovementGenerator : public PointMovementGenerator<Creature>
{
    public:
        AssistanceMovementGenerator(float _x, float _y, float _z) :
            PointMovementGenerator<Creature>(0, _x, _y, _z, 0.0f, true) { }

        MovementGeneratorType GetMovementGeneratorType() override { return ASSISTANCE_MOTION_TYPE; }
        void Finalize(Unit*, bool) override;
};

// Does almost nothing - just doesn't allows previous movegen interrupt current effect.
class EffectMovementGenerator : public MovementGenerator
{
    public:
        explicit EffectMovementGenerator(uint32 Id) : m_Id(Id) { }
        bool Initialize(Unit*) override { return true;  }
        void Finalize(Unit*, bool) override;
        void Reset(Unit*) override { }
        bool Update(Unit*, uint32) override;
        MovementGeneratorType GetMovementGeneratorType() override { return EFFECT_MOTION_TYPE; }
    private:
        uint32 m_Id;
};

#endif