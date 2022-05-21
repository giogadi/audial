#pragma once

void LoadTestScript(GameManager& g) {
    // Camera
    Entity* camera = g._entityManager->AddEntity();
    {
        camera->_name = "camera";

        auto pTrans = camera->AddComponent<TransformComponent>().lock();

        float angle = 45.f * kDeg2Rad;
        Vec3 dir(0.f, sin(angle), cos(angle));
        float dist = 15.f;
        pTrans->SetPos(dist * dir);
        Mat3 rot = Mat3::FromAxisAngle(Vec3(1.f, 0.f, 0.f), -angle);
        pTrans->SetRot(rot);

        auto pCamera = camera->AddComponent<CameraComponent>().lock();

        camera->ConnectComponents(g);
    }

    // Light
    Entity* light = g._entityManager->AddEntity();
    {
        light->_name = "light";

        auto pTrans = light->AddComponent<TransformComponent>().lock();
        pTrans->SetPos(Vec3(0.f, 3.f, 0.f));

        auto pLight = light->AddComponent<LightComponent>().lock();
        pLight->_ambient.Set(0.2f, 0.2f, 0.2f);
        pLight->_diffuse.Set(1.f, 1.f, 1.f);

        light->ConnectComponents(g);
    }

    // Cube1
    Entity* cube1 = g._entityManager->AddEntity();
    {
        cube1->_name = "cube1";
        auto tComp = cube1->AddComponent<TransformComponent>().lock();
        auto modelComp = cube1->AddComponent<ModelComponent>().lock();
        modelComp->_modelId = std::string("wood_box");
        TransformComponent* t = tComp.get();
        auto beepComp = cube1->AddComponent<BeepOnHitComponent>().lock();
        beepComp->_synthChannel = 0;
        beepComp->_midiNotes = { 69, 73 };
        auto rbComp = cube1->AddComponent<RigidBodyComponent>().lock();
        rbComp->_localAabb = MakeCubeAabb(0.5f);
        auto velComp = cube1->AddComponent<VelocityComponent>().lock();
        velComp->_angularY = 2*kPi;

        cube1->ConnectComponents(g);
    }

    // Cube2
    Entity* cube2 = g._entityManager->AddEntity();
    cube2->_name = "cube2";
    {
        auto tComp = cube2->AddComponent<TransformComponent>().lock();
        auto modelComp = cube2->AddComponent<ModelComponent>().lock();
        modelComp->_modelId = std::string("wood_box");
        TransformComponent* t = tComp.get();
        t->_transform.Translate(Vec3(-2.f,0.f,0.f));
        auto rbComp = cube2->AddComponent<RigidBodyComponent>().lock();
        rbComp->_localAabb = MakeCubeAabb(0.5f);
        rbComp->_static = false;
        auto controller = cube2->AddComponent<PlayerControllerComponent>().lock();

        cube2->ConnectComponents(g);
    }

    // drone sequencer
    Entity* droneSeq = g._entityManager->AddEntity();
    {
        droneSeq->_name = "drone_seq";
        auto seqComp = droneSeq->AddComponent<SequencerComponent>().lock();
        {
            audio::Event e;
            e.channel = 1;
            e.type = audio::EventType::NoteOn;
            e.midiNote = 45;
            e.timeInTicks = 0;
            seqComp->AddToSequence(e);

            e.midiNote = 49;
            seqComp->AddToSequence(e);

            e.midiNote = 52;
            seqComp->AddToSequence(e);

            e.type = audio::EventType::SynthParam;
            e.param = audio::SynthParamType::Gain;
            e.newParamValue = 0.5;
            seqComp->AddToSequence(e);
        }
        droneSeq->ConnectComponents(g);
    }

    // Init event queue with a synth sequence
    {
        // double const audioTime = Pa_GetStreamTime(audioContext._stream);
        // InitEventQueueWithSequence(&audioContext._eventQueue, beatClock);
        // InitEventQueueWithParamSequence(&audioContext._eventQueue, beatClock);
    }

}