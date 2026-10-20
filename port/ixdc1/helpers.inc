float unitRotation(CFrameVu1 *frame, float heading) {
    float rotation[4];
    float delta;
    float abs_delta;

    frame->GetRotation(rotation);
    delta = heading - rotation[1];

    if (delta <= 0.0f) {
        abs_delta = -1.0f * delta;
    } else {
        abs_delta = delta;
    }

    if (abs_delta <= PI_D) {
        if (abs_delta <= TWELFTH_PI_D) {
            delta = 0.0f;
        }
    } else {
        abs_delta = TWO_PI_D - abs_delta;

        if (abs_delta <= TWELFTH_PI_D) {
            delta = 0.0f;
        }
    }

    if (delta > 0.0f) {
        if (delta <= PI_D) {
            rotation[1] += TWELFTH_PI;
        } else {
            rotation[1] -= TWELFTH_PI;
        }
    }

    if (delta < 0.0f) {
        if (delta >= -PI_D) {
            rotation[1] -= TWELFTH_PI;
        } else {
            rotation[1] += TWELFTH_PI;
        }
    }

    if (0.0f == delta) {
        rotation[1] = heading;
    }

    if (rotation[1] <= -PI_D) {
        rotation[1] += TWO_PI;
    }

    if (rotation[1] >= PI_D) {
        rotation[1] -= TWO_PI;
    }

    return rotation[1];
}

void ParabolicInitialVector(float *velocity, float *from, float *to, float gravity, float time) {
    velocity[0] = (to[0] - from[0]) / time;
    velocity[1] = (2.0f * (to[1] - from[1]) - time * (gravity * time)) / (2.0f * time);
    velocity[2] = (to[2] - from[2]) / time;
    velocity[3] = 1.0f;
    float negative_one = -1.0f;
    velocity[1] = velocity[1] * negative_one;
}

void getCharacterVector(float *vector, float pitch) {
    sceVu0FVECTOR forward = {0.0f, 0.0f, 1.0f, 1.0f};
    sceVu0FVECTOR rotation;
    sceVu0FMATRIX matrix;
    sceVu0FMATRIX yaw;
    sceVu0FMATRIX tilt;

    CharaMain.frame->GetRotation(rotation);
    sceVu0UnitMatrix(yaw);
    sceVu0RotMatrixY(yaw, yaw, rotation[1]);
    sceVu0UnitMatrix(tilt);
    sceVu0RotMatrixX(tilt, tilt, pitch);
    sceVu0MulMatrix(matrix, yaw, tilt);
    sceVu0ApplyMatrix(vector, matrix, forward);
}

int CCollisionData::Set(float *position, int damage, int life, float radius, float scale_rate, int target_mask, int kind, int flags, int attribute) {
    for (int i = 0; i < 96; i++) {
        if (active[i] == 0) {
            active[i] = 1;
            sceVu0CopyVector(hit[i].pos, position);
            hit[i].base_point[0] = hit[i].base_point[1] = hit[i].base_point[2] = 0.0f;
            hit[i].base_point[3] = 1.0f;
            hit[i].velocity[0] = hit[i].velocity[1] = hit[i].velocity[2] = 0.0f;
            hit[i].velocity[0] = 1.0f;
            hit[i].damage = damage;
            hit[i].scale_rate = scale_rate;
            hit[i].life = life;
            hit[i].radius = radius;
            hit[i].target_mask = target_mask;
            hit[i].kind = kind;
            hit[i].flags = flags;
            hit[i].attribute = attribute;
            hit[i].owner = -1;
            hit[i].monster_no = -1;
            hit[i].attack_no = -1;
            hit[i].vs_monster = NULL;
            hit[i].weapon_flags = 1;
            hit[i].target_kind = -1;
            hit[i].knockback_origin[0] = hit[i].knockback_origin[1] = hit[i].knockback_origin[2] = 0.0f;
            hit[i].knockback_origin[3] = 1.0f;
            hit[i].knockback_speed = 0.0f;
            hit[i].knockback_decay = 0.0f;
            hit[i].knockback_mode = 0;
            hit[i].phase = hit[i].ready_phase = 0;
            now_hit = i;
            return i;
        }
    }
    // With every record in use nothing is set and no index is returned.
}

void CCollisionData::SetKickBack(float *origin, float speed, float decay, int mode) {
    hit[now_hit].knockback_origin[0] = origin[0];
    hit[now_hit].knockback_origin[1] = origin[1];
    hit[now_hit].knockback_origin[2] = origin[2];
    hit[now_hit].knockback_speed = speed;
    hit[now_hit].knockback_decay = decay;
    hit[now_hit].knockback_mode = mode;
}

