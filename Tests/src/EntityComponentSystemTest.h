// ------------------------------------------------------------------------------
//
// Lotus
//    Copyright 2022 Matthew Rogers
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//
// File Name: EntityComponentSystemTest.h
// Date File Created: 8/20/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "Test.h"

#include <Lotus/Components/Entity.h>
#include <Lotus/Components/Transform.h>

#include <iostream>
#include <ctime>

using namespace lotus;

class EngineTest : public Test
{
public:
    bool Init() override
    {
        srand((uint32) time(nullptr));
        return true;
    }


    void Run() override
    {
        do {
            for (uint32 i = 0; i < 10000; ++i)
            {
                CreateRandom();
                RemoveRandom();
                mNumEntities = (u32) mEntities.size();
            }
            PrintResults();
        } while (getchar() != 'q');
    }
    void Shutdown() override { }

private:
    void CreateRandom()
    {
        uint32 count = rand() % 20;
        if (mEntities.empty()) count = 1000;
        transform::create_info transformDesc {};
        game_entity::create_info    entityDesc {
            &transformDesc,
        };

        while (count > 0)
        {
            ++mAdded;
            game_entity::entity ent = create(entityDesc);
            assert(ent.is_valid());
            mEntities.push_back(ent);
            assert(is_alive(ent.get_id()));
            --count;
        }
    }
    void RemoveRandom()
    {
        uint32 count = rand() % 20;
        if (mEntities.size() < 1000) return;

        while (count > 0)
        {
            const uint32         index = (uint32) rand() % (uint32) mEntities.size();
            const game_entity::entity ent   = mEntities [ index ];
            assert(ent.is_valid());
            if (ent.is_valid())
            {
                game_entity::remove(ent.get_id());
                mEntities.erase(mEntities.begin() + index);
                assert(!is_alive(ent.get_id()));
                ++mRemoved;
            }
            --count;
        }
    }

    void PrintResults()
    {
        std::cout << "Entities created: " << mAdded << "\n";
        std::cout << "Entities removed: " << mRemoved << "\n";
    }

private:
    utl::vector<game_entity::entity> mEntities;

    uint32 mAdded { 0 };
    uint32 mRemoved { 0 };
    uint32 mNumEntities { 0 };
};