int StatusErrCheck(int status) {
    return (UserStatus->ailments[UserStatus->cur_chara] & status) ? 1 : 0;
}

int element_tbl[6] = {1, 2, 4, 8, 0x10, 0};

int GetWeaponElementAttr(int element) {
    if (element < 0 || element > 5) {
        element = 5;
    }

    return element_tbl[element];
}

BT_SHOT_EFFECT MyEntryEffect_TOAN = {
    "c01_fuusya", 0, 0,
    {0.0f, 0.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f, 0.0f},
    160, 8, 8, 2, 2,
    {0, -1, -1, -1}, -1, {0, 0}, 0.0f, 0,
    {-1, -1, -1, -1},
};

BT_SHOT_EFFECT MyEntryEffect_GORO = {
    "c06a_tameex", 0, 0,
    {0.0f, 0.0f, 0.0f, 0.0f}, {15.0f, 0.0f, 0.0f, 0.0f},
    160, 8, 0, 2, 2,
    {0, -1, -1, -1}, -1, {0, 0}, 0.0f, 0,
    {-1, -1, -1, -1},
};

BT_SHOT_EFFECT MyEntryEffect_UNGAGA = {
    "c10a_ex", 0, 1,
    {0.2f, 0.9f, 0.2f, 0.2f}, {8.0f, 8.0f, 6.0f, 6.0f},
    90, 20, 8, 3, 2,
    {0, 1, 2, 2}, -1, {0, 0}, 0.0f, 0,
    {-1, -1, -1, -1},
};

void setShotVector(float *velocity, float speed, float angle_y, float angle_x) {
    sceVu0FMATRIX rotation;
    sceVu0FMATRIX identity;

    velocity[0] = 0.0f;
    velocity[1] = 0.0f;
    velocity[2] = speed;
    velocity[3] = 1.0f;
    sceVu0UnitMatrix(identity);
    sceVu0RotMatrixX(rotation, identity, angle_x);
    sceVu0RotMatrixY(rotation, rotation, angle_y);
    sceVu0ApplyMatrix(velocity, rotation, velocity);
}

void CSHOT::step() {
    sceVu0FVECTOR hit_position;

    for (int shot = 0; shot < 12; shot++) {
        if (used[shot] == 0) {
            continue;
        }

        if (halted[shot] == 0) {
            SHOT_COLLISION_RESULT result = checkCollision(hit_position, pos[shot], vector[shot], 2, 2.0f);

            if (result == SHOT_COLLISION_NONE) {
                pos[shot][0] += vector[shot][0];
                pos[shot][1] += vector[shot][1];
                pos[shot][2] += vector[shot][2];
            } else {
                NowColData->Set(pos[shot], damage[shot], 1, 3.0f, 0.0f, 2, 2, 0, 0);
                NowColData->SetUserID(1, 0);
                s8              weapon_element = NowWeaponHave->best_elem;
                CCollisionData *attr_col = NowColData;
                attr_col->hit[attr_col->now_hit].flags = GetWeaponElementAttr(weapon_element);
                NowColData->hit[NowColData->now_hit].weapon_flags = NowWeaponHave->flags;
                NowColData->hit[NowColData->now_hit].vs_monster = NowWeaponHave->vs_monster;
                used[shot] = 0;
            }
        }

        life[shot]--;

        if (life[shot] <= 0) {
            used[shot] = 0;
        }
    }
}

void BattleActionOn_Jinn() {
    if (BtActStatus.action_on == 0 && (s32) BtActStatus.action_gauge == 100) {
        BtActStatus.action_step = 0;
        BtActStatus.charge_time = 0.0f;
        BtActStatus.action_no = 11;
        BtActStatus.action_on = 1;
        BtActStatus.action_gauge = 0.0f;
        driveNoInterpolate = 1;
        SndSePlay(SE_CHARA_ACTION, -1, 0);
    }
}

void BattleActionPlay_Jinn(CCharacter *chara, int aimed) {
    float now = chara->motion_type.state.time;
    int   damage = NowWeaponHave->attack;
    int   i;

    // A poisoned weapon hits twice as hard.
    if (StatusErrCheck(AILMENT_STAMINA) != 0) {
        damage *= 2;
    }

    if (BtActStatus.action_no == 11) {
        if (GamePad.On(PadInput_OK) == 0) {
            BtActStatus.action_step = 1;
        }

        BtActStatus.motion_no = 11;

        float end = (float) chara->motion_type.motion_info[11].end;

        // Letting go before the wind-up ends holds the swing; keeping it
        // pressed goes straight on to the throw.
        if (now >= end - 2.0f && now <= end) {
            if (BtActStatus.action_step == 0) {
                BtActStatus.action_no = 12;
            } else {
                BtActStatus.action_no = 13;
            }
        }
    }

    if (BtActStatus.action_no == 13) {
        BtActStatus.motion_no = 13;

        if (aimed == 0) {
            if (now > 251.0f && now < 252.0f) {
                SndSePlay(SE_CHARA_ACTION_2, -1, 0);
                SndSePlay(SE_CHARA_SHOUT_2, -1, 0);

                sceVu0FVECTOR world;
                sceVu0FVECTOR vector;
                float         offset[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                CFrame       *hand = NowWeapon->frame->SearchFrame("eff30");

                if (hand != NULL) {
                    hand->GetWorldPosition(world, offset);

                    if (lockOnTargetFlag == 0) {
                        float y = CharaMain.GetRotation()->y;

                        setShotVector(vector, 5.0f, y, 0.0f);
                    } else {
                        vector[0] = BtActStatus.target_position[0] - world[0];
                        vector[1] = BtActStatus.target_position[1] - world[1];
                        vector[2] = BtActStatus.target_position[2] - world[2];
                        sceVu0Normalize(vector, vector);
                        vector[0] *= 5.0f;
                        vector[1] *= 5.0f;
                        vector[2] *= 5.0f;
                    }

                    CSHOT *shots = NowShotData;

                    for (i = 0; i < 12; i++) {
                        if (shots->used[i] == 0) {
                            sceVu0CopyVector(shots->pos[i], world);
                            sceVu0CopyVector(shots->vector[i], vector);
                            shots->life[i] = 0x78;
                            shots->damage[i] = damage;
                            shots->size[i] = 1.0f;
                            shots->halted[i] = 0;
                            shots->start_flag[i] = 0;
                            shots->used[i] = 1;
                            break;
                        }
                    }

                    BtActStatus.frames_since_attack = 0;
                }

                SwordDmgCheck1(1.0f, 0);
            }
        } else {
#ifdef PAL_TIMING
            if (now > 21.0f && now < 21.8f) {
#else
            if (now > 21.0f && now < 21.5f) {
#endif
                SndSePlay(SE_CHARA_ACTION_2, -1, 0);
                SndSePlay(SE_CHARA_SHOUT_2, -1, 0);

                sceVu0FVECTOR world;
                sceVu0FVECTOR vector;
                float         offset[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                CFrame       *hand = NowWeapon->frame->SearchFrame("eff30");

                if (hand != NULL) {
                    hand->GetWorldPosition(world, offset);
                    setShotVector(vector, 3.5f, viewAngleH__2, viewAngleV__2);

                    CSHOT *shots = NowShotData;

                    for (i = 0; i < 12; i++) {
                        if (shots->used[i] == 0) {
                            sceVu0CopyVector(shots->pos[i], world);
                            sceVu0CopyVector(shots->vector[i], vector);
                            shots->life[i] = 0x78;
                            shots->damage[i] = damage;
                            shots->size[i] = 1.0f;
                            shots->halted[i] = 0;
                            shots->start_flag[i] = 0;
                            shots->used[i] = 1;
                            break;
                        }
                    }

                    BtActStatus.frames_since_attack = 0;
                }

                SwordDmgCheck1(1.0f, 0);
            }
        }

        float end = (float) chara->motion_type.motion_info[13].end;

        if (now >= end - 2.0f && now <= end) {
            BtActStatus.unk_028 = 0;
            BtActStatus.motion_no = 0;
            BtActStatus.action_on = 0;

            if (lockOnTargetFlag != 0) {
                BtActStatus.motion_no = 0x12;
            }

            // Retail tests the task here and does nothing with the answer.
            if (gameTask == GAME_TASK_EYE_CAMERA) {
                now = now;
            }
        }
    }

    if (BtActStatus.action_no == 12) {
        if (GamePad.On(PadInput_OK) == 0) {
            BtActStatus.action_step = 1;
        }

        BtActStatus.motion_no = 12;
        driveNoInterpolate = 1;

        if (BtActStatus.action_step != 0) {
            driveNoInterpolate = 1;
            BtActStatus.action_no = 13;
        }
    }
}
